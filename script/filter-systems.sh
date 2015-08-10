#!/bin/sh

for system in "$@" ; do
    case "$system" in
	xen-tokyo)
	    [ -r "/usr/include/xenctrl.h" ] \
		|| [ -r "/usr/include/xen/xenctrl.h" ] \
		|| [ -r "/usr/local/include/xenctrl.h" ] \
		|| [ -r "/usr/local/include/xen/xenctrl.h" ] \
		|| continue
	    ;;

	linux)
	    version=`uname -r`
	    modroot="/lib/modules/$version"
	    [ -e "$modroot/kernel/arch/x86/kernel/msr.ko.gz" ] \
		|| [ -e "$modroot/kernel/arch/x86/kernel/msr.ko" ] \
		|| continue
		;;
    esac

    echo "$system"
done
