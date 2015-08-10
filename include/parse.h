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


#ifndef PARSE_H
#define PARSE_H


#include <stdint.h>
#include <stdlib.h>

#include "engine.h"
#include "rwmsr.h"


/*
 * Parse a string indacting a set of cores and fill the dest array with.
 * The string is in the form: "n", "n,m", "n-m" or any combination of these.
 * If a core number is greater or equal to the array length, or if something
 * wrong happens during the parsing, return the address of the first wrong
 * character. Return NULL in case of success.
 */
const char *parse_cores(uint8_t *dest, size_t len, const char *str);


/*
 * Parse a string indicating a rwmsr command and fill the dest structure with.
 * The string is in the form: "[:]<address>[=<value>][@<delay>[-<repeat>]].
 * The leading ":" character indicate to print the value of the register,
 * before the write if any.
 * The <address> is the msr hardware address, the <value> is the number to
 * write in the register. Any of those can be in the decimal form, or in the
 * hexadecimal form (when starting with 'x' or '0x').
 * The <delay> is the amount of millisecond to wait before to execute the
 * command and the <repeat> is the amount of millisecond to wait before to
 * repeat the command (until the program is killed).
 * In case of success, return NULL, otherwise, return the address of the first
 * wrong character.
 */
const char *parse_command(struct command *dest, const char *str);



#endif
