AUTOMAKE_OPTIONS = 1.4 foreign
MAINTAINERCLEANFILES = Makefile.in

CLEANFILES = 

EOLIAN_FLAGS =             \
   @DEPS_EOLIAN_FLAGS@     \
   -I$(top_srcdir)/src/lib

include $(top_srcdir)/Makefile_Eolian_Helper.am

AM_CPPFLAGS =     \
   -I./extensions

libpepper_efl_la_LTLIBRARIES = libpepper-efl.la
libpepper_efl_ladir = $(libdir)
libpepper_efl_la_LIBADD = @LIBPEPPER_EFL_LIBS@
libpepper_efl_la_LDFLAGS = ${LDFLAGS}
libpepper_efl_la_CPPFLAGS = $(AM_CPPFLAGS)
libpepper_efl_la_CFLAGS =  \
   ${CFLAGS}               \
   @LIBPEPPER_EFL_CFLAGS@  \
   -I$(top_srcdir)/src

EXT_SOURCES =              \
extensions/tizen_policy.h  \
extensions/tizen_policy.c

libpepper_efl_la_SOURCES = \
   compositor.c	           \
   log.c		   \
   output.c		   \
   shell.c		   \
   input.c		   \
   pepper_object.c         \
   $(EXT_SOURCES)

libpeppereflincludedir = $(includedir)/pepper-efl
libpeppereflinclude_HEADERS = \
   Pepper_Efl.h               \
   pepper_object.eo.h         \
   pepper_object.eo.legacy.h  \
   extensions/tizen_policy.h

BUILT_SOURCES =                        \
   pepper_efl_comp.eo.c          \
   pepper_efl_comp.eo.h          \
   pepper_efl_comp.eo.legacy.h   \
   pepper_object.eo.c                  \
   pepper_object.eo.h                  \
   pepper_object.eo.legacy.h

CLEANFILES += $(BUILT_SOURCES)
