#!/bin/bash

# Run this to reformat ALL the .c and .h files

repo_root=$(git rev-parse --show-toplevel)
cd $repo_root

for f in $(git ls-files '*.[ch]');
do
	clang-format -style file $f > indent_tmp_file || exit 1
	mv indent_tmp_file $f
done
