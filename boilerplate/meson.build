cairo_boilerplate_sources = [
  'comac-boilerplate-getopt.c',
  'comac-boilerplate-system.c',
  'comac-boilerplate.c',
]

cairo_boilerplate_feature_sources = {
  'comac-xlib': ['comac-boilerplate-xlib.c'],
  'comac-quartz': ['comac-boilerplate-quartz.c'],
  'comac-xcb': ['comac-boilerplate-xcb.c'],
  'comac-win32': ['comac-boilerplate-win32.c', 'comac-boilerplate-win32-printing.c'],
  'comac-pdf': ['comac-boilerplate-pdf.c'],
  'comac-ps': ['comac-boilerplate-ps.c'],
  'comac-svg': ['comac-boilerplate-svg.c'],
  'comac-script': ['comac-boilerplate-script.c'],
  # All tests crash with FPE
  # 'comac-egl': ['comac-boilerplate-egl.c'],
  # comac-glx: ['comac-boilerplate-glx.c'],
  'comac-wgl': ['comac-boilerplate-wgl.c'],
}

foreach feature: built_features
  source_key = feature.get('source-key', feature.get('name'))
  cairo_boilerplate_sources += cairo_boilerplate_feature_sources.get(source_key, [])
endforeach

cairo_boilerplate_constructors = custom_target('comac-boilerplate-constructors.c',
  input: files(cairo_boilerplate_sources),
  output: 'comac-boilerplate-constructors.c',
  command: [python3, files('make-cairo-boilerplate-constructors.py')[0], '@OUTPUT@', '@INPUT@'])

libcairoboilerplate = static_library('cairoboilerplate', cairo_boilerplate_sources + [cairo_boilerplate_constructors],
  include_directories: [incbase],
  dependencies: deps + [libcairo_dep],
  install: false,
  build_by_default: false,
)

cairoboilerplate_dep = declare_dependency(
  link_with: [libcairoboilerplate],
  include_directories: include_directories('.'),
)
