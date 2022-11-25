libcomac_sphinx_sources = [
  'fdr.c',
]

comac_sphinx_sources = [
  'sphinx.c',
]

libcomacsphinx = library('comac-sphinx', libcomac_sphinx_sources,
  include_directories: [incbase, incsrc],
  dependencies: deps,
  install: true,
  install_dir: join_paths(get_option('prefix'), get_option('libdir'), 'comac'),
)

libdir = join_paths(get_option('prefix'), get_option('libdir'))

comacsphinx_exe = executable('comac-sphinx', comac_sphinx_sources,
  include_directories: [incbase],
  c_args: ['-DLIBDIR="@0@"'.format(libdir)] + pthread_c_args,
  dependencies: deps + [glib_dep, rt_dep, libcomac_dep, comacboilerplate_dep, libcomacscript_dep],
  link_with: [libcomacsphinx],
  link_args: extra_link_args,
  install: true,
)
