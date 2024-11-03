testIncDir = include_directories('include')

testInstallDir = 'share/osmscout/tests'

AccessParseTest =  executable('AccessParseTest',
             'src/AccessParseTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of access rights', AccessParseTest)

AsyncProcessingTest =  executable('AsyncProcessingTest',
             'src/AsyncProcessingTest.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check asynchronous processing', AsyncProcessingTest, timeout: 180)

AsyncWorkerTest =  executable('AsyncWorkerTest',
             'src/AsyncWorkerTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check asynchronous worker', AsyncWorkerTest)

Base64Test = executable('Base64Test',
                        'src/Base64Test.cpp',
                        include_directories: [testIncDir, osmscoutIncDir],
                        dependencies: [mathDep, catch2MainDep],
                        link_with: [osmscout],
                        install: true,
                        install_dir: testInstallDir)

test('Check Base64 code', Base64Test)

BearingTest = executable('BearingTest',
             'src/BearingTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check calculation of bearing', BearingTest)

BitsAndBytesNeededTest = executable('BitsAndBytesNeededTest',
             'src/BitsAndBytesNeededTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check encoding of numbers', BitsAndBytesNeededTest)

ByteTest = executable('ByteTest',
             'src/ByteTest.cpp',
             include_directories: [testIncDir],
             dependencies: [mathDep, catch2MainDep],
             install: true,
             install_dir: testInstallDir)

test('Check std_byte behaviour', ByteTest)

CachePerformanceTest = executable('CachePerformanceTest',
             'src/CachePerformanceTest.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check cache functionality with CachePerformance', CachePerformanceTest, args : ['--size', '1000'])

CalculateResolutionTest = executable('CalculateResolutionTest',
             'src/CalculateResolutionTest.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check position accuracy with coordinate bits', CalculateResolutionTest)

CancelableFutureTest = executable('CancelableFutureTest',
             'src/CancelableFutureTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check cancelable future', CancelableFutureTest)

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

CmdLineParsingTest = executable('CmdLineParsingTest',
             'src/CmdLineParsingTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of command line args', CmdLineParsingTest)

ColorParseTest = executable('ColorParseTest',
             'src/ColorParseTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of colors', ColorParseTest)

CoordBufferTest = executable('CoordBufferTest',
                             'src/CoordBufferTest.cpp',
                             include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
                             dependencies: [mathDep, catch2MainDep],
                             link_with: [osmscoutmap, osmscout],
                             install: true,
                             install_dir: testInstallDir)

test('Check coordinate buffer conversions', CoordBufferTest)

CoordinateEncodingTest = executable('CoordinateEncodingTest',
             'src/CoordinateEncodingTest.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of ways.dat', CoordinateEncodingTest, args : [meson.current_source_dir() + '/data/testregion'])

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

EncodeNumberTest = executable('EncodeNumberTest',
             'src/EncodeNumberTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check encoding of numbers', EncodeNumberTest)

FeatureLabelTest = executable('FeatureLabelTest',
             'src/FeatureLabelTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check label formatting', FeatureLabelTest)

FileFormatVersionTest = executable('FileFormatVersionTest',
                               'src/FileFormatVersionTest.cpp',
                               include_directories: [testIncDir, osmscoutIncDir],
                               dependencies: [mathDep, openmpDep, catch2MainDep],
                               link_with: [osmscout],
                               install: true,
                               install_dir: testInstallDir)

test('Check FileFormatVersion APIs',
     FileFormatVersionTest,
     env: ['TESTS_TOP_DIR='+meson.current_source_dir()])

FileScannerWriterTest = executable('FileScannerWriterTest',
                               'src/FileScannerWriterTest.cpp',
                               include_directories: [testIncDir, osmscoutIncDir],
                               dependencies: [mathDep, openmpDep, catch2MainDep],
                               link_with: [osmscout],
                               install: true,
                               install_dir: testInstallDir)

test('Check File access implementation', FileScannerWriterTest)

FileTest = executable('FileTest',
             'src/FileTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check File utilities', FileTest)

GeoBoxTest = executable('GeoBoxTest',
             'src/GeoBoxTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check handling of geo box intersection', GeoBoxTest)

GeoCoordParseTest = executable('GeoCoordParseTest',
             'src/GeoCoordParseTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of geo coordinates', GeoCoordParseTest)

GeometryTest = executable('GeometryTest',
             'src/GeometryTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check impl. of geometric functions', GeometryTest)

headerCheckEnv = environment()
headerCheckEnv.set('SOURCE_ROOT', meson.project_source_root())

