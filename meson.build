project('comac', 'c', 'cpp',
  meson_version: '>= 0.64.0',
  version: '0.0.1',
  default_options: ['warning_level=2'],
)

# Keep in sync with configure.ac!
freetype_required_version = '>= 9.7.3'
fontconfig_required_version = '>= 2.2.95'
xrender_required_version = '>= 0.6'
xcb_required_version = '>= 1.6'
xcb_render_required_version = '>= 1.6'
libudev_required_version = '>= 136'
glib_required_version = '>= 2.14'

# library versioning
version_arr = meson.project_version().split('.')
comac_version_major = version_arr[0]
comac_version_minor = version_arr[1]
comac_version_micro = version_arr[2]

cver = configuration_data()
cver.set('COMAC_VERSION_MAJOR', comac_version_major)
cver.set('COMAC_VERSION_MINOR', comac_version_minor)
cver.set('COMAC_VERSION_MICRO', comac_version_micro)

conf = configuration_data()

cc = meson.get_compiler('c')

# Compiler flags
cflags = []
if cc.get_id() != 'msvc'
  cflags += [
    '-Wmissing-declarations',
    '-Werror-implicit-function-declaration',
    '-Wpointer-arith',
    '-Wwrite-strings',
    '-Wsign-compare',
    '-Wpacked',
    '-Wswitch-enum',
    '-Wmissing-format-attribute',
    '-Wvolatile-register-var',
    '-Wstrict-aliasing=2',
    '-Winit-self',
    '-Wunsafe-loop-optimizations',
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameter',
    '-Wno-attributes',
    '-Wno-long-long',
    '-Winline'
  ]

  cflags += ['-Wno-unused-but-set-variable',
             '-Wno-enum-conversion'
  ]

  cflags += [
    '-fno-strict-aliasing',
    '-fno-common'
  ]

  if get_option('optimization') in ['1', '2', '3']
    cflags += '-Wp,-D_FORTIFY_SOURCE=2'
  endif

  supported_cflags = cc.get_supported_arguments(cflags)
  add_project_arguments(supported_cflags, language: 'c')

  # We only wish to enable attribute(warn_unused_result) if we can prevent
  # gcc from generating thousands of warnings about the misapplication of the
  # attribute to void functions and variables.
  warn_unused_result = ''
  if supported_cflags.contains('-Wno-attributes')
    if cc.compiles(files('meson-cc-tests/check-unused-result.c'), args : ['-Wno-attributes', '-Werror'])
      warn_unused_result = '__attribute__((__warn_unused_result__))'
    endif
  endif
  conf.set('WARN_UNUSED_RESULT', warn_unused_result)
endif

if cc.get_id() == 'msvc'
  # Basic usage in the comac type system that causes spammy and useless warnings
  add_project_arguments('/wd4244', '/wd4146',
                        # Don't warn about double -> float truncation
                        '/wd4305',
                        language : 'c')
endif

add_project_arguments('-D_GNU_SOURCE', language: 'c')

# Autotools compatibility
add_project_arguments('-DHAVE_CONFIG_H', language: 'c')

# Make sure source directory hasn't been configured with autotools
fs = import('fs')
if fs.exists('config.h') or fs.exists('src/comac-features.h') or fs.exists('src/comac-supported-features.h')
  error('''
        The source directory '@0@' appears to contain
        autotools configuration artifacts. This can cause difficult to
        debug build problems. Please clean it up and then re-run meson.
        '''.format(meson.source_root()))
endif

pkgmod = import('pkgconfig')

check_sizeofs = [
  ['void *', {'conf-name': 'SIZEOF_VOID_P'}],
  ['int'],
  ['long'],
  ['long long'],
  ['size_t'],
]

