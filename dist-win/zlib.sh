#!/bin/bash

# Builds zlib for transparent (de)compression of input/output files

zlib_version=1.2.8

scriptdir=$(dirname "$0")
. "${scriptdir}/common.sh"

test -e zlib-${zlib_version}.tar.gz || wget http://zlib.net/zlib-${zlib_version}.tar.gz

builddir=zlib-build-${arch}
rm -rf ${builddir}

tar xzf zlib-${zlib_version}.tar.gz || exit 1
cp -r zlib-${zlib_version} ${builddir}
cd ${builddir}
sed -e s/"PREFIX ="/"PREFIX = ${host}-"/ -i win32/Makefile.gcc
make -f win32/Makefile.gcc &&
	BINARY_PATH=${prefix}/bin \
	INCLUDE_PATH=${prefix}/include \
	LIBRARY_PATH=${prefix}/lib \
	make -f win32/Makefile.gcc SHARED_MODE=1 install || exit 1
cd ..
