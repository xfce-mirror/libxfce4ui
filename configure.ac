dnl
dnl Copyright (c) 2007-2024 The Xfce development team
dnl

dnl ***************************
dnl *** Version information ***
dnl ***************************
XDT_VERSION_INIT([4.21.1], [git])
m4_define([copyright_year], [2025])

m4_define([libxfce4ui_verinfo], [0:0:0])
m4_define([libxfce4ui_version_api], [2])

m4_define([libxfce4kbd_private_verinfo], [0:0:0])
m4_define([libxfce4kbd_private_version_api], [3])

m4_define([glib_min_version], [2.72.0])
m4_define([gtk_min_version], [3.24.0])
m4_define([gdk_pixbuf_min_version], [2.42.8])
m4_define([libxfce4util_min_version], [4.17.2])
m4_define([xfconf_min_version], [4.12.0])

m4_define([libx11_min_version], [1.6.7])
m4_define([libice_min_version], [1.0.10])
m4_define([libsm_min_version], [1.2.3])

m4_define([libstartup_notif_min_version], [0.4])

m4_define([libgtop_min_version], [2.24.0])
m4_define([epoxy_min_version], [1.2])
m4_define([gudev_min_version], [232])
m4_define([gladeui2_min_version], [3.5.0])

dnl # DO NOT MODIFY ANYTHING BELOW THIS LINE, UNLESS YOU KNOW WHAT
dnl # YOU ARE DOING.

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2007-copyright_year() The Xfce development team])
AC_INIT([libxfce4ui], [xdt_version], [https://gitlab.xfce.org/xfce/libxfce4ui/-/issues], [libxfce4ui])
AC_PREREQ([2.69])
AC_CONFIG_MACRO_DIRS([m4])
AC_REVISION([xdt_version_build])
AC_CANONICAL_TARGET()
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])
COPYRIGHT_YEAR=copyright_year()
AC_DEFINE_UNQUOTED([COPYRIGHT_YEAR], ["$COPYRIGHT_YEAR"], [Copyright year])
AC_SUBST([COPYRIGHT_YEAR])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar no-dist-gzip foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
AM_SILENT_RULES([yes])

dnl *******************************
dnl *** Check for UNIX variants ***
dnl *******************************
AC_USE_SYSTEM_EXTENSIONS

AC_MSG_CHECKING([platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform="win32"
    NO_UNDEFINED=-no-undefined
    ;;
  *-apple-darwin*)
    platform="os x"
    ;;
  *)
    platform="unix"
    ;;
esac
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform" = "win32")
AM_CONDITIONAL(PLATFORM_OSX, test "$platform" = "os x")
AC_SUBST(NO_UNDEFINED)

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AC_PROG_CC()
LT_PATH_LD([])
AM_PROG_CC_C_O()
AC_PROG_INSTALL()
AC_PATH_PROG([XDT_GEN_VISIBILITY], [xdt-gen-visibility])
AC_PATH_PROG([GLIB_COMPILE_RESOURCES],[glib-compile-resources])
AC_PROG_SED

dnl **************************
dnl *** Initialize libtool ***
dnl **************************
LT_PREREQ([2.4])
LT_INIT([disable-static])

dnl **************************************
dnl *** Substitute version information ***
dnl **************************************
LIBXFCE4UI_VERINFO=libxfce4ui_verinfo()
LIBXFCE4UI_VERSION_API=libxfce4ui_version_api()
LIBXFCE4UI_VERSION_MAJOR=xdt_version_major()
LIBXFCE4UI_VERSION_MINOR=xdt_version_minor()
LIBXFCE4UI_VERSION_MICRO=xdt_version_micro()
LIBXFCE4KBD_PRIVATE_VERINFO=libxfce4kbd_private_verinfo()
LIBXFCE4KBD_PRIVATE_VERSION_API=libxfce4kbd_private_version_api()
AC_SUBST([LIBXFCE4UI_VERINFO])
AC_SUBST([LIBXFCE4UI_VERSION_API])
AC_SUBST([LIBXFCE4UI_VERSION_MAJOR])
AC_SUBST([LIBXFCE4UI_VERSION_MINOR])
AC_SUBST([LIBXFCE4UI_VERSION_MICRO])
AC_SUBST([LIBXFCE4KBD_PRIVATE_VERINFO])
AC_SUBST([LIBXFCE4KBD_PRIVATE_VERSION_API])

dnl ***************************************
dnl *** Check for standard header files ***
dnl ***************************************
AC_CHECK_HEADERS([crt_externs.h errno.h fcntl.h limits.h locale.h math.h \
                  memory.h signal.h stdarg.h stdlib.h string.h unistd.h \
                  sys/mman.h sys/stat.h sys/types.h mmintrin.h time.h stdio.h])
