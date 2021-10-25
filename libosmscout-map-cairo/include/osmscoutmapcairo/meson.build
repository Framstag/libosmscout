mapcairoFeaturesCfg = configuration_data()

mapcairoFeaturesCfg.set('OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO',pangoDep.found() and pangocairoDep.found(), description: 'text drawing using pango is supported')

configure_file(output: 'MapCairoFeatures.h',
               configuration: mapcairoFeaturesCfg)

