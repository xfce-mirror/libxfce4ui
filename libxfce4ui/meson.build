libxfce4ui_libname = 'libxfce4ui'
libxfce4ui_pkgname = '@0@-@1@'.format(libxfce4ui_libname, libxfce4ui_version_api)

libxfce4ui_install_subdir = 'xfce4' / libxfce4ui_pkgname / libxfce4ui_libname
libxfce4ui_install_dir = get_option('prefix') / get_option('includedir') / libxfce4ui_install_subdir

libxfce4ui_enum_headers = [
  'libxfce4ui-enums.h',
]

libxfce4ui_headers = libxfce4ui_enum_headers + [
  'xfce-cell-renderer-icon.h',
  'xfce-dialogs.h',
  'xfce-execute.h',
  'xfce-filename-input.h',
  'xfce-gdk-extensions.h',
  'xfce-gdk-pixbuf-extensions.h',
  'xfce-gtk-extensions.h',
  'xfce-icon-chooser-dialog.h',
  'xfce-icon-view.h',
  'xfce-notebook.h',
  'xfce-screensaver.h',
  'xfce-spawn.h',
  'xfce-titled-dialog.h',
  'xfce-tree-view.h',
]

# do not add this header to GI sources, it creates unsolvable conflicts with other
# libs whose identifier prefix is Xfce, e.g. libxfce4util and libxfce4panel
configure_file(
  configuration: configuration_data({
    'LIBXFCE4UI_VERSION_MAJOR': version_parts[0],
    'LIBXFCE4UI_VERSION_MINOR': version_parts[1],
    'LIBXFCE4UI_VERSION_MICRO': version_parts[2],
  }),
  input: 'libxfce4ui-config.h.in',
  output: 'libxfce4ui-config.h',
  install: true,
  install_dir: libxfce4ui_install_dir,
)

if enable_x11
libxfce4ui_headers += [
  'xfce-clipboard-manager.h',
  'xfce-sm-client.h',
]
endif

libxfce4ui_sources_gir = [
  'libxfce4ui-config.c',
  'xfce-cell-renderer-icon.c',
  'xfce-dialogs.c',
  'xfce-execute.c',
  'xfce-filename-input.c',
  'xfce-gdk-extensions.c',
  'xfce-gdk-pixbuf-extensions.c',
  'xfce-gtk-extensions.c',
  'xfce-icon-chooser-dialog.c',
  'xfce-icon-view.c',
  'xfce-notebook.c',
  'xfce-screensaver.c',
  'xfce-spawn.c',
  'xfce-titled-dialog.c',
  'xfce-tree-view.c',
]

if enable_x11
libxfce4ui_sources_gir += [
  'xfce-clipboard-manager.c',
  'xfce-sm-client.c',
]
endif

libxfce4ui_sources = libxfce4ui_sources_gir + [
  'libxfce4ui-private.h',
  'xfce-icon-chooser-model.c',
  'xfce-icon-chooser-model.h',
  'xfce-thumbnail-preview.c',
  'xfce-thumbnail-preview.h',
  'xfce-thumbnail.c',
  'xfce-thumbnail.h',
]

libxfce4ui_sources += gnome.genmarshal(
  'libxfce4ui-marshal',
  sources: 'libxfce4ui-marshal.list',
  prefix: '_libxfce4ui_marshal',
  internal: true,
  install_header: false,
)

if enable_x11
  libxfce4ui_sed_script = '/@BEGIN_X11_HEADERS@/d; /@END_X11_HEADERS@/d;'
  symbols_sed_script = '/@BEGIN_X11_SYMBOLS@/d; /@END_X11_SYMBOLS@/d;'
else
  libxfce4ui_sed_script = '/@BEGIN_X11_HEADERS@/,/@END_X11_HEADERS@/d;'
  symbols_sed_script = '/@BEGIN_X11_SYMBOLS@/,/@END_X11_SYMBOLS@/d;'
endif
if enable_wayland
  libxfce4ui_sed_script += '/@BEGIN_WAYLAND_HEADERS@/d; /@END_WAYLAND_HEADERS@/d'
  symbols_sed_script += '/@BEGIN_WAYLAND_SYMBOLS@/d; /@END_WAYLAND_SYMBOLS@/d'
else
  libxfce4ui_sed_script += '/@BEGIN_WAYLAND_HEADERS@/,/@END_WAYLAND_HEADERS@/d'
  symbols_sed_script += '/@BEGIN_WAYLAND_SYMBOLS@/,/@END_WAYLAND_SYMBOLS@/d'
endif
libxfce4ui_h = custom_target(
  'libxfce4ui.h',
  input: 'libxfce4ui.h.in',
  output: 'libxfce4ui.h',
  command: [sed, libxfce4ui_sed_script, '@INPUT@'],
  capture: true,
  install: true,
  install_dir: libxfce4ui_install_dir,
)
symbols = custom_target(
  'libxfce4ui.symbols',
  input: 'libxfce4ui.symbols.in',
  output: 'libxfce4ui.symbols',
  command: [sed, symbols_sed_script, '@INPUT@'],
  capture: true,
)

