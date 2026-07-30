cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapSkia_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapskia')
subdir('include/osmscoutmapskia/private')
subdir('src')

osmscoutmapskia = library('osmscout_map_skia',
                           osmscoutmapskiaSrc,
                           include_directories: [osmscoutmapskiaIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           cpp_args: cppArgs,
                           dependencies: [mathDep, threadDep, skiaDep],
                           link_with: [osmscoutmap, osmscout],
                           version: libraryVersion,
                           install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapskia,
             name: 'libosmscout_map_skia',
             filebase: 'libosmscout_map_skia',
             description: 'Library for rendering OSM data using the Skia graphics library',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')
