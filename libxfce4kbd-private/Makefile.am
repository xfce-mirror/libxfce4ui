# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:

if ENABLE_KEYBOARD_LIBRARY

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"libxfce4kbd-private\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DPREFIX=\"$(prefix)\" \
	$(PLATFORM_CPPFLAGS)

libxfce4kbd_headers = \
	xfce-shortcuts-provider.h \
	xfce-shortcut-dialog.h \
	xfce-shortcuts-editor.h \
	xfce-shortcuts-editor-dialog.h \
	xfce-shortcuts.h \
	xfce-shortcuts-xfwm4.h

libxfce4kbd_built_sources = \
	xfce-shortcuts-marshal.c \
	xfce-shortcuts-marshal.h

libxfce4kbd_sources = \
	$(libxfce4kbd_headers) \
	$(libxfce4kbd_built_sources) \
	xfce-shortcuts-provider.c \
	xfce-shortcut-dialog.c \
	xfce-shortcuts-editor.c \
	xfce-shortcuts-editor-dialog.c \
	xfce-shortcuts.c \
	xfce-shortcuts-xfwm4.c

libxfce4kbd_private_3_la_CFLAGS = \
	$(PLATFORM_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK3_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfce4kbd_private_3_la_LDFLAGS = \
	-export-dynamic \
	-version-info $(LIBXFCE4KBD_PRIVATE_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libxfce4kbd_private_3_la_LIBADD = \
	$(top_builddir)/libxfce4ui/libxfce4ui-2.la \
	$(GLIB_LIBS) \
	$(GTK3_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(XFCONF_LIBS)

if ENABLE_X11
libxfce4kbd_headers += \
	xfce-shortcuts-grabber.h

libxfce4kbd_sources += \
	xfce-shortcuts-grabber.h \
	xfce-shortcuts-grabber.c

libxfce4kbd_private_3_la_CFLAGS += \
	$(LIBX11_CFLAGS)

libxfce4kbd_private_3_la_LIBADD += \
	$(LIBX11_LIBS)
endif

lib_LTLIBRARIES = libxfce4kbd-private-3.la

libxfce4kbd3_private_includedir = \
	$(includedir)/xfce4/libxfce4kbd-private-3/libxfce4kbd-private

libxfce4kbd3_private_include_HEADERS = \
	$(libxfce4kbd_headers)

libxfce4kbd_private_3_la_SOURCES = \
	$(libxfce4kbd_sources)

settingsdir = $(sysconfdir)/xdg/xfce4/xfconf/xfce-perchannel-xml
settings_DATA = xfce4-keyboard-shortcuts.xml

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfce4kbd-private-3.pc

if ENABLE_X11
symbols_sed_script = /@BEGIN_X11_SYMBOLS@/d; /@END_X11_SYMBOLS@/d;
else
symbols_sed_script = /@BEGIN_X11_SYMBOLS@/,/@END_X11_SYMBOLS@/d;
endif
if ENABLE_WAYLAND
symbols_sed_script += /@BEGIN_WAYLAND_SYMBOLS@/d; /@END_WAYLAND_SYMBOLS@/d
else
symbols_sed_script += /@BEGIN_WAYLAND_SYMBOLS@/,/@END_WAYLAND_SYMBOLS@/d
endif

libxfce4kbd-private.symbols: libxfce4kbd-private.symbols.in
	$(AM_V_GEN) $(SED) "$(symbols_sed_script)" < $< > $@

libxfce4kbd-private-visibility.h: libxfce4kbd-private.symbols Makefile
	$(AM_V_GEN) $(XDT_GEN_VISIBILITY) --kind=header $< $@

libxfce4kbd-private-visibility.c: libxfce4kbd-private.symbols Makefile
	$(AM_V_GEN) $(XDT_GEN_VISIBILITY) --kind=source $< $@

DISTCLEANFILES = \
	libxfce4kbd-private-visibility.c \
	libxfce4kbd-private-visibility.h \
	libxfce4kbd-private.symbols \
	$(NULL)

BUILT_SOURCES = \
	libxfce4kbd-private-visibility.c \
	libxfce4kbd-private-visibility.h \
	libxfce4kbd-private.symbols \
	$(NULL)


if MAINTAINER_MODE
xfce-shortcuts-marshal.h: xfce-shortcuts-marshal.list Makefile
	$(AM_V_GEN) glib-genmarshal --prefix=_xfce_shortcuts_marshal --internal --header $< > $@

xfce-shortcuts-marshal.c: xfce-shortcuts-marshal.list Makefile xfce-shortcuts-marshal.h
	$(AM_V_GEN) echo "#include \"xfce-shortcuts-marshal.h\"" > $@ \
	&& glib-genmarshal --prefix=_xfce_shortcuts_marshal --body $< >> $@

DISTCLEANFILES += \
	$(libxfce4kbd_built_sources)

BUILT_SOURCES += \
	$(libxfce4kbd_built_sources)
endif

EXTRA_DIST = \
	$(settings_DATA) \
	libxfce4kbd-private.symbols.in \
	xfce-shortcuts-marshal.list \
	meson.build \
	$(NULL)

# required for gtk-doc
dist-hook: all

endif # ENABLE_KEYBOARD_LIBRARY
