mapcairoCfg = configuration_data()
mapcairoCfg.set('HAVE_VISIBILITY',haveVisibility, description: 'compiler supports simple visibility declarations')
mapcairoCfg.set('OSMSCOUT_MAP_CAIRO_HAVE_LIB_PANGO',pangocairoDep.found(), description: 'text drawing using pango is supported')

configure_file(output: 'Config.h',
               configuration: mapcairoCfg)

