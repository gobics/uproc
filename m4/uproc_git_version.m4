# defines UPROC_VERSION:
# if configure is being run from the root of the git repo, containing the current branch and commit
# else simply the version passed to AC_INIT

AC_DEFUN([UPROC_GIT_VERSION],[
	uproc_version=${PACKAGE_VERSION}
	AC_CHECK_PROGS([GIT], [git])
	if test "x$GIT" = "xgit" && git rev-parse 2>/dev/null && test $(git rev-parse --show-toplevel) = "$PWD";
	then
		git_commit=`git rev-list --max-count=1 --abbrev-commit HEAD`
		git_branch=`git rev-parse --abbrev-ref HEAD`
		if test "x$git_branch" != "xmaster"; then
			uproc_version=git-${git_branch}-${git_commit}
		fi
	fi
	AC_DEFINE_UNQUOTED(UPROC_VERSION,["$uproc_version"], [Define to])
])
