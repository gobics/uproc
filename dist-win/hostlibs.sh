#!/bin/bash

# Copies some mingw dlls

scriptdir=$(dirname $0)
cd "${scriptdir}"
. common.sh

libs=("gomp-1" "gcc*")

distro=unknown

grep -s -q 'ID=ubuntu' /etc/os-release && distro=ubuntu
# TODO: test for archlinux (and maybe others)

case $distro in
	ubuntu)
		base=/usr/lib/gcc/${host}
		dir=$(find ${base}/* -maxdepth 0 | grep -v 'posix$' | sort -n | tail -n 1)
		hostdir=${dir}
		;;
	archlinux)
		echo implement me >&2
		exit 1
		;;
esac

bindir=${prefix}/bin
mkdir -p ${bindir} 2>/dev/null

for lib in ${libs[@]}; do
	echo cp ${hostdir}/lib${lib}.dll ${bindir}/
	cp ${hostdir}/lib${lib}.dll ${bindir}/ || exit 1
done
