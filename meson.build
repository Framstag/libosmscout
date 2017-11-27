project('libosmscout',
        'cpp',
        meson_version: '>=0.40.0',
        default_options: ['cpp_std=c++11', 'buildtype=debugoptimized', 'warning_level=3'],
        license: ['LGPL'])

if build_machine.system()=='darwin'
  add_languages(['objcpp'])
  # Workarounds...
  add_global_arguments(['-fobjc-arc', '-std=c++11'], language: 'objcpp')
  add_global_link_arguments(['-dynamiclib'], language: 'objcpp')
endif

compiler = meson.get_compiler('cpp')

# Check for specific compiler options

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

if compiler.get_id()=='msvc'
  # Allow the compiler to fork itself for faster compiling
  # add_global_arguments('/MP', language: 'cpp')

  # Allow the compiler to create bigger object files
  add_global_arguments('/bigobj', language: 'cpp')

  # Do not warn regarding insecure function calls
  add_global_arguments('-D_CRT_SECURE_NO_DEPRECATE', language: 'cpp')
  add_global_arguments('-D_CRT_NONSTDC_NO_DEPRECATE', language: 'cpp')

  # Fast floating point processing
  add_global_arguments('/fp:fast', language: 'cpp')

  # Disable some over-eager compiler warnings
  add_global_arguments(['/wd4251', '/wd4456', '/wd4458'], language: 'cpp')

  if get_option('default_library')=='shared'
    # TODO: Should only be set for library builds, not executables
    add_global_arguments('-DDLL_EXPORT', language: 'cpp')
  endif
endif

# Check for headers
fcntlAvailable = compiler.has_header('fcntl.h')
statAvailable = compiler.has_header('sys/stat.h')
iconvAvailable = compiler.has_header('iconv.h')
codecvtAvailable = compiler.has_header('codecvt')
jniAvailable = compiler.has_header('jni.h')

# Check for header symbols
sharedMutexAvailable = compiler.has_header_symbol('shared_mutex','shared_mutex', prefix: 'using namespace std;')

# Check for datatype sizes
sizeOfWChar = compiler.sizeof('wchar_t')

# Check for specific functions
mmapAvailable = compiler.has_function('mmap')
posixfadviceAvailable = compiler.has_function('posix_fadvise')
posixmadviceAvailable = compiler.has_function('posix_madvise')
fseeki64Available = compiler.has_function('_fseeki64')
ftelli64Available = compiler.has_function('_ftelli64')
fseekoAvailable = compiler.has_function('fseeko')

# Check for external programs
swigExe = find_program('swig', required: false)

# Base
mathDep = compiler.find_library('m', required : false)
threadDep = dependency('threads')
iconvDep = compiler.find_library('iconv', required: false)
marisaDep = dependency('marisa', required : false)

# Import
protobufDep = dependency('protobuf', required : false)
xml2Dep = dependency('libxml-2.0', version: '>= 2.6.0', required : false)
zlibDep = dependency('zlib', required : false)
wsock32Dep=compiler.find_library('wsock32', required: false)

protocCmd = find_program('protoc', required: false)

# Backends

# Agg
aggDep = dependency('libagg', required : false)
ftDep = dependency('freetype2', required: false)
aggftpicDep = compiler.find_library('aggfontfreetype_pic', required: false)
aggftDep = compiler.find_library('aggfontfreetype', required: false)

if ftDep.found() and aggftpicDep.found()
aggftAvailable = compiler.links('''
#include <agg2/agg_font_freetype.h>

agg::font_engine_freetype_int32 fontEngine;

int main() {
return 0;
}
''', dependencies: [aggDep, aggftpicDep, ftDep])
elif ftDep.found() and aggftDep.found()
aggftAvailable = compiler.links('''
#include <agg2/agg_font_freetype.h>

agg::font_engine_freetype_int32 fontEngine;

int main() {
return 0;
}
''', dependencies: [aggDep, aggftDep, ftDep])
else
  aggftAvailable = false
endif

# cairo
cairoDep = dependency('cairo', required : false)
pangoDep = dependency('pango', required : false)
pangocairoDep = dependency('pangocairo', required : false)
pangoft2Dep = dependency('pangoft2', required : false)
pngDep = dependency('libpng', required: false)

# DirectX

d2d1Dep = compiler.find_library('d2d1', required: false)
dwriteDep = compiler.find_library('dwrite', required: false)
winCodecsDep = compiler.find_library('Windowscodecs', required: false)
d2dlHeaderAvailable = compiler.has_header('d2d1.h')

