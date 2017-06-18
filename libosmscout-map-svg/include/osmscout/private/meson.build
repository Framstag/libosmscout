mapsvgCfg = configuration_data()
mapsvgCfg.set('HAVE_VISIBILITY',haveVisibility, description: 'compiler supports simple visibility declarations')
mapsvgCfg.set('OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO',pangoft2Dep.found(), description: 'Pango available')

configure_file(output: 'Config.h',
               configuration: mapsvgCfg)

