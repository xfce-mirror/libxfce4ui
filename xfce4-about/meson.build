xfce4_about_sources = [
  'contributors.h',
  'main.c',
]

if libgtop.found()
  xfce4_about_sources += [
    'system-info.c',
    'system-info.h',
  ]
endif

xfce4_about_sources += custom_target(
  'about-dialog-ui.h',
  input: 'xfce4-about-dialog.glade',
  output: 'about-dialog-ui.h',
  command: [xdt_csource, '--static', '--strip-comments', '--strip-content', '--name=xfce_about_dialog_ui', '--output=@OUTPUT@', '@INPUT@'],
)

executable(
  'xfce4-about',
  xfce4_about_sources,
  sources: libxfce4ui_h,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-about'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gtk,
    libxfce4util,
    libgtop,
    epoxy,
    gudev,
  ],
  link_with: [
    libxfce4ui,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)

i18n.merge_file(
  input: 'xfce4-about.desktop.in',
  output: 'xfce4-about.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / 'applications',
)
