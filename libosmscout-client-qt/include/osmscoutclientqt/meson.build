clientqtFeaturesCfg = configuration_data()
clientqtFeaturesCfg.set('OSMSCOUT_HAVE_LIB_PIPER',piperDep.found(), description: 'libpiper (Piper TTS) is available')

configure_file(output: 'ClientQtFeatures.h',
               install_dir: 'include/osmscoutclientqt',
               configuration: clientqtFeaturesCfg)

