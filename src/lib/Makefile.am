libpepper_efl_la_LTLIBRARIES = libpepper-efl.la
libpepper_efl_ladir = $(libdir)
libpepper_efl_la_LIBADD = @EFL_LIBS@ @WAYLAND_LIBS@ @PEPPER_LIBS@
libpepper_efl_la_LDFLAGS = ${LDFLAGS}
libpepper_efl_la_CFLAGS = \
    ${CFLAGS} \
    @EFL_CFLAGS@ \
    @WAYLAND_CFLAGS@ \
    @PEPPER_CFLAGS@ \
    -I$(top_srcdir)/src

libpepper_efl_la_SOURCES = \
compositor.c		\
log.c			\
output.c		\
surface.c		\
shell.c			\
input.c			\
object.c

libeomincludedir = $(includedir)/pepper-efl
libeominclude_HEADERS = \
Pepper_Efl.h
