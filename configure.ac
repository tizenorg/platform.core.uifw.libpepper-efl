#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([libpepper-efl], [0.0.1], [shiin.lee@samsung.com])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE(1.6)

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES([PEPPER], [pepper])
PKG_CHECK_MODULES([WAYLAND], [wayland-server xdg-shell-server])
PKG_CHECK_MODULES([EFL], [eina ecore evas])


AC_ARG_ENABLE([examples],
   [AS_HELP_STRING([--enable-examples], [build examples, @<:@default=disabled@:>@])],
   [
    if test "x${enableval}" = "xyes"; then
       PKG_CHECK_MODULES([ELEMENTARY], [elementary])
       want_build_examples="yes"
    else
       want_build_examples="no"
    fi
   ],
   [want_build_examples="no"])
AM_CONDITIONAL([BUILD_EXAMPLES], [test "${want_build_examples}" = "yes"])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/lib/Makefile
src/examples/Makefile
])
AC_OUTPUT([
libpepper-efl.pc
])
