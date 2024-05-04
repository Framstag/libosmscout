testIncDir = include_directories('include')

testInstallDir = 'share/osmscout/tests'

AccessParse =  executable('AccessParse',
             'src/AccessParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of access rights', AccessParse)

AsyncProcessing =  executable('AsyncProcessing',
             'src/AsyncProcessing.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

AsyncWorker =  executable('AsyncWorker',
             'src/AsyncWorker.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check asynchronous worker', AsyncWorker)

TimeParse =  executable('TimeParse',
             'src/TimeParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of time string', TimeParse)

Bearing = executable('Bearing',
             'src/Bearing.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check calculation of bearing', Bearing)

BitsAndBytesNeeded = executable('BitsAndBytesNeeded',
             'src/BitsAndBytesNeeded.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check encoding of numbers', BitsAndBytesNeeded)

ByteTest = executable('ByteTest',
             'src/ByteTest.cpp',
             include_directories: [testIncDir],
             dependencies: [mathDep],
             install: true,
             install_dir: testInstallDir)

test('Check std_byte behaviour', ByteTest)

CachePerformance = executable('CachePerformance',
             'src/CachePerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check cache functionality with CachePerformance', CachePerformance, args : ['--size', '1000'])

CalculateResolution = executable('CalculateResolution',
             'src/CalculateResolution.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check position accuracy with coordinate bits', CalculateResolution)

CancelableFuture = executable('CancelableFuture',
             'src/CancelableFuture.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check cancelable future', CancelableFuture)

CmdLineParsing = executable('CmdLineParsing',
             'src/CmdLineParsing.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of command line args', CmdLineParsing)

ColorParse = executable('ColorParse',
             'src/ColorParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of colors', ColorParse)

CoordinateEncoding = executable('CoordinateEncoding',
             'src/CoordinateEncoding.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of ways.dat', CoordinateEncoding, args : [meson.current_source_dir() + '/data/testregion'])

if buildMapQt
    drawtextMocs = qt.preprocess(moc_headers : ['include/DrawWindow.h'])

    DrawTextQt = executable('DrawTextQt',
                 'src/DrawTextQt.cpp',
                 drawtextMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                 dependencies: [mathDep, openmpDep, qtMapDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout],
                 install: true,
                 install_dir: testInstallDir)
endif

EncodeNumber = executable('EncodeNumber',
             'src/EncodeNumber.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check encoding of numbers', EncodeNumber)

FeatureLabelTest = executable('FeatureLabelTest',
             'src/FeatureLabelTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check label formatting', FeatureLabelTest)

File = executable('File',
             'src/File.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check File utilities', File)

FileScannerWriter = executable('FileScannerWriter',
             'src/FileScannerWriter.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check File access implementation', FileScannerWriter)

GeoBox = executable('GeoBox',
             'src/GeoBox.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check handling of geo box intersection', GeoBox)

GeoCoordParse = executable('GeoCoordParse',
             'src/GeoCoordParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of geo coordinates', GeoCoordParse)

Geometry = executable('Geometry',
             'src/Geometry.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check impl. of geometric functions', Geometry)

headerCheckEnv = environment()
headerCheckEnv.set('SOURCE_ROOT', meson.project_source_root())

HeaderCheck = executable('HeaderCheck',
             'src/HeaderCheck.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check use of \'<\'...\'>\' for includes', HeaderCheck, env: headerCheckEnv)

Latch = executable('Latch',
             'src/Latch.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check latch consistency', Latch)

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
                 install: true,
                 install_dir: testInstallDir)
endif

MapRotate = executable('MapRotate',
             'src/MapRotate.cpp',
             include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: true,
             install_dir: testInstallDir)

if (compiler.get_id()=='gcc' and target_machine.system()=='windows')
  warning('Test "Check rotation of maps" does not run under MSYS because of missing locale support')
else
  test('Check rotation of maps', MapRotate)
endif

MercatorProjection = executable('MercatorProjection',
             'src/MercatorProjection.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check MercatorProjection', MercatorProjection)

