importFeaturesCfg = configuration_data()
importFeaturesCfg.set('OSMSCOUT_IMPORT_HAVE_PROTOBUF_SUPPORT',protobufDep.found() and protocCmd.found() and zlibDep.found(), description: '*osm.pbf files can be imported')
importFeaturesCfg.set('OSMSCOUT_IMPORT_HAVE_XML_SUPPORT',xml2Dep.found(), description: '*.osm can be imported')
importFeaturesCfg.set('OSMSCOUT_IMPORT_HAVE_LIB_MARISA',marisaDep.found(), description: 'libmarisa is available')
importFeaturesCfg.set('OSMSCOUT_IMPORT_MESON_BUILD',true, description: 'we are building using meson')
importFeaturesCfg.set('OSMSCOUT_DEBUG_COASTLINE',false, description: 'Extra debugging of coastline evaluation')
importFeaturesCfg.set('OSMSCOUT_DEBUG_TILING',false, description: 'Extra debugging of water index tiling')

configure_file(output: 'ImportFeatures.h',
               install_dir: 'include/osmscoutimport',
               configuration: importFeaturesCfg)

