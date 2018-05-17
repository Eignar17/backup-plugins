# Process this file with autoconf to produce a configure script.

# Prelude.
AC_PREREQ([2.59])
AC_INIT([Money], [0.3], [chalserogers@gmail.com])

# unique source file --- primitive safety check 
AC_CONFIG_SRCDIR([src/switcher-util.c])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([build-aux])

# fairly severe build strictness
# change foreign to gnu or gnits to comply with gnu standards
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.9.6])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
PKG_CHECK_MODULES([COMPIZ], [compiz >= 0.5.0])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC

# Output files
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile])

AC_OUTPUT
