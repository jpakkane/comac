subdir('comac-missing')

comac_utils = []

if conf.get('COMAC_HAS_INTERPRETER', 0) == 1
  subdir('comac-script')
else
  libcomacscript_dep = dependency('', required: false)
endif

if conf.get('COMAC_HAS_TRACE', 0) == 1
  subdir('comac-trace')

  if (conf.get('COMAC_HAS_DLSYM', 0) == 1 and
      feature_conf.get('COMAC_HAS_SCRIPT_SURFACE', 0) == 1 and
      feature_conf.get('COMAC_HAS_TEE_SURFACE', 0) == 1)
    subdir('comac-fdr')
  endif
endif

if feature_conf.get('COMAC_HAS_XML_SURFACE', 0) == 1
  expat_dep = dependency('expat')
  comac_utils += [
    ['trace-to-xml.c', {'deps': [expat_dep]}],
    ['xml-to-trace.c', {'deps': [expat_dep]}],
  ]
endif

foreach util : comac_utils
  exe_name = util[0].split('.')[0]
  util_deps = util.get(1, {}).get('deps', [])
  executable(exe_name, util[0],
    include_directories: [incbase],
    dependencies: deps + util_deps + [libcomac_dep, libcomacscript_dep],
  )
endforeach

if conf.get('CAIRO_HAS_DLSYM', 0) == 1 and cc.has_header('execinfo.h')
  libmallocstats = library('malloc-stats', 'malloc-stats.c', dependencies : dl_dep)
endif
