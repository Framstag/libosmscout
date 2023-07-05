subdir('include')
subdir('include/osmscoutmapiosx/private')
subdir('src')

osmscoutmapiosx = library('osmscout_map_iosx',
                           osmscoutmapiosxSrc,
                           include_directories: [osmscoutmapiosxIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           dependencies: [mathDep,
                                          threadDep,
                                          foundationDep,
                                          coreGraphicsDep,
                                          coreTextDep,
                                          cocoaDep,
                                          appKitDep],
                           link_with: [osmscoutmap, osmscout],
                           version: libraryVersion,
                           install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapiosx)
