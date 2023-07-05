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

osmscoutmapqt = library('osmscout_map_qt',
                      osmscoutmapqtSrc,
                      include_directories: [osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                      cpp_args: cppArgs,
                      dependencies: [mathDep, threadDep, qtMapDep],
                      link_with: [osmscout, osmscoutmap],
                      version: libraryVersion,
                      install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapqt)
