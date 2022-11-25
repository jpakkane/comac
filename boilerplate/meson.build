comac_boilerplate_sources = [
  'comac-boilerplate-getopt.c',
  'comac-boilerplate-system.c',
  'comac-boilerplate.c',
]

comac_boilerplate_feature_sources = {
  'comac-pdf': ['comac-boilerplate-pdf.c'],
  'comac-script': ['comac-boilerplate-script.c'],
}

foreach feature: built_features
  source_key = feature.get('source-key', feature.get('name'))
  comac_boilerplate_sources += comac_boilerplate_feature_sources.get(source_key, [])
endforeach

comac_boilerplate_constructors = custom_target('comac-boilerplate-constructors.c',
  input: files(comac_boilerplate_sources),
  output: 'comac-boilerplate-constructors.c',
  command: [find_program('make-comac-boilerplate-constructors.py'), '@OUTPUT@', '@INPUT@'])

libcomacboilerplate = static_library('comacboilerplate', comac_boilerplate_sources + [comac_boilerplate_constructors],
  include_directories: [incbase],
  dependencies: deps + [libcomac_dep],
  install: false,
  build_by_default: false,
)

comacboilerplate_dep = declare_dependency(
  link_with: [libcomacboilerplate],
  include_directories: include_directories('.'),
)
