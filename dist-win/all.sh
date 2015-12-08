#!/bin/bash

# Runs all the other things, in the correct order

scriptdir=$(dirname "$0")

package () {
	arch=$1
	destdir=${PWD}/prefix-${arch}

	scripts=('hostlibs' 'zlib' 'winpthreads' 'uproc' 'package')

	for script in ${scripts[@]}; do
		${scriptdir}/${script}.sh ${arch} ${destdir} || exit 1
	done
}

package i686
package x86_64
