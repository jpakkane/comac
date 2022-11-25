configure_file(input: 'comac-version.h.in',
  output: 'comac-version.h',
  configuration: cver,
  install: true,
  install_dir: 'comac')

comac_sources = [
  'comac-analysis-surface.c',
  'comac-arc.c',
  'comac-array.c',
  'comac-atomic.c',
  'comac-base64-stream.c',
  'comac-base85-stream.c',
  'comac-bentley-ottmann-rectangular.c',
  'comac-bentley-ottmann-rectilinear.c',
  'comac-bentley-ottmann.c',
  'comac-botor-scan-converter.c',
  'comac-boxes-intersect.c',
  'comac-boxes.c',
  'comac-cache.c',
  'comac-clip-boxes.c',
  'comac-clip-polygon.c',
  'comac-clip-region.c',
  'comac-clip-surface.c',
  'comac-clip-tor-scan-converter.c',
  'comac-clip.c',
  'comac-color.c',
  'comac-colormanagement.c',
  'comac-composite-rectangles.c',
  'comac-compositor.c',
  'comac-contour.c',
  'comac-damage.c',
  'comac-debug.c',
  'comac-default-context.c',
  'comac-device.c',
  'comac-error.c',
  'comac-fallback-compositor.c',
  'comac-fixed.c',
  'comac-font-face-twin-data.c',
  'comac-font-face-twin.c',
  'comac-font-face.c',
  'comac-font-options.c',
  'comac-freed-pool.c',
  'comac-freelist.c',
  'comac-gstate.c',
  'comac-hash.c',
  'comac-hull.c',
  'comac-image-compositor.c',
  'comac-image-info.c',
  'comac-image-source.c',
  'comac-image-surface.c',
  'comac-line.c',
  'comac-lzw.c',
  'comac-mask-compositor.c',
  'comac-matrix.c',
  'comac-mempool.c',
  'comac-mesh-pattern-rasterizer.c',
  'comac-misc.c',
  'comac-mono-scan-converter.c',
  'comac-mutex.c',
  'comac-no-compositor.c',
  'comac-observer.c',
  'comac-output-stream.c',
  'comac-paginated-surface.c',
  'comac-path-bounds.c',
  'comac-path-fill.c',
  'comac-path-fixed.c',
  'comac-path-in-fill.c',
  'comac-path-stroke-boxes.c',
  'comac-path-stroke-polygon.c',
  'comac-path-stroke-traps.c',
  'comac-path-stroke-tristrip.c',
  'comac-path-stroke.c',
  'comac-path.c',
  'comac-pattern.c',
  'comac-pen.c',
  'comac-polygon-intersect.c',
  'comac-polygon-reduce.c',
  'comac-polygon.c',
  'comac-raster-source-pattern.c',
  'comac-recording-surface.c',
  'comac-rectangle.c',
  'comac-rectangular-scan-converter.c',
  'comac-region.c',
  'comac-rtree.c',
  'comac-scaled-font.c',
  'comac-shape-mask-compositor.c',
  'comac-slope.c',
  'comac-spans-compositor.c',
  'comac-spans.c',
  'comac-spline.c',
  'comac-stroke-dash.c',
  'comac-stroke-style.c',
  'comac-surface-clipper.c',
  'comac-surface-fallback.c',
  'comac-surface-observer.c',
  'comac-surface-offset.c',
  'comac-surface-snapshot.c',
  'comac-surface-subsurface.c',
  'comac-surface-wrapper.c',
  'comac-surface.c',
  'comac-time.c',
  'comac-tor-scan-converter.c',
  'comac-tor22-scan-converter.c',
  'comac-toy-font-face.c',
  'comac-traps-compositor.c',
  'comac-traps.c',
  'comac-tristrip.c',
  'comac-unicode.c',
  'comac-user-font.c',
  'comac-version.c',
  'comac-wideint.c',
  'comac.c',
  'comac-cff-subset.c',
  'comac-scaled-font-subsets.c',
  'comac-truetype-subset.c',
  'comac-type1-fallback.c',
  'comac-type1-glyph-names.c',
  'comac-type1-subset.c',
  'comac-type3-glyph-surface.c',
  'comac-pdf-operators.c',
  'comac-pdf-shading.c',
  'comac-tag-attributes.c',
  'comac-tag-stack.c',
  'comac-deflate-stream.c',
]

comac_headers = [
  'comac.h',
  'comac-deprecated.h',
]

comac_feature_sources = {
  'comac-png': [
    'comac-png.c',
  ],
  'comac-ft': [
    'comac-ft-font.c',
  ],
  'comac-script': [
    'comac-script-surface.c',
  ],
  'comac-pdf': [
    'comac-pdf-surface.c',
    'comac-pdf-interchange.c',
  ],
  'comac-xml': [
    'comac-xml-surface.c',
  ],
  'comac-tee': [
    'comac-tee-surface.c',
  ],
}

comac_feature_headers = {
  'comac-pdf': ['comac-pdf.h'],
  'comac-ft': ['comac-ft.h'],
  'comac-script': ['comac-script.h'],
  'comac-tee': ['comac-tee.h'],
  'comac-xml': ['comac-xml.h'],
}

comac_no_warn_c_args = cc.get_supported_arguments([
  '-Wno-attributes',
  '-Wno-unused-but-set-variable',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-Wno-long-long',
])

foreach feature: built_features
  source_key = feature.get('source-key', feature.get('name'))
  comac_sources += comac_feature_sources.get(source_key, [])
  comac_headers += comac_feature_headers.get(source_key, [])
endforeach

incsrc = include_directories('.')

comac_static_args = []
if get_option('default_library') == 'static' and host_machine.system() == 'windows'
  comac_static_args += ['-DCAIRO_WIN32_STATIC_BUILD']
  add_project_arguments('-DCAIRO_WIN32_STATIC_BUILD', language: 'c')
endif

# Since this library is not meant for public consumption yet,
# only build a static library.
libcomac = static_library('comac', comac_sources,
  dependencies: deps,
  c_args: comac_no_warn_c_args,
  cpp_args: comac_no_warn_c_args,
  link_args: extra_link_args,
  install: true,
  include_directories: incbase,
)

comac_headers += [configure_file(output: 'comac-features.h', configuration: feature_conf)]

libcomac_dep = declare_dependency(link_with: libcomac,
  dependencies: deps,
  include_directories: incsrc,
  compile_args: comac_static_args,
)

pkgmod.generate(libcomac,
  description: 'Multi-platform 2D graphics library',
  subdirs: [meson.project_name()],
)

meson.override_dependency('comac', libcomac_dep)

install_headers(comac_headers, subdir: 'comac')
