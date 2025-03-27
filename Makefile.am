ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}


SUBDIRS = \
	libxfce4ui \
	libxfce4kbd-private \
	xfce4-about \
	glade \
	icons \
	docs \
	po \
	tests \
	xfce-desktop-item-edit \
	xfce-open \
	$(NULL)

distuninstallcheck_listfiles = \
	find . -type f -print | grep -v ./share/icons/hicolor/icon-theme.cache

distclean-local:
	rm -rf *.cache *~

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

EXTRA_DIST = \
	README.md \
	m4/introspection.m4 \
	m4/vapigen.m4 \
	meson.build \
	meson_options.txt \
	po/meson.build \
	xfce-revision.h.in \
	$(NULL)

AM_DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc \
	--disable-gladeui2 \
	--enable-introspection \
	--disable-vala

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
