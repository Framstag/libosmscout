cppArgs = ['-D_UNICODE']

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapDirectX_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapdirectx')
subdir('include/osmscoutmapdirectx/private')
subdir('src')

osmscoutmapdirectx = library('osmscout_map_directx',
                             osmscoutmapdirectxSrc,
                             include_directories: [osmscoutmapdirectxIncDir, osmscoutmapIncDir, osmscoutIncDir],
                             cpp_args: cppArgs,
                             dependencies: [mathDep, threadDep, d2d1Dep, dwriteDep, winCodecsDep],
                             link_with: [osmscoutmap, osmscout],
                             version: libraryVersion,
                             install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapdirectx)
