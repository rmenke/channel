# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

AC_COPYRIGHT([Copyright (C) 2026 by Rob Menke.  Please see the
accompanying LICENSE.txt file for details.])

AC_INIT([channel],[m4_esyscmd_s([perl git-semver])],
[AC_PACKAGE_TARNAME-bugs@the-wabe.com],[],
[http://www.the-wabe.com/AC_PACKAGE_TARNAME/])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([-Wall foreign nostdinc subdir-objects])

AX_CXX_COMPILE_STDCXX([20],[noext])

AC_LANG([C++])

AC_CHECK_HEADERS([functional iterator ranges],[],[
    AC_MSG_ERROR([incomplete C++ standard library])
])

AX_CODE_COVERAGE

AC_SUBST([PACKAGE_DESCRIPTION],["Bresenham-Murphy algorithm in C++"])

AC_REQUIRE_AUX_FILE([tap-driver.sh])

AC_CONFIG_FILES([Makefile test/Makefile Doxyfile])

AC_OUTPUT
