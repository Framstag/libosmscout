cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_MAP_SVG_EXPORT_SYMBOLS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscoutmapsvg = library('osmscout_map_svg',
                         osmscoutmapsvgSrc,
                         include_directories: [osmscoutmapsvgIncDir, osmscoutmapIncDir, osmscoutIncDir],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep, pangoft2Dep],
                         link_with: [osmscoutmap, osmscout],
                         install: true)
        
# TODO: Generate PKG_CONFIG file        
