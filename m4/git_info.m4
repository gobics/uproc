m4_define([m4_chop], [m4_substr($1, 0, m4_decr(m4_len($1)))])
m4_define([m4_esyscmd_chop], [m4_chop(m4_esyscmd($1))])

AC_DEFUN([GIT_INFO],[

    AC_DEFINE(GIT_TAG,
              ["m4_esyscmd_chop([git describe --abbrev=0])"],
              [latest git tag])

    AC_DEFINE(GIT_COMMIT,
              ["m4_esyscmd_chop([git rev-list --max-count=1 --abbrev-commit HEAD])"],
              [sha1 of latest git commit])

    AC_DEFINE(GIT_BRANCH,
              ["m4_esyscmd_chop([git rev-parse --abbrev-ref HEAD])"],
              [active git branch])
])