libxfce4ui_sources += custom_target(
  'libxfce4ui-visibility.h',
  input: symbols,
  output: 'libxfce4ui-visibility.h',
  command: [xdt_gen_visibility, '--kind=header', '@INPUT@', '@OUTPUT@'],
)
libxfce4ui_sources += custom_target(
  'libxfce4ui-visibility.c',
  input: symbols,
  output: 'libxfce4ui-visibility.c',
  command: [xdt_gen_visibility, '--kind=source', '@INPUT@', '@OUTPUT@'],
)

libxfce4ui_resources_c = gnome.compile_resources(
  'libxfce4ui-resources',
  'resources' / 'libxfce4ui.gresource.xml',
  source_dir: 'resources',
  c_name: 'libxfce4ui',
  extra_args: [
    '--internal',
  ],
  export: false,
  install: false,
)

libxfce4ui_enums = []
libxfce4ui_enums += gnome.mkenums(
  'libxfce4ui-enum-types.h',
  install_header: true,
  install_dir: libxfce4ui_install_dir,
  sources: libxfce4ui_enum_headers,
  fhead: '#if !defined(_LIBXFCE4UI_INSIDE_LIBXFCE4UI_H) && !defined(LIBXFCE4UI_COMPILATION)\n#error "Only <libxfce4ui/libxfce4ui.h> can be included directly, this file is not part of the public API."\n#endif\n\n#ifndef __LIBXFCE4UI_ENUM_TYPES_H__\n#define __LIBXFCE4UI_ENUM_TYPES_H__\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n\n',
  fprod: '/* enumerations from "@filename@" */\n\n',
  vhead: 'GType @enum_name@_get_type(void) G_GNUC_CONST;\n#define XFCE_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n\n',
  ftail: 'G_END_DECLS\n\n#endif /* !__LIBXFCE4UI_ENUM_TYPES_H__ */',
)
libxfce4ui_enums += gnome.mkenums(
  'libxfce4ui-enum-types.c',
  install_header: false,
  sources: libxfce4ui_enum_headers,
  fhead: '#include "libxfce4ui-enums.h"\n#include "xfce-gtk-extensions.h"\n#include "libxfce4ui-enum-types.h"\n#include "libxfce4ui-visibility.h"\n\n',
  fprod: '/* enumerations from "@filename@" */\n\n',
  vhead: 'GType\n@enum_name@_get_type(void)\n{\n    static GType type = 0;\n\n    if(!type) {\n        static const G@Type@Value values[] = {',
  vprod: '            { @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
  vtail: '            { 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static("@EnumName@", values);\n    }\n\n    return type;\n}\n\n',
  ftail: '\n#define __LIBXFCE4UI_ENUM_TYPES_C__\n#include "libxfce4ui-visibility.c"\n',
)

libxfce4ui_deps = [
  gio_unix,
  gtk,
  gdk_pixbuf,
  libxfce4util,
  xfconf,
  x11_deps,
  libm,
]

libxfce4ui_so_version = '0'
libxfce4ui_version = '@0@.0.0'.format(libxfce4ui_so_version)

libxfce4ui = library(
  libxfce4ui_pkgname.substring(3),
  sources: libxfce4ui_sources + libxfce4ui_enums + libxfce4ui_resources_c,
  version: libxfce4ui_version,
  soversion: libxfce4ui_so_version,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format(libxfce4ui_libname),
    '-DLIBXFCE4UI_COMPILATION=1',
    '-DSN_API_NOT_YET_FROZEN=1',
  ],
  include_directories: [
    include_directories('..'),
  ],
  gnu_symbol_visibility: gnu_symbol_visibility,
  dependencies: libxfce4ui_deps,
  install: true,
)

install_headers(
  libxfce4ui_headers,
  subdir: libxfce4ui_install_subdir,
)

pkgconfig.generate(
  libraries: [
    libxfce4ui,
  ],
  version: meson.project_version(),
  name: libxfce4ui_libname,
  filebase: libxfce4ui_pkgname,
  description: 'Widgets library for Xfce',
  subdirs: [
    'xfce4' / libxfce4ui_pkgname,
  ],
  install_dir: get_option('prefix') / get_option('libdir') / 'pkgconfig',
)

if get_option('introspection')
  libxfce4ui_gir = gnome.generate_gir(
    libxfce4ui,
    sources: libxfce4ui_sources_gir + libxfce4ui_headers + libxfce4ui_enums,
    namespace: 'Libxfce4ui',
    nsversion: '@0@.0'.format(libxfce4ui_version_api),
    identifier_prefix: 'Xfce',
    symbol_prefix: [
      'xfce',
    ],
    export_packages: libxfce4ui_pkgname,
    includes: [
      'GLib-2.0',
      'GObject-2.0',
      'Gio-2.0',
      'Gtk-3.0',
      'Libxfce4util-1.0',
    ],
    header: 'libxfce4ui' / 'libxfce4ui.h',
    install: true,
    extra_args: [
      '-DLIBXFCE4UI_COMPILATION=1',
    ],
  )

  if generate_vala
    gnome.generate_vapi(
      libxfce4ui_pkgname,
      install: true,
      packages: [
        'glib-2.0',
        'gobject-2.0',
        'gio-2.0',
        'gtk+-3.0',
        'libxfce4util-1.0',
      ],
      sources: libxfce4ui_gir[0],
    )
  endif
endif

libxfce4ui_dep = declare_dependency(
  dependencies: libxfce4ui_deps,
  link_with: libxfce4ui,
)
