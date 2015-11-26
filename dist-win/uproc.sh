#!/bin/bash

# Builds uproc, obviously

scriptdir=$(dirname "$0")
. "${scriptdir}/common.sh"

uproc_version=$(${scriptdir}/../configure --version | head -n 1 | cut -d ' ' -f 3)
if ! [[ -e "${scriptdir}/../uproc-${uproc_version}.tar.gz" ]]; then
	(cd .. &&
		./configure &&
		make distcheck &&
		make dist) || exit 1
fi

tar xzf "${scriptdir}/../uproc-${uproc_version}.tar.gz" || exit 1
builddir=uproc-build-${arch}
rm -rf "${builddir}"
cp -r "uproc-${uproc_version}" "${builddir}"
cd "${builddir}"
./configure --host="${host}" --enable-shared \
	--prefix"=${prefix}" \
	CPPFLAGS="-I${prefix}/include" \
	LDFLAGS="-L${prefix}/lib -L/usr/${host}/lib" \
	&& make && make install || exit 1
cd ..
