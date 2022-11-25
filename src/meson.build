cairo_sources = [
  'cairo-analysis-surface.c',
  'cairo-arc.c',
  'cairo-array.c',
  'cairo-atomic.c',
  'cairo-base64-stream.c',
  'cairo-base85-stream.c',
  'cairo-bentley-ottmann-rectangular.c',
  'cairo-bentley-ottmann-rectilinear.c',
  'cairo-bentley-ottmann.c',
  'cairo-botor-scan-converter.c',
  'cairo-boxes-intersect.c',
  'cairo-boxes.c',
  'cairo-cache.c',
  'cairo-clip-boxes.c',
  'cairo-clip-polygon.c',
  'cairo-clip-region.c',
  'cairo-clip-surface.c',
  'cairo-clip-tor-scan-converter.c',
  'cairo-clip.c',
  'cairo-color.c',
  'cairo-composite-rectangles.c',
  'cairo-compositor.c',
  'cairo-contour.c',
  'cairo-damage.c',
  'cairo-debug.c',
  'cairo-default-context.c',
  'cairo-device.c',
  'cairo-error.c',
  'cairo-fallback-compositor.c',
  'cairo-fixed.c',
  'cairo-font-face-twin-data.c',
  'cairo-font-face-twin.c',
  'cairo-font-face.c',
  'cairo-font-options.c',
  'cairo-freed-pool.c',
  'cairo-freelist.c',
  'cairo-gstate.c',
  'cairo-hash.c',
  'cairo-hull.c',
  'cairo-image-compositor.c',
  'cairo-image-info.c',
  'cairo-image-source.c',
  'cairo-image-surface.c',
  'cairo-line.c',
  'cairo-lzw.c',
  'cairo-mask-compositor.c',
  'cairo-matrix.c',
  'cairo-mempool.c',
  'cairo-mesh-pattern-rasterizer.c',
  'cairo-misc.c',
  'cairo-mono-scan-converter.c',
  'cairo-mutex.c',
  'cairo-no-compositor.c',
  'cairo-observer.c',
  'cairo-output-stream.c',
  'cairo-paginated-surface.c',
  'cairo-path-bounds.c',
  'cairo-path-fill.c',
  'cairo-path-fixed.c',
  'cairo-path-in-fill.c',
  'cairo-path-stroke-boxes.c',
  'cairo-path-stroke-polygon.c',
  'cairo-path-stroke-traps.c',
  'cairo-path-stroke-tristrip.c',
  'cairo-path-stroke.c',
  'cairo-path.c',
  'cairo-pattern.c',
  'cairo-pen.c',
  'cairo-polygon-intersect.c',
  'cairo-polygon-reduce.c',
  'cairo-polygon.c',
  'cairo-raster-source-pattern.c',
  'cairo-recording-surface.c',
  'cairo-rectangle.c',
  'cairo-rectangular-scan-converter.c',
  'cairo-region.c',
  'cairo-rtree.c',
  'cairo-scaled-font.c',
  'cairo-shape-mask-compositor.c',
  'cairo-slope.c',
  'cairo-spans-compositor.c',
  'cairo-spans.c',
  'cairo-spline.c',
  'cairo-stroke-dash.c',
  'cairo-stroke-style.c',
  'cairo-surface-clipper.c',
  'cairo-surface-fallback.c',
  'cairo-surface-observer.c',
  'cairo-surface-offset.c',
  'cairo-surface-snapshot.c',
  'cairo-surface-subsurface.c',
  'cairo-surface-wrapper.c',
  'cairo-surface.c',
  'cairo-time.c',
  'cairo-tor-scan-converter.c',
  'cairo-tor22-scan-converter.c',
  'cairo-toy-font-face.c',
  'cairo-traps-compositor.c',
  'cairo-traps.c',
  'cairo-tristrip.c',
  'cairo-unicode.c',
  'cairo-user-font.c',
  'cairo-version.c',
  'cairo-wideint.c',
  'cairo.c',
  'cairo-cff-subset.c',
  'cairo-scaled-font-subsets.c',
  'cairo-truetype-subset.c',
  'cairo-type1-fallback.c',
  'cairo-type1-glyph-names.c',
  'cairo-type1-subset.c',
  'cairo-type3-glyph-surface.c',
  'cairo-pdf-operators.c',
  'cairo-pdf-shading.c',
  'cairo-tag-attributes.c',
  'cairo-tag-stack.c',
  'cairo-deflate-stream.c',
]

cairo_headers = [
  'cairo.h',
  'cairo-version.h',
  'cairo-deprecated.h',
]

cairo_feature_sources = {
  'cairo-png': [
    'cairo-png.c',
  ],
  'cairo-ft': [
    'cairo-ft-font.c',
  ],
  'cairo-script': [
    'cairo-script-surface.c',
  ],
  'cairo-pdf': [
    'cairo-pdf-surface.c',
    'cairo-pdf-interchange.c',
  ],
  'cairo-xml': [
    'cairo-xml-surface.c',
  ],
  'cairo-tee': [
    'cairo-tee-surface.c',
  ],
}

cairo_feature_headers = {
  'cairo-pdf': ['cairo-pdf.h'],
  'cairo-ft': ['cairo-ft.h'],
  'cairo-script': ['cairo-script.h'],
  'cairo-tee': ['cairo-tee.h'],
  'cairo-xml': ['cairo-xml.h'],
}

cairo_no_warn_c_args = cc.get_supported_arguments([
  '-Wno-attributes',
  '-Wno-unused-but-set-variable',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-Wno-long-long',
])

foreach feature: built_features
  source_key = feature.get('source-key', feature.get('name'))
  cairo_sources += cairo_feature_sources.get(source_key, [])
  cairo_headers += cairo_feature_headers.get(source_key, [])
endforeach

incsrc = include_directories('.')

cairo_static_args = []
if get_option('default_library') == 'static' and host_machine.system() == 'windows'
  cairo_static_args += ['-DCAIRO_WIN32_STATIC_BUILD']
  add_project_arguments('-DCAIRO_WIN32_STATIC_BUILD', language: 'c')
endif

# Since this library is not meant for public consumption yet,
# only build a static library.
libcairo = static_library('cairo', cairo_sources,
  dependencies: deps,
  c_args: cairo_no_warn_c_args,
  cpp_args: cairo_no_warn_c_args,
  link_args: extra_link_args,
  install: true,
  include_directories: incbase,
)

cairo_headers += [configure_file(output: 'cairo-features.h', configuration: feature_conf)]

libcairo_dep = declare_dependency(link_with: libcairo,
  dependencies: deps,
  include_directories: incsrc,
  compile_args: cairo_static_args,
)

pkgmod.generate(libcairo,
  description: 'Multi-platform 2D graphics library',
  subdirs: [meson.project_name()],
)

meson.override_dependency('cairo', libcairo_dep)

install_headers(cairo_headers, subdir: 'cairo')
