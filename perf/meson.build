incmicro = include_directories('.')

subdir('micro')

libcairoperf = static_library('cairoperf',
  [
    'comac-perf.c',
    'comac-perf-report.c',
    'comac-stats.c',
    '../src/comac-time.c',
    'comac-perf.h',
    'comac-stats.h',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [pixman_dep, libcairomissing_dep, cairoboilerplate_dep],
)
libcairoperf_dep = declare_dependency(
  link_with: libcairoperf,
)


analyse_trace = executable('comac-analyse-trace',
  [
    'comac-analyse-trace.c',
    '../src/comac-error.c',
  ],
  include_directories: [incbase],
  dependencies: [pixman_dep, fontconfig_dep, libcairo_dep, cairoboilerplate_dep,
    libcairoscript_dep, libcairomissing_dep],
)

perf_trace = executable('comac-perf-trace',
  [
    'comac-perf-trace.c',
    '../src/comac-error.c',
    '../src/comac-hash.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [pixman_dep, fontconfig_dep, libcairoperf_dep, libcairoscript_dep,
    cairoboilerplate_dep, libcairomissing_dep],
)

micro = executable('comac-perf-micro',
  [ 'comac-perf-micro.c', ],
  include_directories: [incbase, incsrc],
  dependencies: [fontconfig_dep, libcairoperf_dep, libcairoperfmicro_dep,
    cairoboilerplate_dep, libcairomissing_dep],
)

diff_files = executable('comac-perf-diff-files',
  [
    'comac-perf-diff-files.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [libcairoperf_dep, cairoboilerplate_dep],
)

print = executable('comac-perf-print',
  [
    'comac-perf-print.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [libcairoperf_dep, cairoboilerplate_dep],
)

chart = executable('comac-perf-chart',
  [
    'comac-perf-chart.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [libcairoperf_dep, cairoboilerplate_dep],
)

compare_backends = executable('comac-perf-compare-backends',
  [
    'comac-perf-compare-backends.c',
  ],
  include_directories: [incbase, incsrc],
  dependencies: [libcairoperf_dep, cairoboilerplate_dep],
)
