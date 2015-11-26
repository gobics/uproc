#!/bin/bash

# Builds the pthreads library for windows so we have multithreading \o/

scriptdir=$(dirname $0)
. "${scriptdir}/common.sh"

srcdir=mingw-w64

if [[ -e ${srcdir} ]]; then
	(cd ${srcdir} && git pull)
else
	git clone git://git.code.sf.net/p/mingw-w64/mingw-w64 ${srcdir}
fi

builddir=winpthreads-build-${arch}
rm -rf ${builddir}

mkdir -p ${builddir}
cd ${builddir}
../${srcdir}/mingw-w64-libraries/winpthreads/configure \
	--prefix=${prefix} \
	--libdir=${prefix}/lib \
	--bindir=${prefix}/bin \
	--host=${host} --enable-shared &&
	make && make install || exit 1
${host}-strip --strip-unneeded ${prefix}/bin/*.dll
cd ..
