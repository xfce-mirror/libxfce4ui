CLEANFILES =
lib_LTLIBRARIES =
pkgconfig_DATA =

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DDATADIR=\"$(datadir)\" \
	-DSN_API_NOT_YET_FROZEN \
	-DLIBXFCE4UI_COMPILATION \
	-DG_LOG_DOMAIN=\"libxfce4ui\" \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DLIBXFCE4UI_VERSION_API=\"$(LIBXFCE4UI_VERSION_API)\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	$(PLATFORM_CPPFLAGS)

libxfce4ui_enum_headers = \
	libxfce4ui-enums.h

libxfce4ui_headers = \
	libxfce4ui-config.h \
	libxfce4ui-enum-types.h \
	xfce-cell-renderer-icon.h \
	xfce-dialogs.h \
	xfce-execute.h \
	xfce-gdk-extensions.h \
	xfce-gdk-pixbuf-extensions.h \
	xfce-gtk-extensions.h \
	xfce-icon-chooser-dialog.h \
	xfce-icon-view.h \
	xfce-notebook.h \
	xfce-screensaver.h \
	xfce-spawn.h \
	xfce-filename-input.h \
	xfce-titled-dialog.h \
	xfce-tree-view.h \
	$(libxfce4ui_enum_headers)

libxfce4ui_built_sources = \
	libxfce4ui-enum-types.c \
	libxfce4ui-enum-types.h \
	libxfce4ui-marshal.c \
	libxfce4ui-marshal.h \
	libxfce4ui-resources.c

libxfce4ui_private_sources = \
	libxfce4ui-private.h \
	xfce-icon-chooser-model.c \
	xfce-icon-chooser-model.h \
	xfce-thumbnail-preview.c \
	xfce-thumbnail-preview.h \
	xfce-thumbnail.c \
	xfce-thumbnail.h \
	$(NULL)

libxfce4ui_sources = \
	$(libxfce4ui_headers) \
	$(libxfce4ui_built_sources) \
	$(libxfce4ui_private_sources) \
	libxfce4ui-config.c \
	xfce-cell-renderer-icon.c \
	xfce-dialogs.c \
	xfce-execute.c \
	xfce-gdk-extensions.c \
	xfce-gdk-pixbuf-extensions.c \
	xfce-gtk-extensions.c \
	xfce-icon-chooser-dialog.c \
	xfce-icon-view.c \
	xfce-notebook.c \
	xfce-screensaver.c \
	xfce-spawn.c \
	xfce-filename-input.c \
	xfce-titled-dialog.c \
	xfce-tree-view.c \
	$(NULL)

