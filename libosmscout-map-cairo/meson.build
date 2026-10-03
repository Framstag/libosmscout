cppArgs = []

# The pango-less text path draws the face of a configured font file directly, when the cairo
# build offers the FreeType font backend
cairoHasFreeTypeFace = false

if cairoDep.found() and ftDep.found()
  cairoHasFreeTypeFace = compiler.has_function('cairo_ft_font_face_create_for_ft_face',
                                               prefix: '#include <cairo-ft.h>',
                                               dependencies: [cairoDep, ftDep])
endif

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
# FreeType and the font configuration are optional: they serve a configured
# font file that the text stack cannot be handed directly
osmscoutmapcairoDeps = [mathDep, threadDep, cairoDep, pangoDep, pangocairoDep, pngDep, gobjectDep]

if ftDep.found()
  osmscoutmapcairoDeps += [ftDep]
endif

if fontconfigDep.found()
  osmscoutmapcairoDeps += [fontconfigDep]
endif

osmscoutmapcairo = library('osmscout_map_cairo',
                           osmscoutmapcairoSrc,
                           include_directories: [osmscoutmapcairoIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           cpp_args: cppArgs,
                           dependencies: osmscoutmapcairoDeps,
                           link_with: [osmscoutmap, osmscout],
                           version: libraryVersion,
                           install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapcairo,
             name: 'libosmscout_map_cairo',
             filebase: 'libosmscout_map_cairo',
             description: 'Library for rendering OSM data using Cairo',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')
