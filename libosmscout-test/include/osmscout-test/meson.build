testFeaturesCfg = configuration_data()
testFeaturesCfg.set('OSMSCOUT_TEST_MESON_BUILD',true, description: 'we are building using meson')

configure_file(output: 'TestFeatures.h',
               install_dir: 'include/osmscout-test',
               configuration: testFeaturesCfg)

