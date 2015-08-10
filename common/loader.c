#include <alloca.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "loader.h"
#include "main.h"


static void    *_handle;

static char    *_name;

static int8_t (*_init)(const char *sysname);

static int8_t (*_destroy)(void);

static int8_t (*_coreinfo)(size_t *numcore, size_t *maxid);

static size_t (*_rdmsr_arr)(msrval_t *vals, const msradr_t *addrs,
			    const uint8_t *cores, size_t len);

static size_t (*_wrmsr_arr)(const msradr_t *addrs, const msrval_t *vals,
			    const uint8_t *cores, size_t len);

static size_t (*_rwmsr_arr)(const msradr_t *addrs, msrval_t *vals,
			    const uint8_t *cores, size_t len);


/*
 * Check if the operating system is GNU/Linux.
 * Even if a linux kernel is detected, it may be running in a virtual machine.
 * Return a positive value if GNU/Linux is detected, 0 otherwise.
 */
static uint8_t check_linux(void)
{
	size_t size;
	ssize_t ssize;
	char buffer[7], *ptr;
	pid_t uname;
	int fds[2];

	if (pipe(fds))
		return 0;

	if ((uname = fork())) {
		close(fds[1]);

		size = sizeof (buffer);
		ptr = buffer;
		while (size) {
			ssize = read(fds[0], ptr, size);
			if (ssize <= 0)
				break;
			ptr += ssize;
			size -= ssize;
		}
		while (size--)
			*ptr++ = '\0';
		
		wait(NULL);
		close(fds[0]);
	} else {
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);
		
		execlp("uname", "uname", "-s", NULL);
		exit(EXIT_FAILURE);
	}

	return !strcmp(buffer, "Linux\n");
}

static uint8_t check_xen(void)
{
	pid_t xl;
	int status;

	if (getuid() != 0 && verbose)
		vlog("need root privileges to detect all systems correctly");

	if ((xl = fork())) {
		wait(&status);
	} else {
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		execlp("xl", "xl", "info", NULL);
		exit(EXIT_FAILURE);
	}

	if (WEXITSTATUS(status) == EXIT_SUCCESS)
		return 1;
	return 0;
}

const char *probe_system(void)
{
	const char *system = NULL;
	
	if (check_linux())
		system = "linux";
	if (check_xen())
		system = "xen";

	return system;
}


static int8_t load_file(const char *system, const char *file)
{
	void *handle;
	char *error;

	handle = dlopen(file, RTLD_LAZY);
	if (!handle)
		return -1;

	dlerror();

#define LOAD_SYMBOL(symb)						\
	*(void **) (&_##symb) = dlsym(handle, #symb);			\
	error = dlerror();						\
	if (error) {							\
		if (verbose)						\
			vlog("missing function '%s' : '%s'", #symb, file); \
		dlclose(handle);					\
		return -1;						\
	}
	LOAD_SYMBOL(init)
	LOAD_SYMBOL(destroy)
	LOAD_SYMBOL(coreinfo)
	LOAD_SYMBOL(rdmsr_arr)
	LOAD_SYMBOL(wrmsr_arr)
	LOAD_SYMBOL(rwmsr_arr)
#undef LOAD_SYMBOL

	if (init(system)) {
		if (verbose)
			vlog("cannot initialize : '%s'", file);
		dlclose(handle);
		return -1;
	}

	_handle = handle;
	return 0;
}

static int8_t load_path(const char *system, const char *path,
			char *file, size_t flen)
{
	DIR *fh;
	struct dirent *entry;
	size_t plen = strlen(path);
	char *buffer = alloca(plen + 1 + sizeof(entry->d_name));
	char *fname = buffer + plen + 1;
	int8_t ret = -1;

	fh = opendir(path);
	if (fh == NULL)
		return -1;

	if (verbose)
		vlog("scanning path directory : '%s'", path);

	strcpy(buffer, path);
	buffer[plen] = '/';

	while ((entry = readdir(fh))) {
		strcpy(fname, entry->d_name);

		_name = buffer;
		
		if (!load_file(system, buffer)) {
			strncpy(file, buffer, flen);
			ret = 0;
			break;
		}
	}

	if (ret == 0)
		_name = strdup(file);
	else
		_name = NULL;
	
	closedir(fh);
	return ret;
}

int8_t load_module(const char *system, const char **paths, size_t plen,
		   char *file, size_t flen)
{
	size_t i;
	char *delim;
	const char *ptr;
	int8_t ret = -1;
	
	for (i=0; i<plen; i++) {
		ptr = paths[i];
		while ((delim = strchr(ptr, ':'))) {
			*delim = '\0';
			
			if (!load_path(system, ptr, file, flen))
				ret = 0;
			
			*delim = ':';
			ptr = delim + 1;

			if (ret == 0)
				goto out;
		}
		
		if (!load_path(system, ptr, file, flen)) {
			ret = 0;
			goto out;
		}
	}

 out:
	return ret;
}


void unload_module(void)
{
	if (_handle == NULL)
		return;

	dlclose(_handle);
	_handle = NULL;
	
	free(_name);
	_name = NULL;
}


int8_t init(const char *sysname)
{
	int8_t ret;
	const char *prev = module;

	module = _name;
	ret = _init(sysname);
	module = prev;

	return ret;
}

int8_t destroy(void)
{
	int8_t ret;
	const char *prev = module;

	module = _name;
	ret = _destroy();
	module = prev;

	return ret;
}


int8_t coreinfo(size_t *numcore, size_t *maxid)
{
	int8_t ret;
	const char *prev = module;

	module = _name;
	ret = _coreinfo(numcore, maxid);
	module = prev;

	return ret;
}


size_t rdmsr_arr(msrval_t *vals, const msradr_t *addrs, const uint8_t *cores,
		 size_t len)
{
	size_t ret;
	const char *prev = module;

	module = _name;
	ret = _rdmsr_arr(vals, addrs, cores, len);
	module = prev;

	return ret;
}
	
size_t wrmsr_arr(const msradr_t *addrs, const msrval_t *vals,
		 const uint8_t *cores, size_t len)
{
	size_t ret;
	const char *prev = module;

	module = _name;
	ret = _wrmsr_arr(addrs, vals, cores, len);
	module = prev;

	return ret;
}

size_t rwmsr_arr(const msradr_t *addrs, msrval_t *vals, const uint8_t *cores,
		 size_t len)
{
	size_t ret;
	const char *prev = module;

	module = _name;
	ret = _rwmsr_arr(addrs, vals, cores, len);
	module = prev;

	return ret;
}
