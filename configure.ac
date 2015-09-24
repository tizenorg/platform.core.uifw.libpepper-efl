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
PKG_CHECK_MODULES([EFL], [elementary])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT32_T

# Checks for library functions.

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT([
src/libpepper-efl.pc
])
