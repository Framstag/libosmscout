cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapGDI_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapgdi')
subdir('include/osmscoutmapgdi/private')
subdir('src')

osmscoutmapgdi = library('osmscout_map_gdi',
                         osmscoutmapgdiSrc,
                         include_directories: [osmscoutmapgdiIncDir, osmscoutmapIncDir, osmscoutIncDir],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep, gdiplusDep],
                         link_with: [osmscoutmap, osmscout],
                         version: libraryVersion,
                         install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapgdi,
             name: 'libosmscout_map_gdi',
             filebase: 'libosmscout_map_gdi',
             description: 'Library for rendering OSM data using GDI',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')
