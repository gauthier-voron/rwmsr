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


#ifndef ENGINE_H
#define ENGINE_H


#include <stdint.h>
#include <stdlib.h>

#include "rwmsr.h"


#define COMMAND_PRINT   (1 << 0)
#define COMMAND_HEXA    (1 << 1)
#define COMMAND_WRITE   (1 << 2)
#define COMMAND_DELAY   (1 << 3)
#define COMMAND_REPEAT  (1 << 4)


struct command
{
	uint8_t   flags;
	msradr_t  address;
	msrval_t  value;
	uint32_t  delay;
	uint32_t  repeat;
};


void execute(const struct command *commands, size_t mlen, const uint8_t *cores,
	     size_t rlen);


#endif
