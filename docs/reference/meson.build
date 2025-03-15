if get_option('gtk-doc')
  libxfce4ui_doc_name = 'libxfce4ui'

  conf_data = configuration_data()
  conf_data.set_quoted('PACKAGE_NAME', libxfce4ui_doc_name)
  conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
  conf_data.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(libxfce4ui_doc_name, meson.project_version()))

  configure_file(
    input: 'gtkdocentities.ent.in',
    output: 'gtkdocentities.ent',
    configuration: conf_data,
  )

  ignored_headers = [
    'libxfce4ui-visibility.h',
    'xfce-icon-chooser-model.h',
    'xfce-thumbnail-preview.h',
    'xfce-thumbnail.h',
  ]
  if not enable_x11
    ignored_headers += [
      'xfce-clipboard-manager.h',
      'xfce-sm-client.h',
    ]
  endif

  configure_file(
    configuration: configuration_data({
      'COPYRIGHT_YEAR': copyright_year,
    }),
    input: 'libxfce4ui-docs.xml.in',
    output: 'libxfce4ui-docs.xml',
    install: false,
  )

  gnome.gtkdoc(
    libxfce4ui_doc_name,
    src_dir: [
      'libxfce4ui',
    ],
    ignore_headers: ignored_headers,
    main_xml: meson.project_build_root() / 'docs' / 'reference' / 'libxfce4ui-docs.xml',
    dependencies: [
      libxfce4ui_dep,
    ],
    gobject_typesfile: 'libxfce4ui.types',
    install: true,
    check: false,
    scan_args: '--rebuild-types',
  )
endif
