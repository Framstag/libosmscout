cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapAGG_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapagg')
subdir('include/osmscoutmapagg/private')
subdir('src')

mapaggDep = [mathDep, threadDep, aggDep]

if aggftpicDep.found()
  mapaggDep += aggftpicDep
else
  mapaggDep += aggftDep
endif

mapaggDep += ftDep

osmscoutmapagg = library('osmscout_map_agg',
                           osmscoutmapaggSrc,
                           include_directories: [osmscoutmapaggIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           cpp_args: cppArgs,
                           dependencies: mapaggDep,
                           link_with: [osmscoutmap, osmscout],
                           version: libraryVersion,
                           install: true)

# TODO: Generate PKG_CONFIG file
