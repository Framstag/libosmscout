testIncDir = include_directories('include')

AccessParse =  executable('AccessParse',
             'src/AccessParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

AsyncProcessing =  executable('AsyncProcessing',
             'src/AsyncProcessing.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: false)

TimeParse =  executable('TimeParse',
             'src/TimeParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

Bearing = executable('Bearing',
             'src/Bearing.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

BitsAndBytesNeeded = executable('BitsAndBytesNeeded',
             'src/BitsAndBytesNeeded.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

CachePerformance = executable('CachePerformance',
             'src/CachePerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

CalculateResolution = executable('CalculateResolution',
             'src/CalculateResolution.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

CmdLineParsing = executable('CmdLineParsing',
             'src/CmdLineParsing.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

ColorParse = executable('ColorParse',
             'src/ColorParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

CoordinateEncoding = executable('CoordinateEncoding',
             'src/CoordinateEncoding.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

if buildMapQt
  drawtextMocs = qt5.preprocess(moc_headers : ['include/DrawWindow.h'])

  DrawTextQt = executable('DrawTextQt',
               'src/DrawTextQt.cpp',
               drawtextMocs,
               include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
               dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep],
               link_with: [osmscoutmapqt, osmscoutmap, osmscout],
               install: false)
endif

EncodeNumber = executable('EncodeNumber',
             'src/EncodeNumber.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

FeatureLabelTest = executable('FeatureLabelTest',
             'src/FeatureLabelTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

FileScannerWriter = executable('FileScannerWriter',
             'src/FileScannerWriter.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

GeoBox = executable('GeoBox',
             'src/GeoBox.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

GeoCoordParse = executable('GeoCoordParse',
             'src/GeoCoordParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

Geometry = executable('Geometry',
             'src/Geometry.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

if buildImport
    LocationServiceTest = executable('LocationServiceTest',
                 [
                   'src/LocationServiceTest.cpp',
                   'src/SearchForLocationByStringTest.cpp',
                   'src/SearchForLocationByFormTest.cpp',
                   'src/SearchForPOIByFormTest.cpp'
                 ],
                 include_directories: [testIncDir, osmscouttestIncDir, osmscoutimportIncDir, osmscoutIncDir],
                 dependencies: [mathDep, openmpDep],
                 link_with: [osmscouttest, osmscoutimport, osmscout],
                 install: false)
endif

MapRotate = executable('MapRotate',
             'src/MapRotate.cpp',
             include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: false)

MultiDBRouting = executable('MultiDBRouting',
             'src/MultiDBRouting.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

NumberSet = executable('NumberSet',
             'src/NumberSet.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

NumberSetPerformance = executable('NumberSetPerformance',
             'src/NumberSetPerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)


OSTAndOSSCheck = executable('OSTAndOSSCheck',
             'src/OSTAndOSSCheck.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: false)

ReaderScannerPerformance = executable('ReaderScannerPerformance',
             'src/ReaderScannerPerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

ScanConversion = executable('ScanConversion',
             'src/ScanConversion.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

StdByteTest = executable('StdByteTest',
             'src/StdByteTest.cpp',
             include_directories: [testIncDir],
             dependencies: [mathDep],
             install: false)

StringUtils = executable('StringUtils',
             'src/StringUtils.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

ThreadedDatabase = executable('ThreadedDatabase',
             'src/ThreadedDatabase.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: false)

SunriseSunset = executable('SunriseSunset',
             'src/SunriseSunsetTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: false)

TilingTest = executable('TilingTest',
             'src/TilingTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: false)

TransPolygon = executable('TransPolygon',
             'src/TransPolygon.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

WorkQueue = executable('WorkQueue',
             'src/WorkQueue.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: false)

WStringStringConversion = executable('WStringStringConversion',
             'src/WStringStringConversion.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: false)

LabelPathTest = executable('LabelPathTest',
           'src/LabelPathTest.cpp',
           include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscoutmap, osmscout],
           install: false)

Base64Test = executable('Base64Test',
           'src/Base64.cpp',
           include_directories: [testIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscout],
           install: false)

CoordBufferTest = executable('CoordBufferTest',
           'src/CoordBufferTest.cpp',
           include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscoutmap, osmscout],
           install: false)

ostandossEnv = environment()

ostandossEnv.set('TESTS_TOP_DIR', meson.current_source_dir())

test('Check parsing of access rights', AccessParse)
test('Check parsing of time string', TimeParse)
test('Check calculation of bearing', Bearing)
test('Check encoding of numbers', BitsAndBytesNeeded)
test('Check cache functionality with CachePerformance', CachePerformance, args : ['--size', '1000'])
test('Check position accuracy with coordinate bits', CalculateResolution)
test('Check parsing of command line args', CmdLineParsing)
test('Check parsing of colors', ColorParse)
test('Check number set performance', NumberSetPerformance, timeout: 180)
test('Check reader scanner performance', ReaderScannerPerformance, args : [meson.current_source_dir() + '/data/testregion'])
test('Check parsing of ways.dat', CoordinateEncoding, args : [meson.current_source_dir() + '/data/testregion'])
test('Check routing', MultiDBRouting, args : ['50.412', '14.534', '50.424', '14.6013', meson.current_source_dir() + '/data/testregion'])
test('Check threaded database', ThreadedDatabase, args : [
        '--threads', '100',
        '--iterations', '1000',
        meson.current_source_dir() + '/data/testregion',
        meson.current_source_dir() + '/../stylesheets/standard.oss'])

test('Check SunriseSunset utility', SunriseSunset)
test('Check encoding of numbers', EncodeNumber)
test('Check label formatting', FeatureLabelTest)
test('Check File access implementation', FileScannerWriter)
test('Check parsing of geo box intersection', GeoBox)
test('Check parsing of geo coordinates', GeoCoordParse)
test('Check impl. of geometric functions', Geometry)
test('Check coordinate buffer conversions', CoordBufferTest)

if (compiler.get_id()=='gcc' and target_machine.system()=='windows')
  warning('Test "Check rotation of maps" does not run under MSYS because of missing locale support')
else
  test('Check rotation of maps', MapRotate)
endif

test('Check correctness of NumberSet class', NumberSet)
test('Check scan conversion code', ScanConversion)
test('Check string utils', StringUtils)
test('Check tiling calculation code', TilingTest)
test('Check polygon transformation code', TransPolygon)
test('Check implementation of work queue', WorkQueue)
test('Check WString<=>String conversion code', WStringStringConversion)
test('Check LabelPath code', LabelPathTest)
test('Check Base64 code', Base64Test)

if buildImport
    test('Check LocationService', LocationServiceTest, env: ostandossEnv)
endif

stylesheets = [
            'standard.oss',
            'winter-sports.oss',
            'boundaries.oss',
            'railways.oss',
            'motorways.oss',
            'public-transport.oss',
            ]

foreach stylesheet : stylesheets
    test('Check OSS/T file '+stylesheet,
            OSTAndOSSCheck,
            args : ['--warning-as-error',
                    meson.current_source_dir() + '/../stylesheets/map.ost',
                    meson.current_source_dir() + '/../stylesheets/' + stylesheet])
endforeach

if buildClientQt
  threadingMocs = qt5.preprocess(moc_headers : ['include/ClientQtThreading.h'])

  ClientQtThreading = executable('ClientQtThreading',
               'src/ClientQtThreading.cpp',
               threadingMocs,
               include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientqtIncDir],
               dependencies: [mathDep, threadDep, openmpDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5NetworkDep, qt5MultimediaDep],
               link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclientqt],
               install: false)
endif

if buildClientQt
  testMocs = qt5.preprocess(moc_headers : ['include/DownloaderTest.h'])

  QtFileDownloader = executable('QtFileDownloader',
               'src/QtFileDownloader.cpp',
               testMocs,
               include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientqtIncDir],
               dependencies: [mathDep, threadDep, openmpDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5NetworkDep, qt5MultimediaDep],
               link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclientqt],
               install: false)
endif

