cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_MAP_EXPORT_SYMBOLS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscoutmap = library('osmscout_map',
                      osmscoutmapSrc,
                      include_directories: [osmscoutmapIncDir, osmscoutIncDir],
                      cpp_args: cppArgs,
                      dependencies: [mathDep, threadDep],
                      link_with: [osmscout],
                      install: true)
        
# TODO: Generate PKG_CONFIG file        