AC_CHECK_DECLS([environ])
AC_FUNC_MMAP()
AC_CHECK_FUNCS([_NSGetEnviron realpath])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])
AC_SEARCH_LIBS([bind_textdomain_codeset], [intl],
  [AC_DEFINE([HAVE_BIND_TEXTDOMAIN_CODESET], [1], [Define to 1 if you have the 'bind_textdomain_codeset' function.])],
  [])

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GIO_UNIX], [gio-unix-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [glib_min_version])
XDT_CHECK_PACKAGE([GTK3], [gtk+-3.0], [gtk_min_version])
XDT_CHECK_PACKAGE([GDK_PIXBUF], [gdk-pixbuf-2.0], [gdk_pixbuf_min_version])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [libxfce4util_min_version])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [xfconf_min_version])

dnl ***********************************
dnl *** Check for optional packages ***
dnl ***********************************
XDT_CHECK_OPTIONAL_FEATURE([X11],
                           [x11],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_X11], [gdk-x11-3.0], [gtk_min_version])
                             XDT_FEATURE_DEPENDENCY([LIBX11], [x11], [libx11_min_version])
                           ],
                           [the X11 windowing system])
XDT_CHECK_OPTIONAL_FEATURE([WAYLAND],
                           [wayland],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_WAYLAND], [gdk-wayland-3.0], [gtk_min_version])
                           ],
                           [the Wayland windowing system])
if test x"$ENABLE_X11" != x"yes" -a x"$ENABLE_WAYLAND" != x"yes"; then
  AC_MSG_ERROR([Either both X11 and Wayland support was disabled, or required dependencies are missing. One of the two must be enabled.])
fi

if test x"$ENABLE_X11" != x"yes"; then
  dnl Do not check for optional X11 packages if X11 is disabled
  enable_libsm=no
  enable_startup_notification=no
fi
XDT_CHECK_OPTIONAL_FEATURE([LIBSM],
                           [libsm],
                           [
                             XDT_FEATURE_DEPENDENCY([LIBICE], [ice], [libice_min_version])
                             XDT_FEATURE_DEPENDENCY([LIBSM], [sm], [libsm_min_version])
                           ],
                           [session management library])
XDT_CHECK_OPTIONAL_PACKAGE([LIBSTARTUP_NOTIFICATION],
                           [libstartup-notification-1.0],
                           [libstartup_notif_min_version], [startup-notification],
                           [startup notification library])

dnl ****************************************************************
dnl *** Optional support for system information based on glibtop ***
dnl ****************************************************************
XDT_CHECK_OPTIONAL_PACKAGE([GLIBTOP],
                           [libgtop-2.0], [libgtop_min_version], [glibtop],
                           [GLIBTOP support])

dnl **************************************************************
dnl *** Check time.h and sys/time.h for libgtop ***
dnl **************************************************************
if test x"$GLIBTOP_FOUND" = x"yes"; then
  AC_CHECK_HEADERS([time.h sys/time.h])
fi

dnl **************************************************************
dnl *** Optional support for system information based on epoxy ***
dnl **************************************************************
EPOXY_FOUND="no"
if test x"$GLIBTOP_FOUND" = x"yes"; then
  XDT_CHECK_OPTIONAL_PACKAGE([EPOXY],
                             [epoxy],
                             [epoxy_min_version], [epoxy],
                             [library for handling OpenGL function pointer management], [yes])
else
  AM_CONDITIONAL([HAVE_EPOXY], [false])
fi

dnl **********************************
dnl *** Optional support for GUdev ***
dnl **********************************
GUDEV_FOUND="no"
if test x"$GLIBTOP_FOUND" = x"yes"; then
  XDT_CHECK_OPTIONAL_PACKAGE([GUDEV], [gudev-1.0], [gudev_min_version], [gudev],
                             [GObject bindings for libudev], [yes])
else
  AM_CONDITIONAL([HAVE_GUDEV], [false])
fi

dnl ***************************************
dnl *** Check for gobject-introspection ***
dnl ***************************************
GOBJECT_INTROSPECTION_CHECK([1.72.0])

dnl *************************
dnl *** Check for vapigen ***
dnl *************************
VAPIGEN_CHECK()

dnl **************************************************
dnl *** Check whether to build the keyboard libary ***
dnl **************************************************
AC_ARG_ENABLE([keyboard-library],
              [AS_HELP_STRING([--disable-keyboard-library],
                              [Do not compile the keyboard library needed by xfwm4 and xfce4-settings (default=enabled)])],
              [enable_keyboard_library=$enableval], [enable_keyboard_library=yes])
AM_CONDITIONAL([ENABLE_KEYBOARD_LIBRARY], [test "x$enable_keyboard_library" = "xyes"])

