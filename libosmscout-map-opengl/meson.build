cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_MAP_OPENGL_EXPORT_SYMBOLS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscoutmapopengl = library('osmscout_map_opengl',
                            osmscoutmapopenglSrc,
                            include_directories: [osmscoutmapopenglIncDir, osmscoutmapIncDir, osmscoutIncDir],
                            cpp_args: cppArgs,
                            dependencies: [mathDep, threadDep, openGLDep, glutDep, glmDep, glewDep],
                            link_with: [osmscout, osmscoutmap],
                            install: true)
        
# TODO: Generate PKG_CONFIG file        
