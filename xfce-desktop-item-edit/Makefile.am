
AM_CPPFLAGS = 								\
	-I$(top_builddir)						\
	-I$(top_srcdir)							\
	-DBINDIR=\"$(bindir)\"						\
	-DDATADIR=\"$(datadir)\"					\
	-DG_LOG_DOMAIN=\"xfce-desktop-item-edit\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

bin_PROGRAMS =								\
	xfce-desktop-item-edit

xfce_desktop_item_edit_SOURCES =						\
	main.c						\
	xfce-die-command-entry.c						\
	xfce-die-command-entry.h						\
	xfce-die-command-model.c						\
	xfce-die-command-model.h						\
	xfce-die-desktop-model.c						\
	xfce-die-desktop-model.h						\
	xfce-die-editor.c						\
	xfce-die-editor.h						\
	xfce-die-enum-types.c						\
	xfce-die-enum-types.h						\
	xfce-die-utils.c							\
	xfce-die-utils.h							\
	$(NULL)

xfce_desktop_item_edit_CFLAGS =						\
	$(GTK3_CFLAGS)							\
	$(GTHREAD_CFLAGS)						\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(NULL)

xfce_desktop_item_edit_LDFLAGS =						\
	-no-undefined

xfce_desktop_item_edit_DEPENDENCIES =					\
	$(top_builddir)/libxfce4ui/libxfce4ui-2.la

xfce_desktop_item_edit_LDADD =						\
	$(GTK3_LIBS)							\
	$(GTHREAD_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(GIO_LIBS)							\
	$(top_builddir)/libxfce4ui/libxfce4ui-2.la

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
