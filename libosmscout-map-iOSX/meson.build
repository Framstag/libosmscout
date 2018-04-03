subdir('include')
subdir('include/osmscout/private')
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
                           install: true)

# TODO: Generate PKG_CONFIG file
