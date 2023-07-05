cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapCairo_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutmapcairo')
subdir('include/osmscoutmapcairo/private')
subdir('src')

# TODO: We either have only cairo or the complete pango/glib stack...
osmscoutmapcairo = library('osmscout_map_cairo',
                           osmscoutmapcairoSrc,
                           include_directories: [osmscoutmapcairoIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           cpp_args: cppArgs,
                           dependencies: [mathDep, threadDep, cairoDep, pangoDep, pangocairoDep, pngDep, gobjectDep],
                           link_with: [osmscoutmap, osmscout],
                           version: libraryVersion,
                           install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapcairo)
