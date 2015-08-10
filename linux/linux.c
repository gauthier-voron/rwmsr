#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "main.h"
#include "rwmsr.h"


#define BASE_PATH        "/dev/cpu/"
#define BASE_LENGTH      9

#define CORE_MAXLEN      sizeof (struct dirent)

#define END_PATH         "/msr"
#define END_LENGTH       4

#define TOTAL_MAXLEN    (BASE_LENGTH + CORE_MAXLEN + END_LENGTH)


int8_t init(const char *sysname)
{
	size_t numcore;

	if (strcmp(sysname, "linux"))
		return -1;

	if (coreinfo(&numcore, NULL))
		return -1;
	if (numcore == 0) {
		if (verbose)
			vlog("need kernel module 'msr' to be loaded");
		return -1;
	}
	
	return 0;
}

int8_t destroy(void)
{
	return 0;
}


int8_t coreinfo(size_t *numcore, size_t *maxid)
{
	DIR *fh;
	size_t val, num = 0, max = 0;
	struct dirent *entry;
	char buffer[TOTAL_MAXLEN] = { BASE_PATH };
	char *err;

	fh = opendir(BASE_PATH);
	if (!fh)
		return -1;

	while ((entry = readdir(fh))) {
		val = strtol(entry->d_name, &err, 10);
		if (*err)
			continue;

		strcpy(buffer + BASE_LENGTH, entry->d_name);
		strcpy(buffer + BASE_LENGTH + (err - entry->d_name), END_PATH);
		if (access(buffer, F_OK))
			continue;
		
		if (val > max)
			max = val;
		num++;
	}

	closedir(fh);

	if (numcore)
		*numcore = num;
	if (maxid)
		*maxid = max;
	return 0;
}


static int open_msrfd(msradr_t addr, uint8_t core, int flags)
{
	int fd, ret = -1;
	char buffer[TOTAL_MAXLEN] = BASE_PATH;

	sprintf(buffer + BASE_LENGTH, "%d" END_PATH, core);
	
	fd = open(buffer, flags);
	if (fd < 0)
		return -1;
	
	if (lseek(fd, addr, SEEK_SET) < 0)
		close(fd);
	else
		ret = fd;
	
	return ret;
}

size_t rdmsr_arr(msrval_t *vals, const msradr_t *addrs, const uint8_t *cores,
		 size_t len)
{
	int fd;
	uint64_t val;
	size_t i, done = 0;
	ssize_t ret;

	for (i=0; i<len; i++) {
		fd = open_msrfd(addrs[i], cores[i], O_RDONLY);
		if (fd < 0)
			continue;

		ret = read(fd, &val, sizeof (uint64_t));
		if (ret == sizeof (uint64_t)) {
			vals[i] = (msrval_t) val;
			done++;
		}
		
		close(fd);
	}
	
	return done;
}
	
size_t wrmsr_arr(const msradr_t *addrs, const msrval_t *vals,
		 const uint8_t *cores, size_t len)
{
	int fd;
	size_t i, done = 0;
	ssize_t ret;

	for (i=0; i<len; i++) {
		fd = open_msrfd(addrs[i], cores[i], O_WRONLY);
		if (fd < 0)
			continue;

		ret = write(fd, (uint64_t *) &vals[i], sizeof (uint64_t));
		if (ret == sizeof (uint64_t))
			done++;
		
		close(fd);
	}
	
	return done;
}

size_t rwmsr_arr(const msradr_t *addrs, msrval_t *vals, const uint8_t *cores,
		 size_t len)
{
	int fd;
	uint64_t val;
	size_t i, done = 0;
	ssize_t ret;

	for (i=0; i<len; i++) {
		fd = open_msrfd(addrs[i], cores[i], O_RDWR);
		if (fd < 0)
			continue;

		ret = read(fd, &val, sizeof (uint64_t));
		if (ret != sizeof (uint64_t))
			goto end;
		
		ret = write(fd, (uint64_t *) &vals[i], sizeof (uint64_t));
		if (ret != sizeof (uint64_t))
			goto end;
		
		vals[i] = (msrval_t) val;
		done++;

	end:
		close(fd);
	}
	
	return done;
}
