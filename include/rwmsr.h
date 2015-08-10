#ifndef RWMSR_H
#define RWMSR_H


#include <stdint.h>
#include <stdlib.h>


typedef uint64_t msrval_t;
typedef uint64_t msradr_t;


int8_t init(const char *sysname);

int8_t destroy(void);


int8_t coreinfo(size_t *numcore, size_t *maxid);


size_t rdmsr_arr(msrval_t *vals, const msradr_t *addrs, const uint8_t *cores,
		 size_t len);
	
size_t wrmsr_arr(const msradr_t *addrs, const msrval_t *vals,
		 const uint8_t *cores, size_t len);

size_t rwmsr_arr(const msradr_t *addrs, msrval_t *vals, const uint8_t *cores,
		 size_t len);


#endif
