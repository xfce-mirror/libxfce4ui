add_test_setup(
  'default',
  exclude_suites: 'gui',
  is_default: true,
)
add_test_setup(
  'full',
)

test_bins = [
]
test_gui_bins = [
  'test-ui',
]

foreach bin : test_bins + test_gui_bins
  e = executable(
    bin,
    '@0@.c'.format(bin),
    sources: libxfce4ui_h,
    include_directories: [
      include_directories('..'),
    ],
    dependencies: [
      gtk,
      libxfce4util,
      x11_deps,
    ],
    link_with: [
      libxfce4ui,
    ],
    install: false,
  )

  if bin in test_gui_bins
    test(bin, e, suite: 'gui')
  else
    test(bin, e)
  endif
endforeach
