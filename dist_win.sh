#!/bin/bash

export MAKEFLAGS=''

srcdir=${PWD}
version=$(${srcdir}/configure --version | head -n 1 | cut -d ' ' -f 3)

function build {
	arch=$1
	host=${arch}-w64-mingw32
	zipname=uproc-${version}-win-${arch}
	destdir=${PWD}/${zipname}

	mkdir tmp 2>/dev/null
	cd tmp &&
	 ${srcdir}/configure --host ${host} --enable-shared \
		--prefix=${PWD} &&
	make && make install || exit 1

	mkdir ${destdir} 2>/dev/null

	# copy headers
	cp -r ${PWD}/include ${destdir}

	# copy doxygen docs
	cp -r ${PWD}/share/doc/uproc/libuproc ${destdir}
	mv ${destdir}/libuproc ${destdir}/libuproc-docs

	# copy README and generate README.html
	unix2dos -n ${PWD}/share/doc/uproc/README ${destdir}/README.txt
	rst2html ${PWD}/share/doc/uproc/README > ${destdir}/README.html

	# copy .exe and .dll files
	cp ${PWD}/bin/* ${destdir}/

	cp /usr/${host}/bin/{libgomp-1,libwinpthread-1,zlib1}.dll ${destdir} || exit 1
	cp /usr/${host}/bin/libgcc*.dll ${destdir} || exit 1

	cd ..
	zip -r ${zipname}.zip ${zipname}
	gpg -b ${zipname}.zip
	mkdir ${version} 2>/dev/null
	mv ${zipname}.zip{,.sig} ${version}
	rm -rf tmp ${destdir}
}

build i686
build x86_64
