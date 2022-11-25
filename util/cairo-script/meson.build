cairoscript_interpreter_sources = [
  'comac-script-file.c',
  'comac-script-hash.c',
  'comac-script-interpreter.c',
  'comac-script-objects.c',
  'comac-script-operators.c',
  'comac-script-scanner.c',
  'comac-script-stack.c',
]

cairoscript_interpreter_headers = [
  'comac-script-interpreter.h',
]

csi_replay_sources = [
  'csi-replay.c',
]

csi_exec_sources = [
  'csi-exec.c',
]

csi_trace_sources = [
  'csi-trace.c',
]

libcairoscript = library('comac-script-interpreter',
  cairoscript_interpreter_sources,
  include_directories: [incbase],
  dependencies: deps + [libcairo_dep, lzo_dep],
  soversion: cairo_version_sonum,
  version: cairo_libversion,
  install: true,
)

libcairoscript_dep = declare_dependency(link_with: libcairoscript,
  include_directories: include_directories('.'),
  dependencies: libcairo_dep)

pkgmod.generate(libcairoscript,
  libraries: [libcairo],
  description: 'script surface backend for cairo graphics library',
  subdirs: [meson.project_name()],
)

meson.override_dependency('comac-script-interpreter', libcairoscript_dep)

csi_replay_exe = executable('csi-replay', csi_replay_sources,
  include_directories: [incbase],
  dependencies: deps + [libcairo_dep, libcairoscript_dep],
)

csi_exec_exe = executable('csi-exec', csi_exec_sources,
  include_directories: [incbase],
  dependencies: deps + [libcairo_dep, libcairoscript_dep],
)

if feature_conf.get('CAIRO_HAS_SCRIPT_SURFACE', 0) == 1 and conf.get('HAVE_LIBGEN_H', 0) == 1
  csi_trace_exe = executable('csi-trace', csi_trace_sources,
    include_directories: [incbase],
    dependencies: deps + [libcairo_dep, libcairoscript_dep],
  )
endif

install_headers(cairoscript_interpreter_headers, subdir: 'cairo')
