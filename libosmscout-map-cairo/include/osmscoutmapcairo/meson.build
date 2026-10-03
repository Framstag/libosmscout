mapcairoFeaturesCfg = configuration_data()

mapcairoFeaturesCfg.set('OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO',pangoDep.found() and pangocairoDep.found(), description: 'text drawing using pango is supported')
mapcairoFeaturesCfg.set('OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE',ftDep.found(), description: 'the configured font file is loaded to draw and measure its face directly')
mapcairoFeaturesCfg.set('OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG',fontconfigDep.found(), description: 'a configured font file is registered with the font configuration where the text stack cannot be handed one')

configure_file(output: 'MapCairoFeatures.h',
               install_dir: 'include/osmscoutmapcairo',
               configuration: mapcairoFeaturesCfg)

