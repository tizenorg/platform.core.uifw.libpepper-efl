AM_CPPFLAGS =              \
         -I./extensions

libpepper_efl_la_LTLIBRARIES = libpepper-efl.la
libpepper_efl_ladir = $(libdir)
libpepper_efl_la_LIBADD = @EFL_LIBS@ @WAYLAND_LIBS@ @PEPPER_LIBS@ @WAYLAND_TBM_LIBS@
libpepper_efl_la_LDFLAGS = ${LDFLAGS}
libpepper_efl_la_CPPFLAGS = $(AM_CPPFLAGS)
libpepper_efl_la_CFLAGS = \
    ${CFLAGS} \
    @EFL_CFLAGS@ \
    @WAYLAND_CFLAGS@ \
    @PEPPER_CFLAGS@ \
    @WAYLAND_TBM_CFLAGS@ \
    -I$(top_srcdir)/src

EXT_SOURCES =              \
extensions/tizen_policy.h  \
extensions/tizen_policy.c

libpepper_efl_la_SOURCES = \
compositor.c	           \
log.c			   \
output.c		   \
shell.c			   \
input.c			   \
object.c                   \
$(EXT_SOURCES)

libpeppereflincludedir = $(includedir)/pepper-efl
libpeppereflinclude_HEADERS = \
Pepper_Efl.h \
extensions/tizen_policy.h
