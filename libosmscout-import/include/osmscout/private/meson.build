importCfg = configuration_data()
importCfg.set('HAVE_VISIBILITY',haveVisibility, description: 'compiler supports simple visibility declarations')
importCfg.set('HAVE_LIB_PROTOBUF',protobufDep.found() and protocCmd.found(), description: 'libprotobuf detected')
importCfg.set('HAVE_LIB_XML',xml2Dep.found(), description: 'libxml2 detected')
importCfg.set('HAVE_LIB_ZLIB',zlibDep.found(), description: 'zlib detected')
importCfg.set('OSMSCOUT_IMPORT_HAVE_LIB_MARISA',marisaDep.found(), description: 'libmarisa is available')

configure_file(output: 'Config.h',
               configuration: importCfg)

