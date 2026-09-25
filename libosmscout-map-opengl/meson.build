cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutMapOpenGL_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

# OpenGL shader/font installation dirs, mirroring libosmscout-map-opengl/CMakeLists.txt
if host_machine.system() == 'darwin'
  shaderInstallDir = '/Library/Application Support/osmscout/shaders'
  fontInstallDir = '/Library/Application Support/osmscout/fonts'
elif host_machine.system() == 'windows'
  shaderInstallDir = 'C:/ProgramData/osmscout/shaders'
  fontInstallDir = 'C:/ProgramData/osmscout/fonts'
else
  shaderInstallDir = join_paths(get_option('prefix'), 'share', 'osmscout', 'shaders')
  fontInstallDir = join_paths(get_option('prefix'), 'share', 'osmscout', 'fonts')
endif

subdir('include')
subdir('include/osmscoutmapopengl')
subdir('include/osmscoutmapopengl/private')
subdir('src')

osmscoutmapopengl = library('osmscout_map_opengl',
                            osmscoutmapopenglSrc,
                            include_directories: [osmscoutmapopenglIncDir, osmscoutmapIncDir, osmscoutIncDir],
                            cpp_args: cppArgs,
                            dependencies: [mathDep, threadDep, openGLDep, glmDep, glewDep, pngDep, ftDep],
                            link_with: [osmscout, osmscoutmap],
                            version: libraryVersion,
                            install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutmapopengl,
             name: 'libosmscout_map_opengl',
             filebase: 'libosmscout_map_opengl',
             description: 'Library for rendering OSM data using OpenGL',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')

# Install shaders and fonts, mirroring the CMake install rules
# (SHADER_FILES / FONT_FILES in libosmscout-map-opengl/CMakeLists.txt).
shaderFiles = files(
  'data/shaders/AreaFragmentShader.frag',
  'data/shaders/AreaVertexShader.vert',
  'data/shaders/GroundFragmentShader.frag',
  'data/shaders/GroundVertexShader.vert',
  'data/shaders/PathFragmentShader.frag',
  'data/shaders/PathVertexShader.vert',
  'data/shaders/Projection.vert',
  'data/shaders/QuadFragmentShader.frag',
  'data/shaders/QuadVertexShader.vert',
  'data/shaders/TextFragmentShader.frag',
  'data/shaders/TextVertexShader.vert',
)

fontFiles = files(
  'data/fonts/LiberationSans-Regular.ttf',
  'data/fonts/LICENSE',
)

install_data(shaderFiles, install_dir: shaderInstallDir)
install_data(fontFiles, install_dir: fontInstallDir)
