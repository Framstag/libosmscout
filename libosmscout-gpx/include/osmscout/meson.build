gpxFeaturesCfg = configuration_data()
gpxFeaturesCfg.set('OSMSCOUT_GPX_HAVE_LIB_XML',xml2Dep.found(), description: '*.gpx can be imported/exported')
gpxFeaturesCfg.set('OSMSCOUT_GPX_MESON_BUILD',true, description: 'we are building using meson')

configure_file(output: 'GPXFeatures.h',
               configuration: gpxFeaturesCfg)