libxfce4ui_2_la_CFLAGS = \
	$(GIO_UNIX_CFLAGS) \
	$(GDK_CFLAGS) \
	$(GTK3_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfce4ui_2_la_LDFLAGS = \
	-version-info $(LIBXFCE4UI_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-no-undefined \
	$(PLATFORM_LDFLAGS)

libxfce4ui_2_la_LIBADD = \
	$(GIO_UNIX_LIBS) \
	$(GDK_LIBS) \
	$(GTK3_LIBS) \
	$(GDK_PIXBUF_LIBS) \
	$(XFCONF_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	-lm

if ENABLE_X11
libxfce4ui_headers += \
	xfce-clipboard-manager.h \
	xfce-sm-client.h

libxfce4ui_sources += \
	xfce-clipboard-manager.c \
	xfce-clipboard-manager.h \
	xfce-sm-client.c \
	xfce-sm-client.h

libxfce4ui_2_la_CFLAGS += \
	$(LIBICE_CFLAGS) \
	$(LIBSM_CFLAGS) \
	$(LIBX11_CFLAGS) \
	$(LIBSTARTUP_NOTIFICATION_CFLAGS)

libxfce4ui_2_la_LIBADD += \
	$(LIBICE_LIBS) \
	$(LIBSM_LIBS) \
	$(LIBX11_LIBS) \
	$(LIBSTARTUP_NOTIFICATION_LIBS)
endif

libxfce4ui2_includedir = \
	$(includedir)/xfce4/libxfce4ui-2/libxfce4ui

libxfce4ui2_include_HEADERS = \
	$(libxfce4ui_headers)

nodist_libxfce4ui2_include_HEADERS = \
	libxfce4ui.h

lib_LTLIBRARIES += \
	libxfce4ui-2.la

libxfce4ui_2_la_SOURCES = \
	$(libxfce4ui_sources)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA += \
	libxfce4ui-2.pc


LIBXFCE4UI_RESOURCE_FILES = \
	resources/libxfce4ui-dialog-ui.ui \
	resources/xfce4-about-dialog.glade \
	$(NULL)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ENV = CC="$(CC)"
INTROSPECTION_SCANNER_ARGS = \
	--add-include-path=$(srcdir) \
	--add-include-path=$(prefix)/share/gir-1.0 \
	--warn-all \
	--identifier-prefix=Xfce \
	--identifier-prefix=xfce \
	--c-include=libxfce4ui/libxfce4ui.h
INTROSPECTION_COMPILER_ARGS = \
	--includedir=$(srcdir) \
	--includedir=$(prefix)/share/gir-1.0 \
	--includedir=.

if HAVE_INTROSPECTION
introspection_sources = $(filter-out $(libxfce4ui_private_sources) libxfce4ui-config.h, $(libxfce4ui_2_la_SOURCES))

Libxfce4ui-2.0.gir: libxfce4ui-2.la Makefile
Libxfce4ui_2_0_gir_INCLUDES = GObject-2.0 GLib-2.0 Gtk-3.0 Libxfce4util-1.0
Libxfce4ui_2_0_gir_CFLAGS = $(AM_CPPFLAGS)
Libxfce4ui_2_0_gir_LIBS = libxfce4ui-2.la
Libxfce4ui_2_0_gir_FILES = $(introspection_sources)
Libxfce4ui_2_0_gir_EXPORT_PACKAGES = libxfce4ui-2
INTROSPECTION_GIRS += Libxfce4ui-2.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

if ENABLE_X11
symbols_sed_script = /@BEGIN_X11_SYMBOLS@/d; /@END_X11_SYMBOLS@/d;
libxfce4ui_sed_script = /@BEGIN_X11_HEADERS@/d; /@END_X11_HEADERS@/d;
else
symbols_sed_script = /@BEGIN_X11_SYMBOLS@/,/@END_X11_SYMBOLS@/d;
libxfce4ui_sed_script = /@BEGIN_X11_HEADERS@/,/@END_X11_HEADERS@/d;
endif
if ENABLE_WAYLAND
symbols_sed_script += /@BEGIN_WAYLAND_SYMBOLS@/d; /@END_WAYLAND_SYMBOLS@/d
libxfce4ui_sed_script += /@BEGIN_WAYLAND_HEADERS@/d; /@END_WAYLAND_HEADERS@/d
else
symbols_sed_script += /@BEGIN_WAYLAND_SYMBOLS@/,/@END_WAYLAND_SYMBOLS@/d
libxfce4ui_sed_script += /@BEGIN_WAYLAND_HEADERS@/,/@END_WAYLAND_HEADERS@/d
endif

libxfce4ui.symbols: libxfce4ui.symbols.in
	$(AM_V_GEN) $(SED) "$(symbols_sed_script)" < $< > $@

libxfce4ui.h: libxfce4ui.h.in $(top_builddir)/config.h
	$(AM_V_GEN) $(SED) "$(libxfce4ui_sed_script)" < $< > $@

libxfce4ui-visibility.h: libxfce4ui.symbols Makefile
	$(AM_V_GEN) $(XDT_GEN_VISIBILITY) --kind=header $< $@

libxfce4ui-visibility.c: libxfce4ui.symbols Makefile
	$(AM_V_GEN) $(XDT_GEN_VISIBILITY) --kind=source $< $@

DISTCLEANFILES = \
	libxfce4ui-visibility.c \
	libxfce4ui-visibility.h \
	libxfce4ui.symbols \
	libxfce4ui.h

BUILT_SOURCES = \
	libxfce4ui-visibility.c \
	libxfce4ui-visibility.h \
	libxfce4ui.symbols \
	libxfce4ui.h

##
## Rules to auto-generate built sources
##
## This is a bit tricky with automake, and non-trivial to implement. The
## rules below seem to work fine and don't seem to break the build, but
## they are only enabled in maintainer mode, so arbitrary users don't get
## trapped in automake's oddities. Therefore we ship the autogenerated
## files as part of the dist tarball.
##
if MAINTAINER_MODE
DISTCLEANFILES += \
	stamp-libxfce4ui-marshal.h \
	$(libxfce4ui_built_sources)

BUILT_SOURCES += \
	$(libxfce4ui_built_sources)

libxfce4ui-marshal.h: stamp-libxfce4ui-marshal.h
	@true
stamp-libxfce4ui-marshal.h: libxfce4ui-marshal.list Makefile
	$(AM_V_GEN) glib-genmarshal --prefix=_libxfce4ui_marshal --header $(srcdir)/libxfce4ui-marshal.list >xgen-lmh
	cmp -s xgen-lmh libxfce4-marshal.h || cp xgen-lmh libxfce4ui-marshal.h
	rm -f xgen-lmh
	echo timestamp >$(@F)

libxfce4ui-marshal.c: libxfce4ui-marshal.list Makefile
	$(AM_V_GEN) echo "#include \"libxfce4ui-marshal.h\"" > xgen-lmc
	glib-genmarshal --prefix=_libxfce4ui_marshal --body $(srcdir)/libxfce4ui-marshal.list >>xgen-lmc
	cmp -s xgen-lmc libxfce4-marshal.c || cp xgen-lmc libxfce4ui-marshal.c
	rm -f xgen-lmc

libxfce4ui-enum-types.h: stamp-libxfce4ui-enum-types.h
	@true
stamp-libxfce4ui-enum-types.h: $(libxfce4ui_enum_headers) Makefile
	$(AM_V_GEN) ( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __LIBXFCE4UI_ENUM_TYPES_H__\n#define __LIBXFCE4UI_ENUM_TYPES_H__\n\nG_BEGIN_DECLS\n\n" \
		--fprod "/* enumerations from \"@filename@\" */\n\n" \
		--vhead "GType @enum_name@_get_type(void) G_GNUC_CONST;\n#define XFCE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n\n" \
		--ftail "G_END_DECLS\n\n#endif /* !__LIBXFCE4UI_ENUM_TYPES_H__ */" \
		$(libxfce4ui_enum_headers) ) >xgen-leth
	cmp -s xgen-leth libxfce4ui-enum-types.h || cp xgen-leth libxfce4ui-enum-types.h
	rm -f xgen-leth
	echo timestamp > $(@F)
libxfce4ui-enum-types.c: $(libxfce4ui_enum_headers) Makefile
	$(AM_V_GEN) ( cd $(srcdir) && glib-mkenums \
		--fhead "#include \"libxfce4ui.h\"\n#include \"libxfce4ui-visibility.h\"\n\n" \
		--fprod "/* enumerations from \"@filename@\" */\n\n" \
		--vhead "GType\n@enum_name@_get_type(void)\n{\n    static GType type = 0;\n\n    if(!type) {\n        static const G@Type@Value values[] = {"\
		--vprod "            { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "            { 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static(\"@EnumName@\", values);\n    }\n\n    return type;\n}\n\n" \
		--ftail "\n#define __LIBXFCE4UI_ENUM_TYPES_C__\n#include \"libxfce4ui-visibility.c\"\n" \
		$(libxfce4ui_enum_headers) ) > xgen-letc
	cp xgen-letc libxfce4ui-enum-types.c
	rm -f xgen-letc

libxfce4ui-resources.c: resources/libxfce4ui.gresource.xml $(LIBXFCE4UI_RESOURCE_FILES)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/resources --generate-source --internal --c-name libxfce4ui resources/libxfce4ui.gresource.xml

endif

# required for gtk-doc
dist-hook: all

EXTRA_DIST = \
	libxfce4ui.h.in \
	resources/libxfce4ui.gresource.xml \
	$(LIBXFCE4UI_RESOURCE_FILES) \
	libxfce4ui.symbols.in \
	libxfce4ui-marshal.list \
	libxfce4ui-2.deps \
	meson.build \
	xfce-icon-view-accessible.c \
	$(NULL)


if ENABLE_VAPIGEN
-include $(VAPIGEN_MAKEFILE)

libxfce4ui-2.vapi: Libxfce4ui-2.0.gir libxfce4ui-2.deps

VAPIGEN_VAPIS = libxfce4ui-2.vapi
VAPIGEN_GIRDIRS = $(prefix)/share/gir-1.0

libxfce4ui_2_vapi_METADATADIRS = .
libxfce4ui_2_vapi_FILES = Libxfce4ui-2.0.gir

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS) $(VAPIGEN_VAPIS:.vapi=.deps)

endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