check_headers = [
  ['stdint.h'],
  ['inttypes.h'],
  ['sys/int_types.h'],
  ['fcntl.h'],
  ['unistd.h'],
  ['signal.h'],
  ['sys/stat.h'],
  ['sys/socket.h'],
  ['poll.h'],
  ['sys/poll.h'],
  ['sys/un.h'],
  ['sched.h', {'check-funcs': ['sched_getaffinity']}],
  ['sys/mman.h', {'check-funcs': ['mmap']}],
  ['time.h', {'check-funcs': ['clock_gettime']}],
  ['libgen.h'],
  ['byteswap.h'],
  ['signal.h'],
  ['setjmp.h'],
  ['fenv.h'],
  ['sys/wait.h'],
  ['sys/stat.h'],
  ['io.h'],
  ['fenv.h', {'check-funcs': ['feenableexcept', 'fedisableexcept', 'feclearexcept']}],
  ['xlocale.h'],
  ['sys/ioctl.h'],
  ['intsafe.h'],
]

check_types = [
  ['uint64_t', {'headers': ['stdint.h']}],
  ['uint128_t', {'headers': ['stdint.h']}],
  ['__uint128_t']
]

check_funcs = [
  'alarm',
  'ctime_r',
  'localtime_r',
  'gmtime_r',
  'drand48',
  'flockfile',
  'funlockfile',
  'getline',
  'link',
  'fork',
  'waitpid',
  'raise',
  'newlocale',
  'strtod_l',
]

m_dep = cc.find_library('m', required: false)
thread_dep = dependency('threads')
conf.set('COMAC_HAS_PTHREAD', 1)
conf.set('COMAC_HAS_REAL_PTHREAD', 1)

deps = [m_dep, thread_dep]
test_deps = []
internal_deps = []
extra_link_args = []

if host_machine.endian() == 'big'
  conf.set('WORDS_BIGENDIAN', 1)
endif

float_order = cc.get_define('__FLOAT_WORD_ORDER__')
if float_order != ''
  if float_order == cc.get_define('__ORDER_BIG_ENDIAN__')
    conf.set('FLOAT_WORDS_BIGENDIAN', 1)
  endif
else
  # Assume same as platform endian
  if host_machine.endian() == 'big'
    conf.set('FLOAT_WORDS_BIGENDIAN', 1)
  endif
endif

lzo_dep = dependency('lzo2', required: false)
if lzo_dep.found()
  conf.set('HAVE_LZO', 1)
endif

dl_dep = cc.find_library('dl', required: false)
if dl_dep.found() and cc.has_function('dlsym', dependencies: [dl_dep])
  deps += [dl_dep]
  conf.set('COMAC_HAS_DLSYM', 1)
elif cc.has_function('dlsym')
  conf.set('COMAC_HAS_DLSYM', 1)
elif cc.has_function('dlsym', prefix: '#include <dlfcn.h>')
  conf.set('COMAC_HAS_DLSYM', 1)
endif

feature_conf = configuration_data()

# Array of dictionaries, used to generate per-feature pc files
# Mandatory keys: name, description
# Optional keys: requires, libs
built_features = []

zlib_dep = dependency('zlib',
  required: get_option('zlib'),
  fallback : ['zlib', 'zlib_dep'],
)
if zlib_dep.found()
  if zlib_dep.type_name() == 'internal'
    internal_deps += [zlib_dep]
  else
    deps += [zlib_dep]
  endif
  conf.set('HAVE_ZLIB', 1)
endif

png_dep = dependency('libpng',
  required: get_option('png'),
  fallback: ['libpng', 'libpng_dep']
)
if png_dep.found()
  feature_conf.set('COMAC_HAS_PNG_FUNCTIONS', 1)
  built_features += [
    {
      'name': 'comac-png',
      'description': 'PNG functions',
      'deps': [png_dep],
    },
    {
      'name': 'comac-svg',
      'description': 'SVG surface backend',
      'deps': [png_dep],
    }
  ]

  if png_dep.type_name() == 'internal'
    internal_deps += [png_dep]
  else
    deps += [png_dep]
  endif
endif

# Don't build fontconfig as a subproject on Windows unless
# explicitly requested
fontconfig_option = get_option('fontconfig')
if host_machine.system() == 'windows' and not fontconfig_option.enabled()
  fontconfig_option = false
endif