dnl *********************************************
dnl *** Check for vendor specific information ***
dnl *********************************************
AC_MSG_CHECKING([for additional vendor name and/or info])
AC_ARG_WITH([vendor-info],
AS_HELP_STRING([--with-vendor-info=NAME], [Specify an additional vendor name, optionally with a file in $prefix/share/xfce4/venderinfo]),
    [with_vendor_info="$withval"], [with_vendor_info=""])
if test x"$with_vendor_info" != x""; then
    AC_DEFINE_UNQUOTED([VENDOR_INFO], ["$with_vendor_info"], [Additional vendor name and/or info])
    AC_MSG_RESULT([$vendorinfo])
else
    AC_MSG_RESULT([not set])
fi

dnl *********************************************
dnl *** Check for vendor specific information ***
dnl *********************************************
AC_MSG_CHECKING([for a custom manual website location])
AC_ARG_WITH([manual-website],
AS_HELP_STRING([--with-manual-website=URI], [Specify an optional manual website, defaults to https://docs.xfce.org.]),
    [with_manual_website="$withval"],
    [with_manual_website="https://docs.xfce.org/help.php"])
AC_DEFINE_UNQUOTED([MANUAL_WEBSITE], ["$with_manual_website"], [Documentation website])
AC_MSG_RESULT([$with_manual_website])

dnl *********************************************************
dnl *** Optional support for the Glade Interface Designer ***
dnl *********************************************************

XDT_CHECK_OPTIONAL_PACKAGE([GLADEUI2], [gladeui-2.0],
                           [gladeui2_min_version], [gladeui2], [glade interface designer])

AC_ARG_WITH([gladeui2-catalogdir],
            [AS_HELP_STRING([--with-gladeui2-catalogdir=PATH],
                            [Specify where to install glade 3.0 designer catalog files (default=autodetect)])],
            [gladeui2_catalogdir="$withval"])
AC_ARG_WITH([gladeui2-moduledir],
            [AS_HELP_STRING([--with-gladeui2-moduledir=PATH],
                            [Specify where to install glade 3.0 designer modules (default=autodetect)])],
            [gladeui2_moduledir="$withval"])
AC_ARG_WITH([gladeui2-pixmapdir],
            [AS_HELP_STRING([--with-gladeui2-pixmapdir=PATH],
                            [Specify where to install glade 3.0 designer pixmaps (default=autodetect)])],
            [gladeui2_pixmapdir="$withval"])
if test x"$GLADEUI2_FOUND" = x"yes"; then
  if test x"$gladeui2_catalogdir" = x""; then
    GLADEUI2_CATALOGDIR=`$PKG_CONFIG "--variable=catalogdir" "gladeui-2.0"`
  else
    GLADEUI2_CATALOGDIR="$gladeui2_catalogdir"
  fi
  if test x"$gladeui2_moduledir" = x""; then
    GLADEUI2_MODULEDIR=`$PKG_CONFIG "--variable=moduledir" "gladeui-2.0"`
  else
    GLADEUI2_MODULEDIR="$gladeui2_moduledir"
  fi
  if test x"$gladeui2_pixmapdir" = x""; then
    GLADEUI2_PIXMAPDIR=`$PKG_CONFIG "--variable=pixmapdir" "gladeui-2.0"`
  else
    GLADEUI2_PIXMAPDIR="$gladeui2_pixmapdir"
  fi
fi
AC_SUBST([GLADEUI2_CATALOGDIR])
AC_SUBST([GLADEUI2_MODULEDIR])
AC_SUBST([GLADEUI2_PIXMAPDIR])

dnl *************************
dnl *** Check for gtk-doc ***
dnl *************************
GTK_DOC_CHECK([1.0])

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl **************************************
dnl *** Check for linker optimizations ***
dnl **************************************
XDT_FEATURE_LINKER_OPTS()

dnl ****************************************
dnl *** Check for ELF visibility support ***
dnl ****************************************
AC_ARG_ENABLE([visibility],
              AS_HELP_STRING([--disable-visibility],
                             [Do not use ELF visibility attributes]),
              [enable_visibility=$enableval], [enable_visibility=yes])
have_gnuc_visibility=no
if test "x$enable_visibility" != "xno"; then
  XDT_SUPPORTED_FLAGS([xdt_vis_test_cflags], [-Wall -Werror -Wno-unused-parameter -fvisibility=hidden])
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $xdt_vis_test_cflags"
  AC_MSG_CHECKING([whether $CC supports the GNUC visibility attribute])
  AC_COMPILE_IFELSE([AC_LANG_SOURCE(
  [
    void test_default(void);
    void test_hidden(void);
    void __attribute__((visibility("default"))) test_default(void) {}
    void __attribute__((visibility("hidden"))) test_hidden(void) {}
    int main(int argc, char **argv) {
      test_default();
      test_hidden();
      return 0;
    }
  ])],
  [
    have_gnuc_visibility=yes
    AC_MSG_RESULT([yes])
  ],
  [
    AC_MSG_RESULT([no])
  ])
  CFLAGS="$saved_CFLAGS"
fi
if test "x$have_gnuc_visibility" = "xyes"; then
  CPPFLAGS="$CPPFLAGS -DENABLE_SYMBOL_VISIBILITY=1"
  CFLAGS="$CFLAGS -fvisibility=hidden"
fi

dnl *************************************
dnl *** Compensate for broken gtk-doc ***
dnl *************************************
XDT_SUPPORTED_FLAGS([GTK_DOC_EXTRA_CFLAGS], [-Wno-sign-compare])
AC_SUBST([GTK_DOC_EXTRA_CFLAGS])

#
# Enable Tests
#
AC_ARG_ENABLE(tests,
	[AS_HELP_STRING([--enable-tests],[enable validation test framework @<:@default=@<:@disabled@:>@@:>@])],
	[enable_tests=$enableval], [enable_tests=no])
AC_MSG_CHECKING([whether to build the validation test framework])
AM_CONDITIONAL(BUILD_TESTS, test "x$enable_tests" = "xyes")
AC_MSG_RESULT($enable_tests)

dnl *********************************
dnl *** Substitute platform flags ***
dnl *********************************
AC_MSG_CHECKING([PLATFORM_CPPFLAGS])
AC_MSG_RESULT([$PLATFORM_CPPFLAGS])
AC_SUBST([PLATFORM_CPPFLAGS])
AC_MSG_CHECKING([PLATFORM_CFLAGS])
AC_MSG_RESULT([$PLATFORM_CFLAGS])
AC_SUBST([PLATFORM_CFLAGS])
AC_MSG_CHECKING([PLATFORM_LDFLAGS])
AC_MSG_RESULT([$PLATFORM_LDFLAGS])
AC_SUBST([PLATFORM_LDFLAGS])

dnl *****************************
dnl *** Generate output files ***
dnl *****************************
AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
glade/Makefile
glade/icons/Makefile
glade/icons/16x16/Makefile
glade/icons/22x22/Makefile
icons/Makefile
icons/16x16/Makefile
icons/48x48/Makefile
icons/128x128/Makefile
icons/pixmaps/Makefile
icons/scalable/Makefile
libxfce4kbd-private/libxfce4kbd-private-3.pc
libxfce4kbd-private/Makefile
libxfce4ui/libxfce4ui-2.pc
libxfce4ui/libxfce4ui-config.h
libxfce4ui/Makefile
xfce4-about/Makefile
po/Makefile.in
tests/Makefile
xfce-desktop-item-edit/Makefile
xfce-open/Makefile
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
if test x"$GLADEUI2_FOUND" = x"yes"; then
echo "* Glade 3.0 UI Designer:     yes"
echo "  - Catalog directory:       $GLADEUI2_CATALOGDIR"
echo "  - Module directory:        $GLADEUI2_MODULEDIR"
echo "  - Pixmap directory:        $GLADEUI2_PIXMAPDIR"
else
echo "* Glade 3.0 UI Designer:     no"
fi
if test x"$enable_keyboard_library" = x"yes"; then
echo "* Keyboard library support:  yes"
else
echo "* Keyboard library support:  no"
fi
echo "* X11 Support:               ${ENABLE_X11:-no}"
if test x"$ENABLE_X11" = x"yes"; then
echo "  * Libsm support:           ${ENABLE_LIBSM:-no}"
echo "  * Startup notif support:   ${LIBSTARTUP_NOTIFICATION_FOUND:-no}"
fi
echo "* Wayland Support:           ${ENABLE_WAYLAND:-no}"
echo "* Debug support:             $enable_debug"
echo "* Build tests:               $enable_tests"
echo "* GNU Visibility:            $have_gnuc_visibility"
if test -n "$with_vendor_info"; then
echo "* Vendor:                    $with_vendor_info"
echo
echo "Note you can put additional info about the vendor"
echo "in this text file: \"${datarootdir}/xfce4/vendorinfo\""
else
echo "* Vendor:                    none"
fi
if test x"$GLIBTOP_FOUND" = x"yes"; then
echo "* System Info (libgtop):     yes"
echo "* System Info (epoxy):       $EPOXY_FOUND"
echo "* System Info (gudev):       $GUDEV_FOUND"
else
echo "* System Info (libgtop):     no"
echo "* System Info (epoxy):       no (assumes libgtop)"
echo "* System Info (gudev):       no (assumes libgtop)"
fi
echo "* Manual website:            $with_manual_website"
echo
