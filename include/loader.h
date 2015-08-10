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

#ifndef LOADER_H
#define LOADER_H


#include <stdint.h>
#include <stdlib.h>

#include "rwmsr.h"


/*
 * Probes what system the process is running on.
 * Currently, the following systems are supported:
 * linux  the standard Linux kernel
 * xen    the xen-tokyo moified Xen hypervisor
 * Return the system name in case of success, NULL otherwise.
 */
const char *probe_system(void);


/*
 * Load the module matching the specified system.
 * Modules are probed inside the given paths.
 * The first matching module is taken.
 * If a module is found and file is not NULL, it is filled with the file name.
 * Return 0 if the module is found, -1 otherwie.
 */
int8_t load_module(const char *system, const char **paths, size_t plen,
		   char *file, size_t flen);

void unload_module(void);


#endif
