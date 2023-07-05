cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMap_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmap')
subdir('include/osmscoutmap/private')
subdir('src')

osmscoutmap = library('osmscout_map',
                      osmscoutmapSrc,
                      include_directories: [osmscoutmapIncDir, osmscoutIncDir],
                      cpp_args: cppArgs,
                      dependencies: [mathDep, threadDep],
                      link_with: [osmscout],
                      version: libraryVersion,
                      install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmap)
