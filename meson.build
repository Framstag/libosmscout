project('libosmscout',
        'cpp',
        version: 'latest',
        meson_version: '>=0.63.0',
        default_options: ['cpp_std=c++17', 'buildtype=debugoptimized', 'warning_level=3'],
        license: ['LGPL'])

libraryVersion='1.1.1'

if build_machine.system()=='darwin'
  add_languages(['objcpp'])
  # Workarounds...
  add_project_arguments(['-fobjc-arc', '-std=c++17'], language: 'objcpp')
  add_project_link_arguments(['-dynamiclib'], language: 'objcpp')
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

if compiler.get_id()=='msvc'
  # Allow the compiler to fork itself for faster compiling
  # add_project_arguments('/MP', language: 'cpp')

  # Allow the compiler to create bigger object files
  add_project_arguments('/bigobj', language: 'cpp')

  # Do not warn regarding insecure function calls
  add_project_arguments('-D_CRT_SECURE_NO_DEPRECATE', language: 'cpp')
  add_project_arguments('-D_CRT_NONSTDC_NO_DEPRECATE', language: 'cpp')
  add_project_arguments('-D_USE_MATH_DEFINES', language: 'cpp')

  # Fast floating point processing
  add_project_arguments('/fp:fast', language: 'cpp')

  # Disable some over-eager compiler warnings
  add_project_arguments(['/wd4251', '/wd4456', '/wd4458'], language: 'cpp')

  if get_option('default_library')=='shared'
    # TODO: Should only be set for library builds, not executables
    add_project_arguments('-DDLL_EXPORT', language: 'cpp')
  endif
endif

if build_machine.system()=='windows' and compiler.get_id()=='gcc'
  add_project_arguments('-D_USE_MATH_DEFINES', language: 'cpp')
endif

# Check for headers
fcntlAvailable = compiler.has_header('fcntl.h')
statAvailable = compiler.has_header('sys/stat.h')
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
if compiler.get_id()=='clang' or compiler.get_id()=='msvc'
  openmpDep = dependency('', required: false)
else
  openmpDep = dependency('openmp', required: false)
endif

# C++20 Execution support

tbbDep = dependency('', required: false)

stdExecutionAvailable = compiler.has_header('execution')
if stdExecutionAvailable
 tbbDep = compiler.find_library('tbb', required: false)

 stdExecutionAvailable = tbbDep.found()
endif

# Optional
marisaDep = dependency('marisa', required : false)

# Import
zlibDep = dependency('zlib', required : false, fallback: ['zlib','zlib_dep'])
lzmaDep = dependency('liblzma', required : false, fallback: ['liblzma','lzma_dep'])

if get_option('enableXML')
  xml2Dep = dependency('libxml-2.0', version: '>= 2.6.0', required : false, fallback: ['libxml2','xml2lib_dep'])
else
  xml2Dep = dependency('', required: false)
endif

protobufDep = dependency('protobuf', required : false, fallback: ['protobuf','protoc_dep'])
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
gobjectDep = dependency('gobject-2.0',required: false)

# DirectX
d2d1Dep = compiler.find_library('d2d1', required: false)
dwriteDep = compiler.find_library('dwrite', required: false)
winCodecsDep = compiler.find_library('Windowscodecs', required: false)
d2dlHeaderAvailable = compiler.has_header('d2d1.h')

# GdiPlus
gdiplusDep = compiler.find_library('gdiplus', required: false)

# Qt

qtMapDep=dependency('', required: false)
qtClientDep=dependency('', required: false)

qt5=import('qt5', required: false)
qt5MapDep=dependency('qt5', modules: [ 'Core', 'Gui', 'Widgets', 'Svg'], required: false)
haveQt5MapDep=qt5MapDep.found() and qt5MapDep.version().version_compare('>=5.6') and get_option('enableMapQt')
qt5ClientDep=dependency('qt5', modules: [ 'Core', 'Gui', 'Widgets', 'Svg', 'Qml', 'Quick', 'Network', 'Location', 'Multimedia'], required: false)
haveQt5ClientDep=haveQt5MapDep and qt5ClientDep.found() and qt5ClientDep.version().version_compare('>=5.6') and get_option('enableClientQt')

