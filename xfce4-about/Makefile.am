
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"xfce4-about\" \
	-DDATADIR=\"$(datadir)/xfce4\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

bin_PROGRAMS = \
	xfce4-about

xfce4_about_SOURCES = \
	$(top_srcdir)/libxfce4ui/libxfce4ui-resources.c \
	contributors.h \
	main.c

if HAVE_GLIBTOP

xfce4_about_SOURCES += \
	system-info.c \
	system-info.h

endif

xfce4_about_CFLAGS = \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(GTK3_CFLAGS) \
	$(GLIBTOP_CFLAGS) \
	$(EPOXY_CFLAGS) \
	$(GUDEV_CFLAGS) \
	$(PLATFORM_CFLAGS)

xfce4_about_LDADD = \
	$(top_builddir)/libxfce4ui/libxfce4ui-2.la \
	$(LIBXFCE4UTIL_LIBS) \
	$(GLIBTOP_LIBS) \
	$(EPOXY_LIBS) \
	$(GUDEV_LIBS) \
	$(GTK3_LIBS)

xfce4_about_DEPENDENCIES = \
	$(top_builddir)/libxfce4ui/libxfce4ui-2.la

desktopdir = $(datadir)/applications
desktop_in_files = xfce4-about.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

xfce4-about.desktop: xfce4-about.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

DISTCLEANFILES = \
	$(desktop_DATA)

EXTRA_DIST = \
	$(desktop_in_files) \
	meson.build \
	$(NULL)
