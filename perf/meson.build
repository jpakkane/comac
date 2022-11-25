incmicro = include_directories('.')

subdir('micro')

libcomacperf = static_library('comacperf',
  [
    'comac-perf.c',
    'comac-perf-report.c',
    'comac-stats.c',
    '../src/comac-time.c',
    'comac-perf.h',
    'comac-stats.h',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [pixman_dep, libcomacmissing_dep, comacboilerplate_dep],
)
libcomacperf_dep = declare_dependency(
  link_with: libcomacperf,
)


analyse_trace = executable('comac-analyse-trace',
  [
    'comac-analyse-trace.c',
    '../src/comac-error.c',
  ],
  include_directories: [incbase],
  dependencies: [pixman_dep, fontconfig_dep, libcomac_dep, comacboilerplate_dep,
    libcomacscript_dep, libcomacmissing_dep],
)

perf_trace = executable('comac-perf-trace',
  [
    'comac-perf-trace.c',
    '../src/comac-error.c',
    '../src/comac-hash.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [pixman_dep, fontconfig_dep, libcomacperf_dep, libcomacscript_dep,
    comacboilerplate_dep, libcomacmissing_dep],
)

micro = executable('comac-perf-micro',
  [ 'comac-perf-micro.c', ],
  include_directories: [incbase, incsrc],
  dependencies: [fontconfig_dep, libcomacperf_dep, libcomacperfmicro_dep,
    comacboilerplate_dep, libcomacmissing_dep],
)

diff_files = executable('comac-perf-diff-files',
  [
    'comac-perf-diff-files.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [libcomacperf_dep, comacboilerplate_dep],
)

print = executable('comac-perf-print',
  [
    'comac-perf-print.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [libcomacperf_dep, comacboilerplate_dep],
)

chart = executable('comac-perf-chart',
  [
    'comac-perf-chart.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [libcomacperf_dep, comacboilerplate_dep],
)

compare_backends = executable('comac-perf-compare-backends',
  [
    'comac-perf-compare-backends.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [libcomacperf_dep, comacboilerplate_dep],
)
