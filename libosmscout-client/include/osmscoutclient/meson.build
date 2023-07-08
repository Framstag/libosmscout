clientFeaturesCfg = configuration_data()
clientFeaturesCfg.set('OSMSCOUT_CLIENT_MESON_BUILD',true, description: 'we are building using meson')

configure_file(output: 'ClientFeatures.h',
               install_dir: 'include/osmscoutclient',
               configuration: clientFeaturesCfg)