fontconfig_dep = dependency('fontconfig',
  required: fontconfig_option,
  version: fontconfig_required_version,
  fallback: ['fontconfig', 'fontconfig_dep'],
)
if fontconfig_dep.found()
  fc_check_funcs = [
    'FcInit',
    'FcFini'
  ]

  if fontconfig_dep.type_name() == 'internal'
    foreach func : fc_check_funcs
      conf.set('HAVE_@0@'.format(func.to_upper()), 1)
    endforeach
    internal_deps += [fontconfig_dep]
  else
    check_funcs += fc_check_funcs
    deps += [fontconfig_dep]
  endif

  feature_conf.set('COMAC_HAS_FC_FONT', 1)
  built_features += [{
    'name': 'comac-fc',
    'description': 'Fontconfig font backend',
    'deps': [fontconfig_dep],
  }]
endif

freetype_dep = dependency('freetype2',
  required: get_option('freetype'),
  version: freetype_required_version,
  fallback: ['freetype2', 'freetype_dep'],
)
if freetype_dep.found()
  feature_conf.set('COMAC_HAS_FT_FONT', 1)
  built_features += [{
    'name': 'comac-ft',
    'description': 'Freetype font backend',
    'deps': [freetype_dep],
    # comac-ft.h includes fontconfig.h so it needs its cflags
    'compile-deps': [fontconfig_dep.partial_dependency(compile_args: true, includes: true)],
  }]

  ft_check_funcs = [
    'FT_Get_X11_Font_Format',
    'FT_GlyphSlot_Embolden',
    'FT_GlyphSlot_Oblique',
    'FT_Load_Sfnt_Table',
    'FT_Library_SetLcdFilter',
    'FT_Get_Var_Design_Coordinates',
    'FT_Done_MM_Var',
    'FT_Palette_Select',
  ]

  if freetype_dep.type_name() == 'internal'
    foreach func : ft_check_funcs
      conf.set('HAVE_@0@'.format(func.to_upper()), 1)
    endforeach
    internal_deps += [freetype_dep]
  else
    if not cc.links(files('meson-cc-tests/ft_has_color.c'), dependencies: freetype_dep, name: 'FT has color')
      conf.set('FT_HAS_COLOR', '(0)')
    endif
    check_funcs += ft_check_funcs
    deps += [freetype_dep]
  endif
endif

if zlib_dep.found()
  feature_conf.set('COMAC_HAS_SCRIPT_SURFACE', 1)
  built_features += [{
    'name': 'comac-script',
    'description': 'script surface backend',
    'deps': [zlib_dep],
  }]
endif

if zlib_dep.found()
  feature_conf.set('COMAC_HAS_PDF_SURFACE', 1)
  built_features += [{
    'name': 'comac-pdf',
    'description': 'PDF surface backend',
    'deps': [zlib_dep],
  }]
endif

if zlib_dep.found()
  conf.set('COMAC_HAS_INTERPRETER', 1)
endif

if zlib_dep.found() and png_dep.found() and get_option('xml').enabled()
  feature_conf.set('COMAC_HAS_XML_SURFACE', 1)
  built_features += [{
    'name': 'comac-xml',
    'description': 'XML surface backend',
    'deps': [zlib_dep],
  }]
endif

bfd_dep = cc.find_library('bfd', has_headers: ['bfd.h'], required: get_option('symbol-lookup'))
if bfd_dep.found() and \
   cc.has_function('bfd_openr', dependencies: [bfd_dep]) and \
   cc.links(files('meson-cc-tests/bfd-section-flags.c'), name: 'bfd_section_flags', dependencies: bfd_dep)
  conf.set('HAVE_BFD', 1)
  deps += [bfd_dep]
elif get_option('symbol-lookup').enabled()
  error('symbol lookup via bfd was enabled via options but is not available (bfd might be too old)')
endif

if conf.get('HAVE_BFD', 0) == 1
  conf.set('COMAC_HAS_SYMBOL_LOOKUP', 1)
endif

