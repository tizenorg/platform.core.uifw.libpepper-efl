##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_define([v_maj], [0])
m4_define([v_min], [1])
m4_define([v_mic], [99])
#m4_define([v_rev], m4_esyscmd([(git rev-list --count HEAD 2>/dev/null || echo 0) | tr -d '\n']))dnl
##--   When released, remove the dnl on the below line
dnl m4_undefine([v_rev])
##--   When doing snapshots - change soname. remove dnl on below line
dnl m4_define([relname], [ver-pre-svn-07])
dnl m4_define([v_rel], [-release relname])
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], [m4_define([v_ver], [v_maj.v_min.v_mic.v_rev])], [m4_define([v_ver], [v_maj.v_min.v_mic])])
m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##

AC_INIT([libpepper-efl], [v_ver], [shiin.lee@samsung.com])
AC_PREREQ([2.64])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE(1.6 dist-bzip2 dist-xz subdir-objects)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS([config.h])
AC_GNU_SOURCE
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AM_PROG_AS

##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
m4_ifdef([v_rev], , [m4_define([v_rev], [0])])
m4_ifdef([v_rel], , [m4_define([v_rel], [])])
AC_DEFINE_UNQUOTED(VMAJ, [v_maj], [Major version])
AC_DEFINE_UNQUOTED(VMIN, [v_min], [Minor version])
AC_DEFINE_UNQUOTED(VMIC, [v_mic], [Micro version])
AC_DEFINE_UNQUOTED(VREV, [v_rev], [Revison])

version_info="lt_cur:lt_rev:lt_age"
release_info="v_rel"
AC_SUBST(version_info)
AC_SUBST(release_info)
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##--##
PEPPER_EFL_VERSION_MAJOR=v_maj
PEPPER_EFL_VERSION_MINOR=v_min
AC_SUBST(PEPPER_EFL_VERSION_MAJOR)
AC_SUBST(PEPPER_EFL_VERSION_MINOR)

EFL_ENABLE_EO_API_SUPPORT
EFL_ENABLE_BETA_API_SUPPORT

# Checks for libraries.
efl_version="1.16.0"
requirement_libpepperefl=" \
eo >= ${efl_version} \
eina >= ${efl_version} \
ecore >= ${efl_version} \
ecore-input >= ${efl_version} \
ecore-wayland >= ${efl_version} \
evas >= ${efl_version} \
pepper \
xkbcommon \
wayland-server \
xdg-shell-server \
tizen-extension-client \
wayland-tbm-server"

AC_SUBST(requirement_libpepperefl)


PKG_CHECK_MODULES([LIBPEPPER_EFL], [${requirement_libpepperefl}])

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

# Needs to be moved into a macro, and also, needs a way to automatically fetch from all the dependencies using the Requires.
DEPS_EOLIAN_FLAGS=`${PKG_CONFIG} --variable=eolian_flags eo evas efl elementary`
AC_SUBST([DEPS_EOLIAN_FLAGS])

EFL_WITH_BIN([eolian], [eolian-gen], [eolian_gen])

# Force the helper to try external eolian generator
AM_CONDITIONAL([HAVE_EOLIAN_GEN], [true])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/lib/Makefile
src/examples/Makefile
])
AC_OUTPUT([
libpepper-efl.pc
])
