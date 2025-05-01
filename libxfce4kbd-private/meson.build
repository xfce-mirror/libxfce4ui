if get_option('keyboard-library')
  libxfce4kbd_libname = 'libxfce4kbd-private'
  libxfce4kbd_pkgname = '@0@-@1@'.format(libxfce4kbd_libname, libxfce4kbd_version_api)

  libxfce4kbd_install_subdir = 'xfce4' / libxfce4kbd_pkgname / libxfce4kbd_libname
  libxfce4kbd_install_dir = get_option('prefix') / get_option('includedir') / libxfce4kbd_install_subdir

  libxfce4kbd_headers = [
    'xfce-shortcut-dialog.h',
    'xfce-shortcuts-editor-dialog.h',
    'xfce-shortcuts-editor.h',
    'xfce-shortcuts-provider.h',
    'xfce-shortcuts-xfwm4.h',
    'xfce-shortcuts.h',
  ]

  libxfce4kbd_sources = [
    'xfce-shortcut-dialog.c',
    'xfce-shortcuts-editor-dialog.c',
    'xfce-shortcuts-editor.c',
    'xfce-shortcuts-provider.c',
    'xfce-shortcuts-xfwm4.c',
    'xfce-shortcuts.c',
  ]

  if enable_x11
    libxfce4kbd_headers += [
      'xfce-shortcuts-grabber.h',
    ]

    libxfce4kbd_sources += [
      'xfce-shortcuts-grabber.c',
    ]
  endif

  libxfce4kbd_sources += gnome.genmarshal(
    'xfce-shortcuts-marshal',
    sources: 'xfce-shortcuts-marshal.list',
    prefix: '_xfce_shortcuts_marshal',
    internal: true,
    install_header: false,
  )

  if enable_x11
    symbols_sed_script = '/@BEGIN_X11_SYMBOLS@/d; /@END_X11_SYMBOLS@/d;'
  else
    symbols_sed_script = '/@BEGIN_X11_SYMBOLS@/,/@END_X11_SYMBOLS@/d;'
  endif
  if enable_wayland
    symbols_sed_script += '/@BEGIN_WAYLAND_SYMBOLS@/d; /@END_WAYLAND_SYMBOLS@/d'
  else
    symbols_sed_script += '/@BEGIN_WAYLAND_SYMBOLS@/,/@END_WAYLAND_SYMBOLS@/d'
  endif
  symbols = custom_target(
    'libxfce4kbd-private.symbols',
    input: 'libxfce4kbd-private.symbols.in',
    output: 'libxfce4kbd-private.symbols',
    command: [sed, symbols_sed_script, '@INPUT@'],
    capture: true,
  )

  libxfce4kbd_sources += configure_file(
    input: symbols,
    output: 'libxfce4kbd-private-visibility.h',
    command: [xdt_gen_visibility, '--kind=header', '@INPUT@', '@OUTPUT@'],
  )
  libxfce4kbd_sources += configure_file(
    input: symbols,
    output: 'libxfce4kbd-private-visibility.c',
    command: [xdt_gen_visibility, '--kind=source', '@INPUT@', '@OUTPUT@'],
  )

  libxfce4kbd_deps = [
    glib,
    gtk,
    libxfce4util,
    xfconf,
    x11_deps,
  ]
  if need_libintl
    libxfce4kbd_deps += libintl
  endif

  libxfce4kbd_so_version = '0'
  libxfce4kbd_version = '@0@.0.0'.format(libxfce4kbd_so_version)

  libxfce4kbd = library(
    libxfce4kbd_pkgname.substring(3),
    libxfce4kbd_sources,
    sources: libxfce4ui_h,
    version: libxfce4kbd_version,
    soversion: libxfce4kbd_so_version,
    c_args: [
      '-DG_LOG_DOMAIN="@0@"'.format(libxfce4kbd_libname),
    ],
    include_directories: [
      include_directories('..'),
    ],
    gnu_symbol_visibility: gnu_symbol_visibility,
    dependencies: libxfce4kbd_deps,
    link_with: [
      libxfce4ui,
    ],
    install: true,
  )

  install_headers(
    libxfce4kbd_headers,
    subdir: libxfce4kbd_install_subdir,
  )

  pkgconfig.generate(
    libraries: [
      libxfce4kbd,
    ],
    version: meson.project_version(),
    name: libxfce4kbd_libname,
    filebase: libxfce4kbd_pkgname,
    description: 'Private Xfce library for shared code between xfwm4 and xfce4-settings',
    subdirs: [
      'xfce4' / libxfce4kbd_pkgname,
    ],
    install_dir: get_option('prefix') / get_option('libdir') / 'pkgconfig',
  )

  install_data(
    'xfce4-keyboard-shortcuts.xml',
    install_dir: get_option('prefix') / get_option('sysconfdir') / 'xdg' / 'xfce4' / 'xfconf' / 'xfce-perchannel-xml'
  )
endif