if feature_conf.get('COMAC_HAS_PDF_SURFACE', 0) == 1
  poppler_dep = dependency('poppler-glib', version: '>= 0.17.4',
                           required: get_option('tests'))
  if poppler_dep.found() and cc.has_function('poppler_page_render', dependencies: [poppler_dep])
    conf.set('COMAC_CAN_TEST_PDF_SURFACE', 1)
    test_deps += [poppler_dep]
  endif
endif

pixman_dep = dependency('pixman-1',
  version: '>= 0.36.0',
  fallback: ['pixman', 'idep_pixman'],
)
if pixman_dep.found()
  feature_conf.set('COMAC_HAS_IMAGE_SURFACE', 1)
  conf.set('HAS_PIXMAN_GLYPHS', 1)
  if pixman_dep.type_name() == 'internal'
    internal_deps += [pixman_dep]
  else
    deps += [pixman_dep]
  endif
endif

feature_conf.set('COMAC_FEATURES_H', true)
feature_conf.set('COMAC_HAS_USER_FONT', 1)

feature_conf.set('COMAC_HAS_MIME_SURFACE', 1)
feature_conf.set('COMAC_HAS_RECORDING_SURFACE', 1)
feature_conf.set('COMAC_HAS_OBSERVER_SURFACE', 1)

if not get_option('tee').disabled()
  feature_conf.set('COMAC_HAS_TEE_SURFACE', 1)
  built_features += [{
    'name': 'comac-tee',
    'description': 'Tee surface backend',
  }]
endif

incbase = include_directories('.')

foreach check : check_sizeofs
  type = check[0]
  opts = check.length() > 1 ? check[1] : {}

  conf_name = opts.get('conf-name', 'SIZEOF_@0@'.format(type.underscorify().to_upper()))

  conf.set(conf_name, cc.sizeof(type))
endforeach

foreach check : check_headers
  name = check[0]
  opts = check.length() > 1 ? check[1] : {}
  prefix = ''

  foreach header : opts.get('extra-headers', [])
    prefix += '#include <@0@>\n'.format(header)
  endforeach

  if cc.has_header(name, prefix: prefix)
    conf.set('HAVE_@0@'.format(name.to_upper().underscorify()), 1)
    check_funcs += check.length() > 1 ? check[1].get('check-funcs', []) : []
  endif
endforeach

foreach check : check_types
  name = check[0]
  opts = check.length() > 1 ? check[1] : {}
  prefix = ''

  foreach header : opts.get('headers', [])
    prefix += '#include <@0@>\n'.format(header)
  endforeach

  if cc.has_type(name, prefix: prefix)
    conf.set('HAVE_@0@'.format(name.to_upper()), 1)
  endif
endforeach

foreach name : check_funcs
  if cc.has_function(name, dependencies: deps)
    conf.set('HAVE_@0@'.format(name.to_upper()), 1)
  endif
endforeach

conf.set('HAVE_STRNDUP', cc.has_function('strndup', prefix : '#include <string.h>'))

if cc.links(files('meson-cc-tests/atomic-ops-cxx11.c'), name: 'Atomic ops: cxx11')
  conf.set('HAVE_CXX11_ATOMIC_PRIMITIVES', 1)
elif cc.links(files('meson-cc-tests/atomic-ops-gcc-legacy.c'), name: 'Atomic ops: gcc legacy')
  conf.set('HAVE_GCC_LEGACY_ATOMICS', 1)
elif cc.has_header('atomic_ops.h')
  conf.set('HAVE_LIB_ATOMIC_OPS', 1)
elif cc.has_header('libkern/OSAtomic.h')
  conf.set('HAVE_OS_ATOMIC_OPS', 1)
endif

test_mkdir_c_args = []
if conf.get('HAVE_SYS_STAT_H', 0) == 1
  test_mkdir_c_args += ['-DHAVE_SYS_STAT_H']
endif

if conf.get('HAVE_IO_H', 0) == 1
  test_mkdir_c_args += ['-DHAVE_IO_H']
endif

if cc.links(files('meson-cc-tests/mkdir-variant-1.c'), args: test_mkdir_c_args)
  conf.set('HAVE_MKDIR', 1)
