bin_PROGRAMS = pepper_efl_example

pepper_efl_example_SOURCES = pepper_efl_example.c

pepper_efl_example_CFLAGS = 				\
	-I$(top_builddir)/src/lib/			\
	@ELEMENTARY_CFLAGS@
pepper_efl_example_LDADD = 				\
	$(top_builddir)/src/lib/libpepper-efl.la	\
	@ELEMENTARY_LIBS@
