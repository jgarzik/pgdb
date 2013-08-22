
dnl Process this file with autoconf to produce a configure script.
AC_INIT(pgdb, 0.1git, [http://sourceforge.net/projects/XXXXXXXXX0 ..])
AC_PREREQ(2.52)
AC_CONFIG_SRCDIR([lib/open.c])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([gnu])
AC_CONFIG_HEADERS([pgdb-config.h])

AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PROG_RANLIB
AC_ISC_POSIX

dnl Checks for header files.
AC_HEADER_STDC
AC_C_BIGENDIAN

dnl Checks for typedefs, structures, and compiler characteristics.

AC_CHECK_LIB(crypto, SHA1_Update, CRYPTO_LIBS=-lcrypto, exit 1)
AC_CHECK_LIB(uuid, uuid_generate_random, UUID_LIBS=-luuid, exit 1)

AC_SUBST(CRYPTO_LIBS)
AC_SUBST(UUID_LIBS)

AC_CONFIG_FILES([Makefile
		lib/Makefile])
AC_OUTPUT