# Qt
qt5 = import('qt5')
qt5GuiDep = dependency('qt5', modules: [ 'Core', 'Gui'], required: false)
qt5QmlDep = dependency('qt5', modules: 'Qml', required: false)
qt5QuickDep = dependency('qt5', modules: 'Quick', required: false)
qt5WidgetsDep = dependency('qt5', modules: 'Widgets', required: false)
qt5SvgDep = dependency('qt5', modules: 'Svg', required: false)
qt5NetworkDep = dependency('qt5', modules: 'Network', required: false)
qt5LocationDep = dependency('qt5', modules: 'Location', required: false)

if qt5GuiDep.found() and qt5QmlDep.found() and qt5QuickDep.found() and qt5WidgetsDep.found() and qt5NetworkDep.found()
  buildClientQt=true
  buildMapQt=true
else
  buildClientQt=false
  buildMapQt=false
endif

# OpenGL
openGLDep = dependency('gl', required: false)

if build_machine.system()=='darwin'
  glutDep = dependency('appleframeworks', modules: 'glut', required: false)
else
  glutDep = dependency('glut', required: false)

  if not glutDep.found()
    glutDep = dependency('freeglut', required: false)
  endif

  if not glutDep.found()
   glutDep = compiler.find_library('glut', required: false)
  endif
endif


glmDep = dependency('glm', required: false)
glmAvailable = compiler.has_header('glm/glm.hpp', dependencies: glmDep)


glewDep = dependency('glew', required: false)

# Workaround for dependency, else error under Ubuntu 14.04

if build_machine.system()=='darwin'
  glfwDep=dependency('glfw3', required: false)
else
  glfwDep = compiler.find_library('glfw', required: false)
endif

if openGLDep.found() and glutDep.found() and (glmDep.found() or glmAvailable) and glewDep.found() and ftDep.found()
  buildMapOpenGL=true
else
  buildMapOpenGL=false
endif

# Agg
if aggDep.found() and aggftAvailable
  buildMapAgg=true
else
  buildMapAgg=false
endif

# Cairo
if cairoDep.found()
  buildMapCairo=true
else
  buildMapCairo=false
endif

#DirectX

if d2d1Dep.found() and dwriteDep.found() and winCodecsDep.found() and d2dlHeaderAvailable
  buildMapDirectX=true
else
  buildMapDirectX=false
endif

# iOSX
buildMapIOSX=build_machine.system()=='darwin'

# Binding

if swigExe.found()
  buildBinding=true
else
  buildBinding=false
endif

buildOSMScout2=buildMapQt and buildClientQt and qt5SvgDep.found()
buildStyleEditor=buildMapQt and buildClientQt and qt5SvgDep.found()

message('libosmscout:             @0@'.format(true))
message('libosmscout-gpx:         @0@'.format(true))
message('libosmscout-import:      @0@'.format(true))
message('libosmscout-map:         @0@'.format(true))
message('libosmscout-map-agg:     @0@'.format(buildMapAgg))
message('libosmscout-map-cairo:   @0@'.format(buildMapCairo))
message('libosmscout-map-directx: @0@'.format(buildMapDirectX))
#message('libosmscout-map-iosx:   @0@'.format(buildMapIOSX))
message('libosmscout-map-iosx:    @0@'.format(false))
message('libosmscout-map-opengl:  @0@'.format(buildMapOpenGL))
message('libosmscout-map-qt:      @0@'.format(buildMapQt))
message('libosmscout-map-svg:     @0@'.format(true))
message('BasemapImport:           @0@'.format(true))
message('Import:                  @0@'.format(true))
message('Demos:                   @0@'.format(true))
message('DumpData:                @0@'.format(true))
message('OSMScout2:               @0@'.format(buildOSMScout2))
message('StyleEditor:             @0@'.format(buildStyleEditor))
message('libosmscout-binding:     @0@'.format(buildBinding))
message('Tests:                   @0@'.format(true))

subdir('libosmscout')
subdir('libosmscout-import')
subdir('libosmscout-map')
subdir('libosmscout-gpx')

if buildMapAgg
  subdir('libosmscout-map-agg')
endif

if buildMapCairo
  subdir('libosmscout-map-cairo')
endif

if buildMapDirectX
  subdir('libosmscout-map-directx')
endif

if buildMapIOSX
# Does not yet work, because we are not able to active std11 support, fixed in meson trunk
#  subdir('libosmscout-map-iosx')
endif

if buildMapOpenGL
  subdir('libosmscout-map-opengl')
endif

if buildMapQt
  subdir('libosmscout-map-qt')
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

if buildMapOpenGL and glfwDep.found() and ftDep.found()
  subdir('OSMScoutOpenGL')
endif

subdir('Tests')

if buildBinding
#  subdir('libosmscout-binding')
endif
