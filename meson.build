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

protocCmd = find_program('protoc')

# Backends
aggDep = dependency('libagg', required : false)

cairoDep = dependency('cairo', required : false)
pangoDep = dependency('pango', required : false)
pangocairoDep = dependency('pangocairo', required : false)
pngDep = dependency('libpng', required: false)

qt5 = import('qt5')
qt5GuiDep = dependency('Qt5Gui')
qt5QmlDep = dependency('Qt5Qml')
qt5QuickDep = dependency('Qt5Quick')
qt5WidgetsDep = dependency('Qt5Widgets')
qt5LocationDep = dependency('Qt5Location')

if cairoDep.found()
  buildMapCairo=true
else
  buildMapCairo=false
endif

if qt5GuiDep.found() and qt5QuickDep.found() and qt5WidgetsDep.found()
  buildClientQt=true
  buildMapQt=true
else
  buildClientQt=false
  buildMapQt=false
endif

subdir('libosmscout')
subdir('libosmscout-import')
subdir('libosmscout-map')

if buildMapCairo
  subdir('libosmscout-map-cairo')
endif

if buildMapQt
  subdir('libosmscout-map-qt')
endif

if buildClientQt
  subdir('libosmscout-client-qt')
endif
  
subdir('BasemapImport')
subdir('Import')
subdir('Demos')
subdir('DumpData')

if buildMapQt and buildClientQt
  subdir('OSMScout2')
endif
