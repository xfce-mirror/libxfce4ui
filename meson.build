project(
  'libxfce4ui',
  'c',
version : '4.21.0-dev',
  license : 'GPL-2.0-or-later',
  meson_version : '>= 0.57.0',
  default_options : ['c_std=gnu11', 'buildtype=debugoptimized', 'warning_level=2']
)

project_namespace = 'xfce'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
copyright_year = '2025'
libxfce4ui_version_api = '2'
libxfce4kbd_version_api = '3'

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'glib': '>= 2.72.0',
  'gtk': '>= 3.24.0',
  'gdk-pixbuf': '>= 2.42.8',
  'libxfce4util': '>= 4.17.2',
  'xfconf': '>= 4.12.0',

  'libx11': '>= 1.6.7',
  'libice': '>= 1.0.10',
  'libsm': '>= 1.2.3',
  'libstartup-notif': '>= 0.4',

  'libgtop': '>= 2.24.0',
  'epoxy': '>= 1.2',
  'gudev': '>= 232',
  'glade': '>= 3.5.0',
  'gir': '>= 1.72.0',
}

glib = dependency('glib-2.0', version: dependency_versions['glib'])
gio = dependency('gio-2.0', version: dependency_versions['glib'])
gio_unix = dependency('gio-unix-2.0', version: dependency_versions['glib'])
gthread = dependency('gthread-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
gdk_pixbuf = dependency('gdk-pixbuf-2.0', version: dependency_versions['gdk-pixbuf'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['libxfce4util'])
xfconf = dependency('libxfconf-0', version: dependency_versions['xfconf'])

xdt_gen_visibility = find_program('xdt-gen-visibility', required: true)
sed = find_program('sed', required: true)

# Feature: 'x11'
x11_deps = []
x11_deps += dependency('gdk-x11-3.0', version: dependency_versions['gtk'], required: get_option('x11'))
x11_deps += dependency('x11', version: dependency_versions['libx11'], required: get_option('x11'))

# Feature: 'wayland'
wayland_deps = []
wayland_deps += dependency('gdk-wayland-3.0', version: dependency_versions['gtk'], required: get_option('wayland'))

feature_cflags = []

enable_x11 = not get_option('x11').disabled()
foreach dep : x11_deps
  enable_x11 = enable_x11 and dep.found()
endforeach
if enable_x11
  feature_cflags += '-DENABLE_X11=1'

  libsm = dependency('sm', version: dependency_versions['libsm'], required: get_option('session-management'))
  if libsm.found()
    libice = dependency('ice', version: dependency_versions['libice'], required: get_option('session-management'))
    if libice.found()
      feature_cflags += '-DENABLE_LIBSM=1'
      x11_deps += [libsm, libice]
    endif
  endif

  libstartup_notif = dependency('libstartup-notification-1.0', version: dependency_versions['libstartup-notif'], required: get_option('startup-notification'))
  if libstartup_notif.found()
    feature_cflags += '-DHAVE_LIBSTARTUP_NOTIFICATION=1'
    x11_deps += libstartup_notif
  endif
endif

enable_wayland = not get_option('wayland').disabled()
foreach dep : wayland_deps
  enable_wayland = enable_wayland and dep.found()
endforeach

if not enable_x11 and not enable_wayland
  error('At least one of the X11 and Wayland backends must be enabled')
endif

libgtop = dependency('libgtop-2.0', version: dependency_versions['libgtop'], required: get_option('libgtop'))
headers = []
if libgtop.found()
  feature_cflags += '-DHAVE_GLIBTOP=1'
  headers += [
    'sys/time.h',
    'time.h',
  ]

  epoxy = dependency('epoxy', version: dependency_versions['epoxy'], required: get_option('epoxy'))
  if epoxy.found()
    feature_cflags += '-DHAVE_EPOXY=1'
  endif

  gudev = dependency('gudev-1.0', version: dependency_versions['gudev'], required: get_option('gudev'))
  if gudev.found()
    feature_cflags += '-DHAVE_GUDEV=1'
  endif
else
  epoxy = dependency('', required: false)
  gudev = dependency('', required: false)
endif

glade = dependency('gladeui-2.0', version: dependency_versions['glade'], required: get_option('glade'))
if glade.found()
  glade_catalogdir = get_option('glade-catalogdir')
  if glade_catalogdir == ''
    glade_catalogdir = glade.get_variable(pkgconfig: 'catalogdir')
  endif

  glade_moduledir = get_option('glade-moduledir')
  if glade_moduledir == ''
    glade_moduledir = glade.get_variable(pkgconfig: 'moduledir')
  endif

  glade_pixmapdir = get_option('glade-pixmapdir')
  if glade_pixmapdir == ''
    glade_pixmapdir = glade.get_variable(pkgconfig: 'pixmapdir')
  endif
endif

gir = dependency('gobject-introspection-1.0', version: dependency_versions['gir'], required: get_option('introspection'))

vapigen = find_program('vapigen', required: get_option('vala'))
if get_option('vala').enabled()
  assert(get_option('introspection'), 'Building Vala bindings requires building GObject introspection data')
endif
generate_vala = not get_option('vala').disabled() and vapigen.found()

gnu_symbol_visibility = 'default'
if get_option('visibility')
  gnu_symbol_visibility = 'hidden'
  feature_cflags += '-DENABLE_SYMBOL_VISIBILITY=1'
endif

vendor_info = get_option('vendor-info')
if vendor_info != ''
  feature_cflags += '-DVENDOR_INFO="@0@"'.format(vendor_info)
endif

manual_website = get_option('manual-website')
if manual_website == ''
  manual_website = 'https://docs.xfce.org/help.php'
endif
feature_cflags += '-DMANUAL_WEBSITE="@0@"'.format(manual_website)

need_libintl = false
has_bind_textdomain_codeset = cc.has_function('bind_textdomain_codeset')
if not has_bind_textdomain_codeset
  libintl = cc.find_library('intl', required: false)
  if libintl.found()
    has_bind_textdomain_codeset = cc.has_function(
      'bind_textdomain_codeset',
      dependencies: [
        libintl,
      ],
    )
    if has_bind_textdomain_codeset
      need_libintl = true
    endif
  endif
endif
if has_bind_textdomain_codeset
  feature_cflags += '-DHAVE_BIND_TEXTDOMAIN_CODESET=1'
endif

if cc.has_header_symbol('unistd.h', 'environ')
  feature_cflags += '-DHAVE_DECL_ENVIRON=1'
else
  feature_cflags += '-DHAVE_DECL_ENVIRON=0'
endif
if cc.has_function('_NSGetEnviron')
  feature_cflags += '-DHAVE__NSGETENVIRON=1'
endif

functions = [
  'mmap',
  'realpath',
]
foreach function : functions
  if cc.has_function(function)
    feature_cflags += '-DHAVE_@0@=1'.format(function.to_upper())
  endif
endforeach

headers += [
  'sys/mman.h',
  'sys/stat.h',
  'sys/types.h',
  'crt_externs.h',
  'errno.h',
  'fcntl.h',
  'limits.h',
  'locale.h',
  'math.h',
  'memory.h',
  'mmintrin.h',
  'stdarg.h',
  'stdio.h',
  'stdlib.h',
  'string.h',
  'time.h',
  'unistd.h',
]
foreach header : headers
  if cc.check_header(header)
    feature_cflags += '-DHAVE_@0@=1'.format(header.underscorify().to_upper())
  endif
endforeach

libm = cc.find_library('m', required: true)

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DCOPYRIGHT_YEAR="@0@"'.format(copyright_year),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(extra_cflags, language: 'c')
add_project_arguments(feature_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

subdir('libxfce4ui')
subdir('docs' / 'reference')
subdir('glade')
subdir('icons')
subdir('libxfce4kbd-private')
subdir('po')
subdir('tests')
subdir('xfce4-about')
subdir('xfce-desktop-item-edit')
subdir('xfce-open')