MultiDBRouting = executable('MultiDBRouting',
             'src/MultiDBRouting.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check routing', MultiDBRouting, args : ['50.412', '14.534', '50.424', '14.6013', meson.current_source_dir() + '/data/testregion'])

NumberSet = executable('NumberSet',
             'src/NumberSet.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check correctness of NumberSet class', NumberSet)

NumberSetPerformance = executable('NumberSetPerformance',
             'src/NumberSetPerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check number set performance', NumberSetPerformance, timeout: 180)

OpeningHours = executable('OpeningHours',
             'src/OpeningHours.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of opening hours', OpeningHours)

OSTAndOSSCheck = executable('OSTAndOSSCheck',
             'src/OSTAndOSSCheck.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: true,
             install_dir: testInstallDir)

if buildMapCairo or buildMapQt or buildMapAgg or buildMapOpenGL
  includes = [demosIncDir, osmscoutIncDir, osmscoutmapIncDir]
  deps = [mathDep, openmpDep]
  links = [osmscout, osmscoutmap]

  if buildMapCairo
    includes += osmscoutmapcairoIncDir
    deps += pangocairoDep
    links += osmscoutmapcairo
  endif

  if buildMapQt
    includes += osmscoutmapqtIncDir
    deps += [qtMapDep]
    links += osmscoutmapqt
  endif

  if buildMapAgg
    includes += osmscoutmapaggIncDir
    deps += [mathDep, threadDep, aggDep, ftDep]
    links += osmscoutmapagg

    if aggftpicDep.found()
      deps += aggftpicDep
    else
      deps += aggftDep
    endif
  endif

  if buildMapOpenGL
    includes += osmscoutmapopenglIncDir
    deps += openGLDep
    deps += glfwDep
    deps += glewDep
    links += osmscoutmapopengl
  endif

  if buildMapGDI
    includes += osmscoutmapgdiIncDir
    deps += gdiplusDep
    links += osmscoutmapgdi
  endif

  PerformanceTest = executable('PerformanceTest',
                               'src/PerformanceTest.cpp',
                               cpp_args: ['-fPIC'],
                               include_directories: includes,
                               dependencies: deps,
                               link_with: links,
                               install: true)
endif

ReaderScannerPerformance = executable('ReaderScannerPerformance',
             'src/ReaderScannerPerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check reader scanner performance', ReaderScannerPerformance, args : [meson.current_source_dir() + '/data/testregion'])

ScanConversion = executable('ScanConversion',
             'src/ScanConversion.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check scan conversion code', ScanConversion)

ScreenBox = executable('ScreenBox',
             'src/ScreenBox.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check ScreenBox functionality', ScreenBox)

ScreenMaskTest = executable('ScreenMaskTest',
                       'src/ScreenMaskTest.cpp',
                       include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
                       dependencies: [mathDep, openmpDep],
                       link_with: [osmscoutmap, osmscout],
                       install: true,
                       install_dir: testInstallDir)

test('Check ScreenMask functionality', ScreenMaskTest)

StdByteTest = executable('StdByteTest',
             'src/StdByteTest.cpp',
             include_directories: [testIncDir],
             dependencies: [mathDep],
             install: true,
             install_dir: testInstallDir)

Signal = executable('Signal',
             'src/Signal.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check signal', Signal)

StringUtils = executable('StringUtils',
             'src/StringUtils.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check string utils', StringUtils)

ThreadedDatabase = executable('ThreadedDatabase',
             'src/ThreadedDatabase.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check threaded database', ThreadedDatabase, args : [
        '--threads', '100',
        '--iterations', '1000',
        meson.current_source_dir() + '/data/testregion',
        meson.current_source_dir() + '/../stylesheets/standard.oss'])

SunriseSunset = executable('SunriseSunset',
             'src/SunriseSunsetTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check SunriseSunset utility', SunriseSunset)

PolygonCenter = executable('PolygonCenter',
             'src/PolygonCenter.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check PolygonCenter utility', PolygonCenter)

