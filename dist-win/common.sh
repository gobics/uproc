#!/bin/bash

if [[ -z "$1" ]]; then
	echo "usage: $0 i686|x86_64 [DESTDIR]"
	exit 1
fi
arch=$1

if [[ -z "$2" ]]; then
	destdir=${PWD}
else
	destdir=$2
fi
destdir=$(realpath "${destdir}")

mkdir ${destdir} 2>/dev/null

host=${arch}-w64-mingw32
prefix=${destdir}/${arch}
