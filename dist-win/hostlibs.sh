#!/bin/bash

# Copies some mingw dlls

scriptdir=$(dirname $0)
cd "${scriptdir}"
. common.sh

libs=("gomp-1" "gcc*")

distro=unknown

distro=$(grep '^ID=' /etc/os-release | cut -f2 -d= || echo UNKNOWN)

case $distro in
	ubuntu)
		base=/usr/lib/gcc/${host}
		dir=$(find ${base}/* -maxdepth 0 | grep -v 'posix$' | sort -n | tail -n 1)
		hostdir=${dir}
		;;
	arch)
		hostdir=/usr/${host}/bin
		;;
	*)
		echo "No idea how to build windows package on distro \"$distro\""
		exit 1
		;;
esac

bindir=${prefix}/bin
mkdir -p ${bindir} 2>/dev/null

for lib in ${libs[@]}; do
	echo cp ${hostdir}/lib${lib}.dll ${bindir}/
	cp ${hostdir}/lib${lib}.dll ${bindir}/ || exit 1
done
