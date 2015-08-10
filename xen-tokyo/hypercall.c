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

#include <alloca.h>
#include <stdint.h>
#include <string.h>
#include <xenctrl.h>
#include <xenguest.h>
#include <xc_private.h>
#include "xen-msr.h"


#define HYPERCALL_BIGOS_RDMSR    -2
#define HYPERCALL_BIGOS_WRMSR    -3


static int __hypercall_perform(unsigned long cmd, unsigned long *arr)
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

void hypercall_wrmsr(unsigned long addr, unsigned long value,
		     unsigned long size, const unsigned long *cores,
		     unsigned long *values)
{
	unsigned long *arr = alloca((3 + size) * sizeof(unsigned long));
	arr[0] = addr;
	arr[1] = value;
	arr[2] = size;
	memcpy(arr + 3, cores, size * sizeof(unsigned long));
	__hypercall_perform(HYPERCALL_BIGOS_WRMSR, arr);
	memcpy(values, arr + 3, size * sizeof(unsigned long));
}

void hypercall_rdmsr(unsigned long addr, unsigned long size,
		     const unsigned long *cores, unsigned long *values)
{
	unsigned long *arr = alloca((2 + size) * sizeof(unsigned long));
	arr[0] = addr;
	arr[1] = size;
	memcpy(arr + 2, cores, size * sizeof(unsigned long));
	__hypercall_perform(HYPERCALL_BIGOS_RDMSR, arr);
	memcpy(values, arr + 2, size * sizeof(unsigned long));
}
