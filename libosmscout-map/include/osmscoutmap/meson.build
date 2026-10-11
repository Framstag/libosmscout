mapFeaturesCfg = configuration_data()
mapFeaturesCfg.set('OSMSCOUT_MAP_HAVE_LIB_FREETYPE',ftDep.found(), description: 'the family name of a configured font file can be read')
mapFeaturesCfg.set('OSMSCOUT_DEBUG_LABEL_LAYOUTER',false, description: 'Extra debugging of label layouter')
mapFeaturesCfg.set('OSMSCOUT_DEBUG_GROUNDTILES',false, description: 'Extra debugging of ground tiles rendering')

configure_file(output: 'MapFeatures.h',
               install_dir: 'include/osmscoutmap',
               configuration: mapFeaturesCfg)

