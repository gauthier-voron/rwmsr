#!/bin/sh

for path in "$@" ; do
    path=${path%/}
    
    while [ "x$path" != "x" ] ; do
	[ ! -e "$path" ] || rmdir "$path" || break

	temp=${path%/*}
	if [ "x$path" != "x$temp" ] ; then
	    path=$temp
	else
	    path=""
	fi
    done
done
