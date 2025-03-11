sizes = [16, 48, 128]
names = [
  'org.xfce.about',
  'xfce4-logo',
]

foreach name : names
  foreach size : sizes
    install_data(
      '@0@x@0@'.format(size) / '@0@.png'.format(name),
      install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
    )
  endforeach

  install_data(
    'scalable' / '@0@.svg'.format(name),
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
  )
endforeach

install_data(
  'pixmaps' / 'xfce-thumbnail-frame.png',
  install_dir: get_option('prefix') / get_option('datadir') / 'pixmaps' / 'libxfce4ui',
)
