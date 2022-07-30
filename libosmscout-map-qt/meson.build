cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapQt_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapqt')
subdir('include/osmscoutmapqt/private')
subdir('src')

if get_option('qtVersion') == 5
  osmscoutmapqt = library('osmscout_map_qt',
                          osmscoutmapqtSrc,
                          include_directories: [osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                          cpp_args: cppArgs,
                          dependencies: [mathDep, threadDep, qt5GuiDep, qt5SvgDep],
                          link_with: [osmscout, osmscoutmap],
                          install: true)
elif get_option('qtVersion') == 6
  osmscoutmapqt = library('osmscout_map_qt',
                          osmscoutmapqtSrc,
                          include_directories: [osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                          cpp_args: cppArgs,
                          dependencies: [mathDep, threadDep, qt6GuiDep, qt6SvgDep],
                          link_with: [osmscout, osmscoutmap],
                          install: true)
endif

# TODO: Generate PKG_CONFIG file
