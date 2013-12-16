dnl from https://raw.github.com/kakaroto/e17/master/PROTO/autotools/ac_attribute.m4

dnl Copyright (C) 2004-2008 Kim Woelders
dnl Copyright (C) 2008 Vincent Torri <vtorri at univ-evry dot fr>
dnl That code is public domain and can be freely used or copied.
dnl Originally snatched from somewhere...


dnl Macro for checking if the compiler supports __attribute__

dnl Usage: AC_C___ATTRIBUTE__
dnl call AC_DEFINE for HAVE___ATTRIBUTE__
dnl iff the compiler supports __attribute__, HAVE___ATTRIBUTE__ is
dnl defined to 1

AC_DEFUN([AC_C___ATTRIBUTE__],
[

AC_MSG_CHECKING([for __attribute__])

AC_CACHE_VAL([ac_cv___attribute__],
   [AC_TRY_COMPILE(
       [
#include <stdlib.h>

struct foo {
    int x;
    char y;
    double z[10];
} __attribute__((packed));
       ],
       [],
       [ac_cv___attribute__="yes"],
       [ac_cv___attribute__="no"]
    )]
)

AC_MSG_RESULT($ac_cv___attribute__)

if test "x${ac_cv___attribute__}" = "xyes" ; then
   AC_DEFINE([HAVE___ATTRIBUTE__], [1], [Define to 1 if your compiler has __attribute__])
fi

])

