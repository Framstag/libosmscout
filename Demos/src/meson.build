demosIncDir = include_directories('.')

demoCfg = configuration_data()

demoCfg.set('HAVE_LIB_OSMSCOUTMAPCAIRO',buildMapCairo, description: 'Cairo backend supported')
demoCfg.set('HAVE_LIB_OSMSCOUTMAPQT',buildMapQt, description: 'Qt backend supported')
demoCfg.set('HAVE_LIB_OSMSCOUTMAPAGG',buildMapAgg, description: 'Agg backend supported')
demoCfg.set('HAVE_LIB_OSMSCOUTMAPOPENGL',buildMapOpenGL and glfwDep.found() and ftDep.found(), description: 'OpenGL backend supported')

configure_file(output: 'config.h',
               configuration: demoCfg)

