
# The name of the module.
DOC_MODULE=libxfce4ui

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS= --deprecated-guards="XFCE_DISABLE_DEPRECATED" --rebuild-types

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR=$(top_srcdir)/libxfce4ui

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--xml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=

# Used for dependencies
HFILE_GLOB=
CFILE_GLOB=

# Header files to ignore when scanning
IGNORE_HFILES = \
	libxfce4ui-visibility.h \
	xfce-icon-chooser-model.h \
	xfce-thumbnail-preview.h \
	xfce-thumbnail.h \
	$(NULL)

if !ENABLE_X11
IGNORE_HFILES += \
	xfce-clipboard-manager.h \
	xfce-sm-client.h
endif

# Extra files to add when scanning (relative to $srcdir)
EXTRA_HFILES=

# Images to copy into HTML directory
HTML_IMAGES =

# Extra SGML files that are included by DOC_MAIN_SGML_FILE
content_files=

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
GTKDOC_CFLAGS= -I$(top_srcdir) -I$(top_builddir) $(LIBXFCE4UTIL_CFLAGS) $(GTK3_CFLAGS) 
GTKDOC_LIBS= $(top_builddir)/libxfce4ui/libxfce4ui-$(LIBXFCE4UI_VERSION_API).la


include $(top_srcdir)/gtk-doc.make

$(top_builddir)/docs/reference/$(DOC_MODULE)-docs.xml: $(DOC_MODULE)-docs.xml.in
	$(AM_V_GEN) $(SED) -e "s,\@COPYRIGHT_YEAR\@,@COPYRIGHT_YEAR@,g" $< >$@

EXTRA_DIST += \
	gtkdocentities.ent.in \
	$(DOC_MODULE)-docs.xml.in \
	meson.build \
	$(NULL)

DISTCLEANFILES = \
	$(DOC_MODULE)-docs.xml \
	$(NULL)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
