mapopenglFeaturesCfg = configuration_data()
# shaderInstallDir / fontInstallDir defined in libosmscout-map-opengl/meson.build
mapopenglFeaturesCfg.set('SHADER_INSTALL_DIR','"' + shaderInstallDir + '"', description: 'OpenGL shader installation dir')
mapopenglFeaturesCfg.set('DEFAULT_FONT_FILE','"' + join_paths(fontInstallDir, 'LiberationSans-Regular.ttf') + '"', description: 'OpenGL default font path')

configure_file(output: 'MapOpenGLFeatures.h',
               install_dir: 'include/osmscoutmapopengl',
               configuration: mapopenglFeaturesCfg)