HeaderCheckTest = executable('HeaderCheckTest',
             'src/HeaderCheckTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check use of \'<\'...\'>\' for includes', HeaderCheckTest, env: headerCheckEnv)

LabelPathTest = executable('LabelPathTest',
                           'src/LabelPathTest.cpp',
                           include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           dependencies: [mathDep, catch2MainDep],
                           link_with: [osmscoutmap, osmscout],
                           install: true,
                           install_dir: testInstallDir)

test('Check LabelPath code', LabelPathTest)

LatchTest = executable('LatchTest',
             'src/LatchTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check latch consistency', LatchTest)

if buildImport
    LocationServiceTest = executable('LocationServiceTest',
                 [
                   'src/LocationServiceTest.cpp',
                   'src/SearchForLocationByStringTest.cpp',
                   'src/SearchForLocationByFormTest.cpp',
                   'src/SearchForPOIByFormTest.cpp'
                 ],
                 include_directories: [testIncDir, osmscouttestIncDir, osmscoutimportIncDir, osmscoutIncDir],
                 dependencies: [mathDep, openmpDep, catch2Dep],
                 link_with: [osmscouttest, osmscoutimport, osmscout],
                 install: true,
                 install_dir: testInstallDir)

    ostandossEnv = environment()
    ostandossEnv.set('TESTS_TOP_DIR', meson.current_source_dir())
    test('Check LocationService', LocationServiceTest, env: ostandossEnv)
endif

MapRotateTest = executable('MapRotateTest',
             'src/MapRotateTest.cpp',
             include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscoutmap, osmscout],
             install: true,
             install_dir: testInstallDir)

if (compiler.get_id()=='gcc' and target_machine.system()=='windows')
  warning('Test "Check rotation of maps" does not run under MSYS because of missing locale support')
else
  test('Check rotation of maps', MapRotateTest)
endif

MercatorProjectionTest = executable('MercatorProjectionTest',
             'src/MercatorProjectionTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check MercatorProjection', MercatorProjectionTest)

MultiDBRoutingTest = executable('MultiDBRoutingTest',
             'src/MultiDBRoutingTest.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check routing', MultiDBRoutingTest, args : ['50.412', '14.534', '50.424', '14.6013', meson.current_source_dir() + '/data/testregion'])

NumberSetPerformanceTest = executable('NumberSetPerformanceTest',
                                  'src/NumberSetPerformanceTest.cpp',
                                  include_directories: [osmscoutIncDir],
                                  dependencies: [mathDep, openmpDep],
                                  link_with: [osmscout],
                                  install: true,
                                  install_dir: testInstallDir)

test('Check number set performance', NumberSetPerformanceTest, timeout: 180)

NumberSetTest = executable('NumberSetTest',
             'src/NumberSetTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check correctness of NumberSet class', NumberSetTest)

OpeningHoursTest = executable('OpeningHoursTest',
             'src/OpeningHoursTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check parsing of opening hours', OpeningHoursTest)

OSTAndOSSTest = executable('OSTAndOSSTest',
             'src/OSTAndOSSTest.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: true,
             install_dir: testInstallDir)

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
       OSTAndOSSTest,
       args : ['--warning-as-error',
               meson.current_source_dir() + '/../stylesheets/map.ost',
               meson.current_source_dir() + '/../stylesheets/' + stylesheet])
endforeach

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
    deps += glmDep
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

PolygonCenterTest = executable('PolygonCenterTest',
                           'src/PolygonCenterTest.cpp',
                           include_directories: [testIncDir, osmscoutIncDir],
                           dependencies: [mathDep, threadDep, openmpDep, catch2MainDep],
                           link_with: [osmscout],
                           install: true,
                           install_dir: testInstallDir)

test('Check PolygonCenter utility', PolygonCenterTest)

if buildClientQt
  testMocs = qt.preprocess(moc_headers : ['include/DownloaderTest.h'])

  QtFileDownloaderTest = executable('QtFileDownloaderTest',
                                'src/QtFileDownloaderTest.cpp',
                                testMocs,
                                include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientIncDir, osmscoutclientqtIncDir],
                                dependencies: [mathDep, threadDep, openmpDep, qtClientDep],
                                link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclient, osmscoutclientqt],
                                install: true,
                                install_dir: testInstallDir)
endif

ReaderScannerPerformanceTest = executable('ReaderScannerPerformanceTest',
             'src/ReaderScannerPerformanceTest.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check reader scanner performance', ReaderScannerPerformanceTest, args : [meson.current_source_dir() + '/data/testregion'])

