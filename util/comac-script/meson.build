comacscript_interpreter_sources = [
  'comac-script-file.c',
  'comac-script-hash.c',
  'comac-script-interpreter.c',
  'comac-script-objects.c',
  'comac-script-operators.c',
  'comac-script-scanner.c',
  'comac-script-stack.c',
]

comacscript_interpreter_headers = [
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

libcomacscript = static_library('comac-script-interpreter',
  comacscript_interpreter_sources,
  include_directories: [incbase],
  dependencies: deps + [libcomac_dep, lzo_dep],
  install: true,
)

libcomacscript_dep = declare_dependency(link_with: libcomacscript,
  include_directories: include_directories('.'),
  dependencies: libcomac_dep)

pkgmod.generate(libcomacscript,
  libraries: [libcomac],
  description: 'script surface backend for comac graphics library',
  subdirs: [meson.project_name()],
)

meson.override_dependency('comac-script-interpreter', libcomacscript_dep)

csi_replay_exe = executable('csi-replay', csi_replay_sources,
  include_directories: [incbase],
  dependencies: deps + [libcomac_dep, libcomacscript_dep],
)

csi_exec_exe = executable('csi-exec', csi_exec_sources,
  include_directories: [incbase],
  dependencies: deps + [libcomac_dep, libcomacscript_dep],
)

if feature_conf.get('CAIRO_HAS_SCRIPT_SURFACE', 0) == 1 and conf.get('HAVE_LIBGEN_H', 0) == 1
  csi_trace_exe = executable('csi-trace', csi_trace_sources,
    include_directories: [incbase],
    dependencies: deps + [libcomac_dep, libcomacscript_dep],
  )
endif

install_headers(comacscript_interpreter_headers, subdir: 'comac')
