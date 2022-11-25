comac_missing_sources = [
  'strndup.c',
  'getline.c',
]

libcomacmissing = static_library('comac-missing', comac_missing_sources,
  include_directories: [incbase, incsrc],
  install: false,
  build_by_default: false,
)

libcomacmissing_dep = declare_dependency(
  link_with: libcomacmissing,
  include_directories: include_directories('.'),
)
