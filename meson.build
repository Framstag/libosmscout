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

if get_option('qtVersion') == 5
  qt5 = import('qt5')
  qt5GuiDep = dependency('qt5', modules: [ 'Core', 'Gui'], required: false)
  qt5QmlDep = dependency('qt5', modules: 'Qml', required: false)
  qt5QuickDep = dependency('qt5', modules: 'Quick', required: false)
  qt5WidgetsDep = dependency('qt5', modules: 'Widgets', required: false)
  qt5SvgDep = dependency('qt5', modules: 'Svg', required: false)
  qt5NetworkDep = dependency('qt5', modules: 'Network', required: false)
  qt5LocationDep = dependency('qt5', modules: 'Location', required: false)
  qt5MultimediaDep = dependency('qt5', modules: 'Multimedia', required: false)

  message('Qt5 version : @0@ (required >= 5.6)'.format(qt5GuiDep.version()))

  if qt5GuiDep.found() and qt5GuiDep.version().version_compare('>=5.6') and qt5WidgetsDep.found() and get_option('enableMapQt')
    buildMapQt=true
  else
    buildMapQt=false
  endif

  if buildMapQt and qt5MultimediaDep.found() and qt5QmlDep.found() and qt5QuickDep.found() and qt5NetworkDep.found() and get_option('enableClientQt')
    buildClientQt=true
  else
    buildClientQt=false
  endif

elif get_option('qtVersion') == 6
  qt6 = import('qt6')
  qt6GuiDep = dependency('qt6', modules: [ 'Core', 'Core5Compat', 'Gui'], required: false)
  qt6QmlDep = dependency('qt6', modules: 'Qml', required: false)
  qt6QuickDep = dependency('qt6', modules: 'Quick', required: false)
  qt6WidgetsDep = dependency('qt6', modules: 'Widgets', required: false)
  qt6SvgDep = dependency('qt6', modules: 'Svg', required: false)
  qt6NetworkDep = dependency('qt6', modules: 'Network', required: false)
  qt6LocationDep = dependency('qt6', modules: 'Location', required: false)
  qt6MultimediaDep = dependency('qt6', modules: 'Multimedia', required: false)

  message('Qt6 version : @0@ (required >= 6.0)'.format(qt6GuiDep.version()))

  if qt6GuiDep.found() and qt6GuiDep.version().version_compare('>=6.0') and qt6WidgetsDep.found() and get_option('enableMapQt')
    buildMapQt=true
  else
    buildMapQt=false
  endif

  if buildMapQt and qt6MultimediaDep.found() and qt6QmlDep.found() and qt6QuickDep.found() and qt6NetworkDep.found() and get_option('enableClientQt')
    buildClientQt=true
  else
    buildClientQt=false
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

if get_option('qtVersion') == 5
  buildOSMScout2=buildGpx and buildMapQt and buildClientQt and qt5SvgDep.found()
  buildStyleEditor=buildMapQt and buildClientQt and qt5SvgDep.found()
elif get_option('qtVersion') == 6
  buildOSMScout2=buildGpx and buildMapQt and buildClientQt and qt6SvgDep.found()
  buildStyleEditor=buildMapQt and buildClientQt and qt6SvgDep.found()
endif

message('Meson version:           @0@'.format(meson.version()))
message('Project version:         @0@'.format(meson.project_version()))
message('Library version:         @0@'.format(libraryVersion))
message('Project license:         @0@'.format(meson.project_license()))
message('Build OS:                @0@'.format(build_machine.system()))
message('Host OS:                 @0@'.format(host_machine.system()))
message('Target OS:               @0@'.format(target_machine.system()))
message('compiler id:             @0@'.format(compiler.get_id()))
message('compiler arg. syntax:    @0@'.format(compiler.get_argument_syntax()))
message('openmp support:          @0@'.format(openmpDep.found()))
message('C++17 execution support: @0@'.format(stdExecutionAvailable))
message('libosmscout:             @0@'.format(true))
message('libosmscout-gpx:         @0@'.format(buildGpx))
message('libosmscout-import:      @0@'.format(buildImport))
message('libosmscout-test:        @0@'.format(buildImport))
message('libosmscout-map:         @0@'.format(true))
message('libosmscout-map-agg:     @0@'.format(buildMapAgg))
message('libosmscout-map-cairo:   @0@'.format(buildMapCairo))
message('libosmscout-map-directx: @0@'.format(buildMapDirectX))
message('libosmscout-map-gdi:     @0@'.format(buildMapGDI))
message('libosmscout-map-iosx:    @0@'.format(buildMapIOSX))
message('libosmscout-map-opengl:  @0@'.format(buildMapOpenGL))
message('libosmscout-map-qt:      @0@'.format(buildMapQt))
message('libosmscout-map-svg:     @0@'.format(buildMapSVG))
message('libosmscout-client-qt:   @0@'.format(buildClientQt))
message('BasemapImport:           @0@'.format(buildImport))
message('Import:                  @0@'.format(buildImport))
message('Demos:                   @0@'.format(true))
message('DumpData:                @0@'.format(true))
message('OSMScout2:               @0@'.format(buildOSMScout2))
message('StyleEditor:             @0@'.format(buildStyleEditor))
message('libosmscout-binding:     @0@'.format(buildBinding))
message('Tests:                   @0@'.format(buildTests))

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

if buildBinding
#  subdir('libosmscout-binding')
endif


run_target('cppcheck', command : ['scripts/cppcheck.sh',
  meson.current_build_dir(),
  join_paths(meson.current_build_dir(),'compile_commands.json')])
