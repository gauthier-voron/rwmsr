/*
 * Copyright 2015 Gauthier Voron
 * This file is part of rwmsr.
 *
 * Rwmsr is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Rwmsr is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rwmsr. If not, see <http://www.gnu.org/licenses/>.
 */


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
