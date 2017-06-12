subdir('include')
subdir('include/osmscout/private')
subdir('src')

osmscoutmapiosx = library('osmscout-map-iosx',
                           osmscoutmapiosxSrc,
                           include_directories: [osmscoutmapiosxIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           dependencies: [mathDep, threadDep],
                           link_with: [osmscoutmap, osmscout],
                           install: true)
        
# TODO: Generate PKG_CONFIG file        
