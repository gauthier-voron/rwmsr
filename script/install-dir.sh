#!/bin/sh

for path in "$@" ; do
    elm=
    rem=$path

    while [ "x$elm" != "x$path" ] && [ "x$elm" != "x$path/" ] ; do
	elm=$elm${rem%%/*}/
	rem=${path#$elm}
    
	[ -d "$elm" ] || mkdir "$elm" || break
    done
done
