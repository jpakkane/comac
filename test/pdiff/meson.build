libpdiff_sources = [
  'lpyramid.c',
  'pdiff.c',
]

perceptualdiff_sources = [
  'args.c',
  'perceptualdiff.c',
]

libpdiff = static_library('pdiff', libpdiff_sources,
  include_directories: [incbase],
  dependencies: deps + [libcomac_dep],
)

libpdiff_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: [libpdiff],
)

perceptualdiff = executable('perceptualdiff', perceptualdiff_sources,
  include_directories: [incbase],
  dependencies: [libcomac_dep, libpdiff_dep],
)
