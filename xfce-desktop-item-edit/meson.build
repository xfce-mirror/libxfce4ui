executable(
  'xfce-desktop-item-edit',
  [
    'main.c',
    'xfce-die-command-entry.c',
    'xfce-die-command-entry.h',
    'xfce-die-command-model.c',
    'xfce-die-command-model.h',
    'xfce-die-desktop-model.c',
    'xfce-die-desktop-model.h',
    'xfce-die-editor.c',
    'xfce-die-editor.h',
    'xfce-die-enum-types.c',
    'xfce-die-enum-types.h',
    'xfce-die-utils.c',
    'xfce-die-utils.h',
  ],
  sources: [
    libxfce4ui_h,
    xfce_revision_h,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce-desktop-item-edit'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gio,
    gthread,
    gtk,
    libxfce4util,
  ],
  link_with: [
    libxfce4ui,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)
