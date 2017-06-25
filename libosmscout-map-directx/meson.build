cppArgs = ['-D_UNICODE']

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_MAP_DIRECTX_EXPORT_SYMBOLS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscoutmapdirectx = library('osmscout_map_directx',
                             osmscoutmapdirectxSrc,
                             include_directories: [osmscoutmapdirectxIncDir, osmscoutmapIncDir, osmscoutIncDir],
                             cpp_args: cppArgs,
                             dependencies: [mathDep, threadDep, d2d1Dep, dwriteDep, winCodecsDep],
                             link_with: [osmscoutmap, osmscout],
                             install: true)
        
# TODO: Generate PKG_CONFIG file        
