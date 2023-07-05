coreFeaturesCfg = configuration_data()
# TODO
coreFeaturesCfg.set('OSMSCOUT_HAVE_SSE2',false, description: 'SSE2 processor extension available')
coreFeaturesCfg.set('OSMSCOUT_HAVE_LIB_MARISA',marisaDep.found(), description: 'libmarisa is available')
coreFeaturesCfg.set('OSMSCOUT_DEBUG_ROUTING',false, description: 'Extra debugging of routing')

configure_file(output: 'CoreFeatures.h',
               install_dir: 'include/osmscout/lib',
               configuration: coreFeaturesCfg)