qt6 = import('qt6', required: false)
qt6MapDep = dependency('qt6', modules: [ 'Core', 'Core5Compat', 'Gui', 'Svg', 'Widgets'], required: false)
haveQt6MapDep=qt6MapDep.found() and qt6MapDep.version().version_compare('>=6.0') and get_option('enableMapQt')
qt6ClientDep = dependency('qt6', modules: [ 'Core', 'Core5Compat', 'Gui', 'Svg', 'Widgets', 'Qml', 'Quick', 'Network', 'Multimedia'], required: false)
haveQt6ClientDep=haveQt6MapDep and qt6ClientDep.found() and qt6ClientDep.version().version_compare('>=6.0') and get_option('enableClientQt')

buildMapQt=false
buildClientQt=false

if get_option('qtVersion') == 5 and haveQt5MapDep
    message('Qt5 version : @0@ (required >= 5.6)'.format(qt5MapDep.version()))
    buildMapQt=true
    qtMapDep=qt5MapDep
    qt=qt5

    if haveQt5ClientDep
      qtClientDep=qt5ClientDep
      buildClientQt=true
    endif
elif haveQt6MapDep
    message('Qt6 version : @0@ (required >= 6.0)'.format(qt6MapDep.version()))
    buildMapQt=true
    qtMapDep=qt6MapDep
    qt=qt6

    if haveQt6ClientDep
      qtClientDep=qt6ClientDep
      buildClientQt=true
  endif
endif

# OpenGL
openGLDep = dependency('gl', required: false)
glmDep = dependency('glm', required: false, fallback: ['glm', 'glm_dep'])
glewDep = dependency('glew', required: false, fallback: ['glew', 'glew_dep'])

if build_machine.system()=='darwin' or build_machine.system()=='linux'
  glfwDep=dependency('glfw3', required: false)
else
  glfwDep = compiler.find_library('glfw', required: false)
endif

if openGLDep.found() and glmDep.found() and glewDep.found() and ftDep.found() and get_option('enableMapOpenGL')
  buildMapOpenGL=true
else
  buildMapOpenGL=false
endif

# Agg
if aggDep.found() and aggftAvailable and get_option('enableMapAgg')
  buildMapAgg=true
else
  buildMapAgg=false
endif

# Cairo
if cairoDep.found() and get_option('enableMapCairo')
  buildMapCairo=true
else
  buildMapCairo=false
endif

# DirectX
if d2d1Dep.found() and dwriteDep.found() and winCodecsDep.found() and d2dlHeaderAvailable and get_option('enableMapDirectX')
  buildMapDirectX=true
else
  buildMapDirectX=false
endif

# GDI
if build_machine.system()=='windows' and gdiplusDep.found()
  buildMapGDI=true
else
  buildMapGDI=false
endif

# GPX
if xml2Dep.found() and get_option('enableGpx')
    buildGpx=true
else
    buildGpx=false
endif

# import
if get_option('enableImport')
    buildImport=true
else
    buildImport=false
endif

if get_option('enableTests')
    buildTests=true
else
    buildTests=false
endif

# iOSX
if build_machine.system()=='darwin'
  foundationDep = dependency('appleframeworks', modules : 'Foundation', required : false)
  coreGraphicsDep = dependency('appleframeworks', modules : 'CoreGraphics', required : false)
  coreTextDep = dependency('appleframeworks', modules : 'CoreText', required : false)
  cocoaDep = dependency('appleframeworks', modules : 'Cocoa', required : false)
  appKitDep = dependency('appleframeworks', modules : 'AppKit', required : false)
endif

if build_machine.system()=='darwin' and foundationDep.found() and coreGraphicsDep.found() and coreTextDep.found() and cocoaDep.found() and appKitDep.found() and get_option('enableMapIOSX')
  buildMapIOSX=true
else
  buildMapIOSX=false
endif

if get_option('enableMapSvg')
    buildMapSVG=true
else
    buildMapSVG=false