elif cc.links(files('meson-cc-tests/mkdir-variant-2.c'), args: test_mkdir_c_args)
  conf.set('HAVE_MKDIR', 2)
else
  conf.set('HAVE_MKDIR', 0)
endif

if not ['x86', 'x86_64'].contains(host_machine.cpu_family())
  conf.set('ATOMIC_OP_NEEDS_MEMORY_BARRIER', 1)
endif

have_ld_preload = ['linux', 'freebsd', 'darwin', 'dragonfly'].contains(host_machine.system())

if have_ld_preload and zlib_dep.found() and conf.get('COMAC_HAS_REAL_PTHREAD', 0) == 1 and conf.get('COMAC_HAS_DLSYM', 0) == 1
  conf.set('COMAC_HAS_TRACE', 1)
endif

rt_dep = cc.find_library('rt', required: false)
have_shm = false
if rt_dep.found() and cc.has_function('shm_open', dependencies: [rt_dep])
  have_shm = true
endif

# This to make sure we don't run checks against internal deps
deps += internal_deps

subdir('src')

if feature_conf.get('COMAC_HAS_PNG_FUNCTIONS', 0) == 1
  subdir('boilerplate')
else
  comacboilerplate_dep = dependency('', required: false)
endif

subdir('util')

if not get_option('tests').disabled() and feature_conf.get('COMAC_HAS_PNG_FUNCTIONS', 0) == 1
  subdir('test')
  subdir('perf')
endif

configure_file(output: 'config.h', configuration: conf)

foreach feature: built_features
  feature_deps = feature.get('deps', [])
  feature_libs = feature.get('libs', [])
  feature_compile_deps = feature.get('compile-deps', [])
  pkgmod.generate(libraries: [libcomac, feature_deps, feature_libs],
    name: feature['name'],
    description: feature['description'] + ' for comac graphics library',
  )
  meson.override_dependency(feature['name'],
    declare_dependency(dependencies: [libcomac_dep, feature_deps, feature_compile_deps],
      link_args: feature_libs,
    )
  )
endforeach

# summary
summary({
        'Image': true,
        'Recording': true,
        'Observer': true,
        'Mime': true,
        'Tee': feature_conf.get('COMAC_HAS_TEE_SURFACE', 0) == 1,
        'XML': feature_conf.get('COMAC_HAS_XML_SURFACE', 0) == 1,
        'Xlib': feature_conf.get('COMAC_HAS_XLIB_SURFACE', 0) == 1,
        'Xlib Xrender': feature_conf.get('COMAC_HAS_XLIB_XRENDER_SURFACE', 0) == 1,
        'XCB': feature_conf.get('COMAC_HAS_XCB_SURFACE', 0) == 1,
        'CairoScript': feature_conf.get('COMAC_HAS_SCRIPT_SURFACE', 0) == 1,
        'PDF':  feature_conf.get('COMAC_HAS_PDF_SURFACE', 0) == 1,
        'OpenGL': feature_conf.get('COMAC_HAS_GL_SURFACE', 0) == 1,
        'OpenGL ES 2.0': feature_conf.get('COMAC_HAS_GLESV2_SURFACE', 0) == 1,
        'OpenGL ES 3.0': feature_conf.get('COMAC_HAS_GLESV3_SURFACE', 0) == 1,
        }, section: 'Surface Backends', bool_yn: true)

summary({
        'User': true,
        'FreeType': feature_conf.get('COMAC_HAS_FT_FONT', 0) == 1,
        'Fontconfig': feature_conf.get('COMAC_HAS_FC_FONT', 0) == 1,
        }, section: 'Font Backends', bool_yn: true)

summary({
        'PNG functions': feature_conf.get('COMAC_HAS_PNG_FUNCTIONS', 0) == 1,
        }, section: 'Functions', bool_yn: true)

summary({
        'comac-trace:': conf.get('COMAC_HAS_TRACE', 0) == 1,
        'comac-script-interpreter': conf.get('COMAC_HAS_INTERPRETER', 0) == 1,
        'API reference': get_option('gtk_doc'),
        }, section: 'Features and Utilities', bool_yn: true)
