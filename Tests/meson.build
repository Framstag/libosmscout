testIncDir = include_directories('include')

AccessParse =  executable('AccessParse',
             'src/AccessParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

TimeParse =  executable('TimeParse',
             'src/TimeParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

Bearing = executable('Bearing',
             'src/Bearing.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

BitsAndBytesNeeded = executable('BitsAndBytesNeeded',
             'src/BitsAndBytesNeeded.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

CachePerformance = executable('CachePerformance',
             'src/CachePerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

CalculateResolution = executable('CalculateResolution',
             'src/CalculateResolution.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

CmdLineParsing = executable('CmdLineParsing',
             'src/CmdLineParsing.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

ColorParse = executable('ColorParse',
             'src/ColorParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

CoordinateEncoding = executable('CoordinateEncoding',
             'src/CoordinateEncoding.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

if buildMapQt
  drawtextMocs = qt5.preprocess(moc_headers : ['include/DrawWindow.h'])

  DrawTextQt = executable('DrawTextQt',
               'src/DrawTextQt.cpp',
               drawtextMocs,
               include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
               dependencies: [mathDep, qt5GuiDep, qt5WidgetsDep],
               link_with: [osmscoutmapqt, osmscoutmap, osmscout],
               install: false)
endif

EncodeNumber = executable('EncodeNumber',
             'src/EncodeNumber.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

FileScannerWriter = executable('FileScannerWriter',
             'src/FileScannerWriter.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

GeoBox = executable('GeoBox',
             'src/GeoBox.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

GeoCoordParse = executable('GeoCoordParse',
             'src/GeoCoordParse.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

Geometry = executable('Geometry',
             'src/Geometry.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

MapRotate = executable('MapRotate',
             'src/MapRotate.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscoutmap, osmscout],
             install: false)

MultiDBRouting = executable('MultiDBRouting',
             'src/MultiDBRouting.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

NumberSet = executable('NumberSet',
             'src/NumberSet.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

NumberSetPerformance = executable('NumberSetPerformance',
             'src/NumberSetPerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)


OSTAndOSSCheck = executable('OSTAndOSSCheck',
             'src/OSTAndOSSCheck.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscoutmap, osmscout],
             install: false)

ReaderScannerPerformance = executable('ReaderScannerPerformance',
             'src/ReaderScannerPerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

ScanConversion = executable('ScanConversion',
             'src/ScanConversion.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

ThreadedDatabase = executable('ThreadedDatabase',
             'src/ThreadedDatabase.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscoutmap, osmscout],
             install: false)

TransPolygon = executable('TransPolygon',
             'src/TransPolygon.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

WorkQueue = executable('WorkQueue',
             'src/WorkQueue.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

WStringStringConversion = executable('WStringStringConversion',
             'src/WStringStringConversion.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep],
             link_with: [osmscout],
             install: false)

ostandossEnv = environment()

ostandossEnv.set('TESTS_TOP_DIR', meson.current_source_dir())

test('Check parsing of access rights', AccessParse)
test('Check parsing of time string', TimeParse)
test('Check calculation of bearing', Bearing)
test('Check encoding of numbers', BitsAndBytesNeeded)
test('Check parsing of command line args', CmdLineParsing)
test('Check parsing of colors', ColorParse)
test('Check encoding of numbers', EncodeNumber)
test('Check File access implementation', FileScannerWriter)
test('Check parsing of geo box intersection', GeoBox)
test('Check parsing of geo coordinates', GeoCoordParse)
test('Check impl. of geometric functions', Geometry)
test('Check rotation of maps', MapRotate)
test('Check correctness of NumberSet class', NumberSet)
test('Check standard OST and OSS files', OSTAndOSSCheck, env: ostandossEnv)
test('Check scan conversion code', ScanConversion)
test('Check polygon transformation code', TransPolygon)
test('Check implementation of work queue', WorkQueue)
test('Check WString<=>String conversion code', WStringStringConversion)
