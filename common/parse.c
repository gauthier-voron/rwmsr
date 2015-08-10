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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "parse.h"


const char *parse_cores(uint8_t *dest, size_t len, const char *str)
{
	size_t i, start = 0, val;
	uint8_t range = 0;
	char *err;

	while (1) {
		if (*str < '0' || *str > '9')
			return str;
		
		val = strtol(str, &err, 10);
		
		if (val >= len)
			return str;
		
		if (range && val >= start) {
			for (i=start; i<=val; i++)
				dest[i] |= 1;
		} else if (range) {
			for (i=start; i>=val && i<=start; i--)
				dest[i] |= 1;
		} else if (*err != '-') {
			dest[val] |= 1;
		}
		
		range = 0;
		
		switch (*err) {
		case ',':
			break;
		case '-':
			range = 1;
			start = val;
			break;
		case '\0':
			return NULL;
		}

		str = err + 1;
	}
}


/*
 * Parse a number either in decimal or hexadecimal form.
 * A number is in hexadecimal form if it starts with 'x' or '0x'.
 * Set the end field at the first character following the number in case of
 * success, or NULL in case of failure.
 */
static uint64_t parse_uint64(const char *str, const char **end)
{
	int base = 10;
	uint64_t val;
	char *err;

	if (*str == 'x') {
		base = 16;
		str++;
	} else if (str[0] == '0' && str[1] == 'x') {
		base = 16;
		str += 2;
	}

	val = strtol(str, &err, base);

	if (end) {
		if (err == str)
			*end = NULL;
		else
			*end = (const char *) err;
	}

	return val;
}

const char *parse_command(struct command *dest, const char *str)
{
	const char *ptr;
	
	memset(dest, 0, sizeof (*dest));

	if (*str == ':') {
		dest->flags |= COMMAND_PRINT;
		str++;
	}
	
	if (*str == ':') {
		dest->flags |= COMMAND_HEXA;
		str++;
	}


	dest->address = parse_uint64(str, &ptr);
	if (!ptr)
		return str;
	str = ptr;

	if (*str == '=') {
		str++;
		dest->flags |= COMMAND_WRITE;
		dest->value = parse_uint64(str, &ptr);
		if (!ptr)
			return str;
		str = ptr;
	}

	if (*str == '@') {
		str++;
		dest->flags |= COMMAND_DELAY;
		dest->delay = strtol(str, (char **) &str, 10);

		if (*str == '-') {
			str++;
			dest->flags |= COMMAND_REPEAT;
			dest->repeat = strtol(str, (char **) &str, 10);
		}
	}

	if (*str != '\0')
		return str;
	return NULL;
}
