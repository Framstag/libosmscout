project('libosmscout',
        'cpp',
        meson_version: '>=0.40.0',
        default_options: ['cpp_std=c++11'],
        license: ['LGPL'])

compiler = meson.get_compiler('cpp')

if get_option('default_library')=='shared'
  if compiler.get_id()=='gcc' and compiler.version().version_compare('>=4.8.0')
    haveVisibility=true
  else
    haveVisibility=false  
  endif
else
  haveVisibility=false      
endif

if compiler.has_argument('-fopenmp')
  add_global_arguments('-fopenmp', language: 'cpp')
  add_global_link_arguments('-fopenmp', language: 'cpp')
endif

fcntlAvailable = compiler.has_header('fcntl.h')
statAvailable = compiler.has_header('sys/stat.h')
iconvAvailable = compiler.has_header('iconv.h')
codecvtAvailable = compiler.has_header('codecvt')

sharedMutexAvailable = compiler.has_header_symbol('shared_mutex','shared_mutex', prefix: 'using namespace std;')

sizeOfWChar = compiler.sizeof('wchar_t')

mmapAvailable = compiler.has_function('mmap')
posixfadviceAvailable = compiler.has_function('posix_fadvise')
posixmadviceAvailable = compiler.has_function('posix_madvise')
fseeki64Available = compiler.has_function('_fseeki64')
ftelli64Available = compiler.has_function('_ftelli64')
fseekoAvailable = compiler.has_function('fseeko')

# Base
mathDep = compiler.find_library('m', required : false)
threadDep = dependency('threads')
marisaDep = dependency('marisa', required : false)

# Import
protobufDep = dependency('protobuf', required : false)
xml2Dep = dependency('libxml-2.0', version: '>= 2.6.0', required : false)
zlibDep = dependency('zlib', required : false)
wsock32Dep=compiler.find_library('wsock32', required: false)

protocCmd = find_program('protoc')

# Backends

# Agg
aggDep = dependency('libagg', required : false)

# cairo
cairoDep = dependency('cairo', required : false)
pangoDep = dependency('pango', required : false)
pangocairoDep = dependency('pangocairo', required : false)
pangoft2Dep = dependency('pangoft2', required : false)
pngDep = dependency('libpng', required: false)

# Qt
qt5 = import('qt5')
qt5GuiDep = dependency('qt5', modules: [ 'Core', 'Gui'], required: false)
qt5QmlDep = dependency('qt5', modules: 'Qml', required: false)
qt5QuickDep = dependency('qt5', modules: 'Quick', required: false)
qt5WidgetsDep = dependency('qt5', modules: 'Widgets', required: false)
qt5SvgDep = dependency('qt5', modules: 'Svg', required: false)
qt5NetworkDep = dependency('qt5', modules: 'Network', required: false)
qt5LocationDep = dependency('qt5', modules: 'Location', required: false)

# OpenGL
openGLDep = dependency('gl', required: false)

glutDep = dependency('glut', required: false)

if not glutDep.found()
  glutDep = dependency('freeglut', required: false)
endif

if not glutDep.found()
  glutDep = compiler.find_library('glut', required: false)
endif
  
glewDep = dependency('glew', required: false)

# Workaround for dependency, else error under Ubuntu 14.04
glfwDep = compiler.find_library('glfw', required: false)

if aggDep.found()
  buildMapAgg=true
else
  buildMapAgg=false  
endif

if cairoDep.found()
  buildMapCairo=true
else
  buildMapCairo=false
endif

if qt5GuiDep.found() and qt5QmlDep.found() and qt5QuickDep.found() and qt5WidgetsDep.found() and qt5NetworkDep.found()
  buildClientQt=true
  buildMapQt=true
else
  buildClientQt=false
  buildMapQt=false
endif

if openGLDep.found() and glutDep.found() and glewDep.found()
  buildMapOpenGL=true
else
  buildMapOpenGL=false
endif  

subdir('libosmscout')
subdir('libosmscout-import')
subdir('libosmscout-map')

if buildMapAgg
  # TODO: Build libosmsocut-map-agg
endif  

if buildMapCairo
  subdir('libosmscout-map-cairo')
endif

if buildMapQt
  subdir('libosmscout-map-qt')
endif

# TODO: Build libosmscout-map-iosx

if buildMapOpenGL
  subdir('libosmscout-map-opengl')
endif

subdir('libosmscout-map-svg')

if buildClientQt
  subdir('libosmscout-client-qt')
endif
  
subdir('BasemapImport')
subdir('Import')
subdir('Demos')
subdir('DumpData')

if buildMapQt and buildClientQt and qt5SvgDep.found()
  subdir('OSMScout2')
  subdir('StyleEditor')
endif

if buildMapOpenGL and glfwDep.found()
  subdir('OSMScoutOpenGL')
endif

# TODO: Tests
