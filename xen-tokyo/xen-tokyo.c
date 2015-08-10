#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xenctrl.h>
#include <xenguest.h>
#include <xc_private.h>

#include <sys/wait.h>

#include "main.h"
#include "rwmsr.h"


#define DEFAULT_LINE_LENGTH   128

#define HYPERCALL_BIGOS_RDMSR    -2
#define HYPERCALL_BIGOS_WRMSR    -3


int8_t init(const char *sysname)
{
	if (strcmp(sysname, "xen-tokyo"))
		return -1;
	if (getuid() != 0) {
		if (verbose)
			vlog("need root privileges to work");
		return -1;
	}
	
	return 0;
}

int8_t destroy(void)
{
	return 0;
}


static uint8_t probe_keyval(char *value, size_t len, int fd, const char *key)
{
	size_t blen = DEFAULT_LINE_LENGTH, off = 0;
	size_t klen = strlen(key);
	size_t i, end, nl;
	char *buf = malloc(blen), *tmp;
	uint8_t found = 0;
	char *ret = NULL;
	ssize_t rd;

	if (!buf)
		goto out;

	while ((rd = read(fd, buf + off, blen - off))) {
		end = (size_t) (off + rd);

		for (i=0; i<end; i++)
			if (buf[i] == '\n')
				break;

		if (i == end) {
			if (end == blen) {
				blen *= 2;
				tmp = realloc(buf, blen);
				if (!tmp)
					goto err;
				buf = tmp;
			}
			off = end;
			continue;
		}

		nl = (size_t) i;

		if (!strncmp(buf, key, klen)) {
			for (i=klen; i<nl; i++) {
				if (buf[i] == ':')
					break;
				if (buf[i] != ' ' && buf[i] != '\t')
					break;
			}

			if (buf[i] == ':') {
				buf[nl] = '\0';
				ret = buf + i + 1;
				while (*ret && (*ret == ' ' || *ret == '\t'))
					ret++;
				strncpy(value, ret, len);
				found = 1;
				break;
			}
		}

		memmove(buf, buf + nl + 1, end - nl);
		off = end - nl - 1;
	}

 err:
	free(buf);
 out:
	return found;
}

int8_t coreinfo(size_t *numcore, size_t *maxid)
{
	int fds[2];
	pid_t xlinfo;
	char buffer[255];
	int8_t ret = -1;
	size_t numc;
	char *err;

	if (pipe(fds))
		return -1;

	if ((xlinfo = fork())) {
		close(fds[1]);

		if (probe_keyval(buffer, sizeof (buffer), fds[0], "nr_cpus")) {
			numc = strtol(buffer, &err, 10);
			if (!*err)
				ret = 0;
		}

		close(fds[0]);
		wait(NULL);
	} else {
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);
		close(STDERR_FILENO);
		execlp("xl", "xl", "info", NULL);
		exit(EXIT_FAILURE);
	}

	if (ret == -1)
		return ret;
	if (numcore)
		*numcore = numc;
	if (maxid)
		*maxid = numc - 1;
	return 0;
}


static int hypercall_perform(unsigned long cmd, uint64_t *arr)
{
	int ret;
	xc_interface *xch = xc_interface_open(0, 0, 0);

	DECLARE_HYPERCALL;

	if (xch == NULL)
		goto err;

	hypercall.op = __HYPERVISOR_xen_version;
	hypercall.arg[0] = cmd;
	hypercall.arg[1] = (unsigned long) arr;

	ret = do_xen_hypercall(xch, &hypercall);
	xc_interface_close(xch);

	if (ret != 0)
		goto err;
 out:
	return ret;
 err:
	ret = -1;
	goto out;
}

static void hypercall_wrmsr(uint64_t addr, uint64_t value,
			    uint64_t *cores, size_t len)
{
	uint64_t *arr = alloca((3 + len) * sizeof (uint64_t));
	
	arr[0] = addr;
	arr[1] = value;
	arr[2] = len;
	
	memcpy(arr + 3, cores, len * sizeof (uint64_t));
	hypercall_perform(HYPERCALL_BIGOS_WRMSR, arr);
	memcpy(cores, arr + 3, len * sizeof (uint64_t));
}

static void hypercall_rdmsr(uint64_t addr, uint64_t *cores, size_t len)
{
	unsigned long *arr = alloca((2 + len) * sizeof (uint64_t));
	
	arr[0] = addr;
	arr[1] = len;

	memcpy(arr + 2, cores, len * sizeof (uint64_t));
	hypercall_perform(HYPERCALL_BIGOS_RDMSR, arr);
	memcpy(cores, arr + 2, len * sizeof (uint64_t));
}


size_t rdmsr_arr(msrval_t *vals, const msradr_t *addrs, const uint8_t *cores,
		 size_t len)
{
	size_t i, j, done = 0;
	size_t start, num = 0;

	for (i=0; i<len; i++) {
		if (num == 0) {
			num = 1;
			start = i;
		} else if (addrs[i] == addrs[start]) {
			num++;
		} else {
			for (j=start; j<i; j++)
				vals[j] = (uint64_t) cores[j];
			hypercall_rdmsr(addrs[start], vals + start, num);
			done += num;
			
			num = 1;
			start = i;
		}
	}

	if (num) {
		for (j=start; j<i; j++)
			vals[j] = (uint64_t) cores[j];
		hypercall_rdmsr(addrs[start], vals + start, num);
		done += num;
	}
	
	return done;
}
	
size_t wrmsr_arr(const msradr_t *addrs, const msrval_t *vals,
		 const uint8_t *cores, size_t len)
{
	size_t i, done = 0;

	for (i=0; i<len; i++) {
		hypercall_wrmsr(addrs[i], vals[i], (uint64_t *) &cores[i], 1);
		done++;
	}

	return done;
}

size_t rwmsr_arr(const msradr_t *addrs, msrval_t *vals, const uint8_t *cores,
		 size_t len)
{
	size_t i, j, done = 0;
	size_t start, num = 0;
	uint64_t val;

	for (i=0; i<len; i++) {
		if (num == 0) {
			num = 1;
			start = i;
		} else if (addrs[i] == addrs[start]) {
			num++;
		} else {
			val = vals[start];
			for (j=start; j<i; j++)
				vals[j] = (uint64_t) cores[j];
			hypercall_wrmsr(addrs[start], val, vals + start, num);
			done += num;
			
			num = 1;
			start = i;
		}
	}

	if (num) {
		val = vals[start];
		for (j=start; j<i; j++)
			vals[j] = (uint64_t) cores[j];
		hypercall_wrmsr(addrs[start], val, vals + start, num);
		done += num;
	}
	
	return done;
}
