cairo_boilerplate_sources = [
  'comac-boilerplate-getopt.c',
  'comac-boilerplate-system.c',
  'comac-boilerplate.c',
]

cairo_boilerplate_feature_sources = {
  'comac-pdf': ['comac-boilerplate-pdf.c'],
  'comac-script': ['comac-boilerplate-script.c'],
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