endif

# Binding

if swigExe.found()
  buildBinding=true
else
  buildBinding=false
endif

buildClient=true

buildOSMScout2=buildGpx and buildMapQt and buildClientQt
buildStyleEditor=buildMapQt and buildClientQt

subdir('libosmscout')
subdir('libosmscout-map')

if buildGpx
    subdir('libosmscout-gpx')
endif

if buildMapAgg
  subdir('libosmscout-map-agg')
endif

if buildMapCairo
  subdir('libosmscout-map-cairo')
endif

if buildMapDirectX
  subdir('libosmscout-map-directx')
endif

if buildMapGDI
  subdir('libosmscout-map-gdi')
endif

if buildMapIOSX
  subdir('libosmscout-map-iosx')
endif

if buildMapOpenGL
  subdir('libosmscout-map-opengl')
endif

if buildMapQt
  subdir('libosmscout-map-qt')
endif

if buildMapSVG
    subdir('libosmscout-map-svg')
endif

subdir('libosmscout-client')

if buildClientQt
  subdir('libosmscout-client-qt')
endif

if buildImport
    subdir('libosmscout-import')
    subdir('libosmscout-test')
    subdir('BasemapImport')
    subdir('Import')
endif

subdir('Demos')
subdir('DumpData')
subdir('PublicTransportMap')

if buildGpx
  subdir('WellScoutedRoute')
endif

if buildOSMScout2
  subdir('OSMScout2')
endif

if buildStyleEditor
  subdir('StyleEditor')
endif

if buildMapOpenGL and glfwDep.found() and ftDep.found()
  subdir('OSMScoutOpenGL')
endif

if buildTests
    subdir('Tests')
endif

#if buildBinding
#  subdir('libosmscout-binding')
#endif


run_target('cppcheck', command : ['scripts/cppcheck.sh',
  meson.current_build_dir(),
  join_paths(meson.current_build_dir(),'compile_commands.json')])

summary({
  'Build OS': build_machine.system(),
  'Build CPU': build_machine.cpu(),
  'Host OS': host_machine.system(),
  'Host CPU': build_machine.cpu(),
  'Target OS': target_machine.system(),
  'Target CPU': build_machine.cpu(),
  }, section: 'OS')

summary({
  'Compiler id': compiler.get_id(),
  'Compiler arg. syntax': compiler.get_argument_syntax(),
  'OpenMP support': openmpDep.found(),
  'C++17 execution support': stdExecutionAvailable,
  }, section: 'Compiler')

summary({
  'Meson backend': meson.backend(),
  'Is unity': meson.is_unity(),
  }, section: 'Build')

summary({
  'Meson version': meson.version(),
  'Project version': meson.project_version(),
  'Library version': libraryVersion,
  'Project license': meson.project_license(),
  }, section: 'Project')

summary({
  'Qt map dependency': qtMapDep.version(),
  'Qt client dependency': qtClientDep.version(),
  }, section: 'Qt dependencies')

summary({
  'libosmscout': true,
  'libosmscout-test': buildImport,
  'libosmscout-import': buildImport,
  'libosmscout-map': true,
  'libosmscout-map-agg': buildMapAgg,
  'libosmscout-map-cairo': buildMapCairo,
  'libosmscout-map-directx': buildMapDirectX,
  'libosmscout-map-gdi': buildMapGDI,
  'libosmscout-map-iosx': buildMapIOSX,
  'libosmscout-map-opengl': buildMapOpenGL,
  'libosmscout-map-qt': buildMapQt,
  'libosmscout-map-svg': buildMapSVG,
  'libosmscout-gpx': buildGpx,
  'libosmscout-client': buildClient,
  'libosmscout-client-qt': buildClientQt,
  'libosmscout-binding': buildBinding
}, section: 'Libraries')

summary({
  'BasemapImport': buildImport,
  'Import': buildImport,
  'Demos': true,
  'DumpData': true,
  'OSMScout2': buildOSMScout2,
  'StyleEditor': buildStyleEditor,
  'Tests': buildTests
}, section: 'Apps')