TilingTest = executable('TilingTest',
             'src/TilingTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check tiling calculation code', TilingTest)

TransPolygon = executable('TransPolygon',
             'src/TransPolygon.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check polygon transformation code', TransPolygon)

WaterIndex = executable('WaterIndex',
             'src/WaterIndex.cpp',
             include_directories: [testIncDir, osmscoutIncDir, osmscoutimportIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout, osmscoutimport],
             install: true,
             install_dir: testInstallDir)

test('Check water index import code', WaterIndex)

WorkQueue = executable('WorkQueue',
             'src/WorkQueue.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check implementation of work queue', WorkQueue)

WStringStringConversion = executable('WStringStringConversion',
             'src/WStringStringConversion.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check WString<=>String conversion code', WStringStringConversion)

LabelPathTest = executable('LabelPathTest',
           'src/LabelPathTest.cpp',
           include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscoutmap, osmscout],
           install: true,
           install_dir: testInstallDir)

test('Check LabelPath code', LabelPathTest)

Base64Test = executable('Base64Test',
           'src/Base64.cpp',
           include_directories: [testIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscout],
           install: true,
           install_dir: testInstallDir)

test('Check Base64 code', Base64Test)

CoordBufferTest = executable('CoordBufferTest',
           'src/CoordBufferTest.cpp',
           include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscoutmap, osmscout],
           install: true,
           install_dir: testInstallDir)

test('Check coordinate buffer conversions', CoordBufferTest)

if marisaDep.found()
    TextLookupTest = executable('TextLookupTest',
               'src/TextLookupTest.cpp',
               include_directories: [osmscoutIncDir],
               dependencies: [mathDep],
               link_with: [osmscout],
               install: true,
               install_dir: testInstallDir)

   test('Check text lookup - vysoka', TextLookupTest, args : [
           '--expected-results', '5',
           meson.current_source_dir() + '/data/testregion',
           'vysoka'])

   test('Check text lookup - Vysoká', TextLookupTest, args : [
           '--expected-results', '5',
           meson.current_source_dir() + '/data/testregion',
           'Vysoká'])

   test('Check text lookup - bosyne', TextLookupTest, args : [
           '--expected-results', '1',
           meson.current_source_dir() + '/data/testregion',
           'bosyne'])
endif

Thread = executable('Thread',
             'src/Thread.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check Thread utilities', Thread)

if buildImport
    ostandossEnv = environment()
    ostandossEnv.set('TESTS_TOP_DIR', meson.current_source_dir())
    test('Check LocationService', LocationServiceTest, env: ostandossEnv)
endif

stylesheets = [
            'standard.oss',
            'winter-sports.oss',
            'boundaries.oss',
            'railways.oss',
            'motorways.oss',
            'public-transport.oss',
            'cycle.oss'
            ]

foreach stylesheet : stylesheets
    test('Check OSS/T file '+stylesheet,
            OSTAndOSSCheck,
            args : ['--warning-as-error',
                    meson.current_source_dir() + '/../stylesheets/map.ost',
                    meson.current_source_dir() + '/../stylesheets/' + stylesheet])
endforeach

if buildClientQt
    threadingMocs = qt.preprocess(moc_headers : ['include/ClientQtThreading.h'])

    ClientQtThreading = executable('ClientQtThreading',
                 'src/ClientQtThreading.cpp',
                 threadingMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutclientIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientqtIncDir],
                 dependencies: [mathDep, threadDep, openmpDep, qtClientDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclient, osmscoutclientqt],
                 install: true,
                 install_dir: testInstallDir)
endif

if buildClientQt
    testMocs = qt.preprocess(moc_headers : ['include/DownloaderTest.h'])

    QtFileDownloader = executable('QtFileDownloader',
                 'src/QtFileDownloader.cpp',
                 testMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientIncDir, osmscoutclientqtIncDir],
                 dependencies: [mathDep, threadDep, openmpDep, qtClientDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclient, osmscoutclientqt],
                 install: true,
                 install_dir: testInstallDir)
endif