RoutePostprocessorTest = executable('RoutePostprocessorTest',
                            'src/RoutePostprocessorTest.cpp',
                            include_directories: [testIncDir, osmscoutIncDir],
                            dependencies: [mathDep, openmpDep, catch2MainDep],
                            link_with: [osmscout],
                            install: true,
                            install_dir: testInstallDir)

test('Check route postprocessor', RoutePostprocessorTest)

ScanConversionTest = executable('ScanConversionTest',
             'src/ScanConversionTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check scan conversion code', ScanConversionTest)

ScreenBoxTest = executable('ScreenBoxTest',
             'src/ScreenBoxTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check ScreenBox functionality', ScreenBoxTest)

ScreenMaskTest = executable('ScreenMaskTest',
                       'src/ScreenMaskTest.cpp',
                       include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
                       dependencies: [mathDep, openmpDep, catch2MainDep],
                       link_with: [osmscoutmap, osmscout],
                       install: true,
                       install_dir: testInstallDir)

test('Check ScreenMask functionality', ScreenMaskTest)

SignalTest = executable('SignalTest',
                    'src/SignalTest.cpp',
                    include_directories: [testIncDir, osmscoutIncDir],
                    dependencies: [catch2MainDep],
                    link_with: [osmscout],
                    install: true,
                    install_dir: testInstallDir)

test('Check signal', SignalTest)

StdByteTest = executable('StdByteTest',
             'src/StdByteTest.cpp',
             include_directories: [testIncDir],
             dependencies: [mathDep, catch2MainDep],
             install: true,
             install_dir: testInstallDir)

StringUtilsTest = executable('StringUtilsTest',
             'src/StringUtilsTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check string utils', StringUtilsTest)

SunriseSunsetTest = executable('SunriseSunsetTest',
                           'src/SunriseSunsetTest.cpp',
                           include_directories: [testIncDir, osmscoutIncDir],
                           dependencies: [mathDep, threadDep, openmpDep, catch2MainDep],
                           link_with: [osmscout],
                           install: true,
                           install_dir: testInstallDir)

test('Check SunriseSunset utility', SunriseSunsetTest)

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

ThreadedDatabaseTest = executable('ThreadedDatabaseTest',
             'src/ThreadedDatabaseTest.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check threaded database', ThreadedDatabaseTest, args : [
        '--threads', '100',
        '--iterations', '1000',
        meson.current_source_dir() + '/data/testregion',
        meson.current_source_dir() + '/../stylesheets/standard.oss'])

ThreadTest = executable('ThreadTest',
                    'src/ThreadTest.cpp',
                    include_directories: [testIncDir, osmscoutIncDir],
                    dependencies: [mathDep, threadDep, openmpDep, catch2MainDep],
                    link_with: [osmscout],
                    install: true,
                    install_dir: testInstallDir)

if (compiler.get_id()=='gcc' and target_machine.system()=='windows')
    warning('Test "Check Thread utilities" does not run under MSYS because of partial support for thread local')
else
    test('Check Thread utilities', ThreadTest)
endif

TilingTest = executable('TilingTest',
                        'src/TilingTest.cpp',
                        include_directories: [testIncDir, osmscoutIncDir],
                        dependencies: [mathDep, threadDep, openmpDep, catch2MainDep],
                        link_with: [osmscout],
                        install: true,
                        install_dir: testInstallDir)

test('Check tiling calculation code', TilingTest)

TimeParseTest =  executable('TimeParseTest',
                        'src/TimeParseTest.cpp',
                        include_directories: [testIncDir, osmscoutIncDir],
                        dependencies: [mathDep, openmpDep, catch2MainDep],
                        link_with: [osmscout],
                        install: true,
                        install_dir: testInstallDir)

test('Check parsing of time string', TimeParseTest)

TransPolygonTest = executable('TransPolygonTest',
             'src/TransPolygonTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check polygon transformation code', TransPolygonTest)

WaterIndexTest = executable('WaterIndexTest',
             'src/WaterIndexTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir, osmscoutimportIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout, osmscoutimport],
             install: true,
             install_dir: testInstallDir)

test('Check water index import code', WaterIndexTest)

WorkQueueTest = executable('WorkQueueTest',
             'src/WorkQueueTest.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check implementation of work queue', WorkQueueTest, timeout: 180)

WStringStringConversionTest = executable('WStringStringConversionTest',
             'src/WStringStringConversionTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep, catch2MainDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

test('Check WString<=>String conversion code', WStringStringConversionTest)


