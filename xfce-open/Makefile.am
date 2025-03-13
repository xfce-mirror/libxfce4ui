
AM_CPPFLAGS = 								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"xfce-open\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

bin_PROGRAMS =								\
	xfce-open

xfce_open_SOURCES =							\
	main.c

xfce_open_CFLAGS =							\
	$(GTK3_CFLAGS)							\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)

xfce_open_LDFLAGS =							\
	-no-undefined

xfce_open_LDADD =							\
	$(GTK3_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(top_builddir)/libxfce4ui/libxfce4ui-2.la

EXTRA_DIST =								\
	$(desktop_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
