mapopenglFeaturesCfg = configuration_data()
# TODO: setup installation dir properly
mapopenglFeaturesCfg.set('SHADER_INSTALL_DIR','"shaders"', description: 'OpenGL shader installation dir')
mapopenglFeaturesCfg.set('DEFAULT_FONT_FILE','"LiberationSans-Regular.ttf"', description: 'OpenGL default font path')

configure_file(output: 'MapOpenGLFeatures.h',
               configuration: mapopenglFeaturesCfg)

