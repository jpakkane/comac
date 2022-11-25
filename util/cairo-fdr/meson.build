comac_fdr_sources = [
  'fdr.c',
]

libcomacfdr = library('comac-fdr', comac_fdr_sources,
  include_directories: [incbase, incsrc],
  dependencies: deps,
  install: true,
  install_dir: join_paths(get_option('prefix'), get_option('libdir'), 'comac'),
)
