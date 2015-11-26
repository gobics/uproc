#!/bin/bash

# Puts all files in a .zip for distribution

scriptdir=$(dirname $0)
. "${scriptdir}/common.sh"

uproc_version=$(${scriptdir}/../configure --version | head -n 1 | cut -d ' ' -f 3)

zipdir="uproc-${uproc_version}-win-${arch}"

rm -rf ${zipdir}
mkdir ${zipdir}

docdir=${prefix}/share/doc/uproc

cp ${prefix}/bin/*.{exe,dll} ${zipdir}
cp -r ${prefix}/include ${zipdir}

cp -r ${docdir}/libuproc ${zipdir}/libuproc-docs

unix2dos -n ${docdir}/README.rst ${zipdir}/README.txt
rst2html ${docdir}/README.rst > ${zipdir}/README.html

zip -r ${zipdir}.zip ${zipdir}
rm -rf ${zipdir}
gpg -b ${zipdir}.zip || true
