datafilesdir = $(datadir)/libpepper-efl/data
datafiles_DATA = red.png

EXTRA_DIST = $(datafiles_DATA)

widgets_bindir = $(libexecdir)/libpepper-efl/samples
widgets_bin_PROGRAMS = touch_sample thread_sample entry_sample

touch_sample_SOURCES = touch_sample.c
touch_sample_CFLAGS = @ELEMENTARY_CFLAGS@
touch_sample_LDADD = @ELEMENTARY_LIBS@

thread_sample_SOURCES = thread_sample.c
thread_sample_CFLAGS = @ELEMENTARY_CFLAGS@
thread_sample_LDADD = @ELEMENTARY_LIBS@

entry_sample_SOURCES = entry_sample.c
entry_sample_CFLAGS = @ELEMENTARY_CFLAGS@
entry_sample_LDADD = @ELEMENTARY_LIBS@


bin_PROGRAMS = pepper_efl_example pepper_efl_simple

pepper_efl_example_SOURCES = pepper_efl_example.c
pepper_efl_example_CFLAGS = 				\
	-I$(top_builddir)/src/lib/			\
        -DDATA_PATH=\"$(datafilesdir)\"                 \
        -DSAMPLE_PATH=\"$(widgets_bindir)\"             \
        @ELEMENTARY_CFLAGS@
pepper_efl_example_LDADD = 				\
	$(top_builddir)/src/lib/libpepper-efl.la	\
	@ELEMENTARY_LIBS@

pepper_efl_simple_SOURCES = pepper_efl_simple.c
pepper_efl_simple_CFLAGS = 				\
	-I$(top_builddir)/src/lib/			\
        -DDATA_PATH=\"$(datafilesdir)\"                 \
        -DSAMPLE_PATH=\"$(widgets_bindir)\"             \
        @ELEMENTARY_CFLAGS@
pepper_efl_simple_LDADD = 				\
	$(top_builddir)/src/lib/libpepper-efl.la	\
	@ELEMENTARY_LIBS@

