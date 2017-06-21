cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_MAP_CAIRO_EXPORT_SYMBOLS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscoutmapcairo = library('osmscout_map_cairo',
                           osmscoutmapcairoSrc,
                           include_directories: [osmscoutmapcairoIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           cpp_args: cppArgs,
                           dependencies: [mathDep, threadDep, cairoDep, pangoDep, pangocairoDep, pngDep],
                           link_with: [osmscoutmap, osmscout],
                           install: true)
        
# TODO: Generate PKG_CONFIG file        
