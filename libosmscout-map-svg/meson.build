cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapSVG_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapsvg')
subdir('include/osmscoutmapsvg/private')
subdir('src')

osmscoutmapsvg = library('osmscout_map_svg',
                         osmscoutmapsvgSrc,
                         include_directories: [osmscoutmapsvgIncDir, osmscoutmapIncDir, osmscoutIncDir],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep, pangoft2Dep],
                         link_with: [osmscoutmap, osmscout],
                         version: libraryVersion,
                         install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapsvg)
