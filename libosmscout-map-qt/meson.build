cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapQt_EXPORTS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscoutmapqt = library('osmscout_map_qt',
                        osmscoutmapqtSrc,
                        include_directories: [osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                        cpp_args: cppArgs,
                        dependencies: [mathDep, threadDep, qt5GuiDep, qt5SvgDep],
                        link_with: [osmscout, osmscoutmap],
                        install: true)
        
# TODO: Generate PKG_CONFIG file        
