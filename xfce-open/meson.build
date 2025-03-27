executable(
  'xfce-open',
  [
    'main.c',
  ],
  sources: [
    libxfce4ui_h,
    xfce_revision_h,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce-open'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gio,
    gio_unix,
    gtk,
    libxfce4util,
  ],
  link_with: [
    libxfce4ui,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)
