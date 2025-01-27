if glade.found()
  shared_module(
    'xfce4uiglade2',
    'libxfce4ui-glade.c',
    sources: libxfce4ui_h,
    gnu_symbol_visibility: 'hidden',
    c_args: [
      '-DG_LOG_DOMAIN="@0@"'.format('libxfce4ui-glade'),
    ],
    include_directories: [
      include_directories('..'),
    ],
    dependencies: [
      glade,
      libxfce4util,
    ],
    link_with: [
      libxfce4ui,
    ],
    install: true,
    install_dir: glade_moduledir,
  )

  install_data('libxfce4ui-2.xml', install_dir: glade_catalogdir)

  sizes = [16, 22]
  foreach size : sizes
    install_data(
      'icons' / '@0@x@0@'.format(size) / 'widget-libxfce4ui-xfce-titled-dialog.png',
      install_dir: glade_pixmapdir / 'hicolor' / '@0@x@0@'.format(size) / 'actions'
    )
  endforeach
endif
