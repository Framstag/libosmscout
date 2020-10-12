cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapGDI_EXPORTS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscoutmapgdi = library('osmscout_map_gdi',
                         osmscoutmapgdiSrc,
                         include_directories: [osmscoutmapgdiIncDir, osmscoutmapIncDir, osmscoutIncDir],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep],
                         link_with: [osmscoutmap, osmscout],
                         install: true)
        
# TODO: Generate PKG_CONFIG file        
