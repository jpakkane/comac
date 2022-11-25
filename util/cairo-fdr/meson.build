cairo_fdr_sources = [
  'fdr.c',
]

libcairofdr = library('comac-fdr', cairo_fdr_sources,
  include_directories: [incbase, incsrc],
  dependencies: deps,
  install: true,
  install_dir: join_paths(get_option('prefix'), get_option('libdir'), 'cairo'),
)
