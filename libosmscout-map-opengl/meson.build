cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapOpenGL_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapopengl')
subdir('include/osmscoutmapopengl/private')
subdir('src')

osmscoutmapopengl = library('osmscout_map_opengl',
                            osmscoutmapopenglSrc,
                            include_directories: [osmscoutmapopenglIncDir, osmscoutmapIncDir, osmscoutIncDir],
                            cpp_args: cppArgs,
                            dependencies: [mathDep, threadDep, openGLDep, glmDep, glewDep, pngDep, ftDep],
                            link_with: [osmscout, osmscoutmap],
                            version: libraryVersion,
                            install: true)

# TODO: Generate PKG_CONFIG file
