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

osmscoutmapcairo = library('osmscout-map-cairo',
                           osmscoutmapcairoSrc,
                           include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapcairoIncDir],
                           cpp_args: cppArgs,
                           dependencies: [mathDep, threadDep, cairoDep, pangoDep, pangocairoDep, pngDep],
                           link_with: [osmscout, osmscoutmap],
                           install: true)
        
# TODO: Generate PKG_CONFIG file        
