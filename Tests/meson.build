testIncDir = include_directories('include')

testInstallDir = 'share/osmscout/tests'

AccessParse =  executable('AccessParse',
             'src/AccessParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

AsyncProcessing =  executable('AsyncProcessing',
             'src/AsyncProcessing.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

TimeParse =  executable('TimeParse',
             'src/TimeParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

Bearing = executable('Bearing',
             'src/Bearing.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

BitsAndBytesNeeded = executable('BitsAndBytesNeeded',
             'src/BitsAndBytesNeeded.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

ByteTest = executable('ByteTest',
             'src/ByteTest.cpp',
             include_directories: [testIncDir],
             dependencies: [mathDep],
             install: true,
             install_dir: testInstallDir)

CachePerformance = executable('CachePerformance',
             'src/CachePerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

CalculateResolution = executable('CalculateResolution',
             'src/CalculateResolution.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

CmdLineParsing = executable('CmdLineParsing',
             'src/CmdLineParsing.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

ColorParse = executable('ColorParse',
             'src/ColorParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

CoordinateEncoding = executable('CoordinateEncoding',
             'src/CoordinateEncoding.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

if buildMapQt
  if get_option('qtVersion') == 5
    drawtextMocs = qt5.preprocess(moc_headers : ['include/DrawWindow.h'])

    DrawTextQt = executable('DrawTextQt',
                 'src/DrawTextQt.cpp',
                 drawtextMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                 dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout],
                 install: true,
                 install_dir: testInstallDir)
  elif get_option('qtVersion') == 6
    drawtextMocs = qt6.preprocess(moc_headers : ['include/DrawWindow.h'])

    DrawTextQt = executable('DrawTextQt',
                 'src/DrawTextQt.cpp',
                 drawtextMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                 dependencies: [mathDep, openmpDep, qt6GuiDep, qt6WidgetsDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout],
                 install: true,
                 install_dir: testInstallDir)
  endif
endif

EncodeNumber = executable('EncodeNumber',
             'src/EncodeNumber.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

FeatureLabelTest = executable('FeatureLabelTest',
             'src/FeatureLabelTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

File = executable('File',
             'src/File.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

FileScannerWriter = executable('FileScannerWriter',
             'src/FileScannerWriter.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

GeoBox = executable('GeoBox',
             'src/GeoBox.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

GeoCoordParse = executable('GeoCoordParse',
             'src/GeoCoordParse.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

Geometry = executable('Geometry',
             'src/Geometry.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

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

MultiDBRouting = executable('MultiDBRouting',
             'src/MultiDBRouting.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

NumberSet = executable('NumberSet',
             'src/NumberSet.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

NumberSetPerformance = executable('NumberSetPerformance',
             'src/NumberSetPerformance.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)


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
    if get_option('qtVersion') == 5
      deps += [qt5WidgetsDep, qt5GuiDep]
    elif get_option('qtVersion') == 6
      deps += [qt6WidgetsDep, qt6GuiDep]
    endif
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

ScanConversion = executable('ScanConversion',
             'src/ScanConversion.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

ScreenBox = executable('ScreenBox',
             'src/ScreenBox.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

StdByteTest = executable('StdByteTest',
             'src/StdByteTest.cpp',
             include_directories: [testIncDir],
             dependencies: [mathDep],
             install: true,
             install_dir: testInstallDir)

StringUtils = executable('StringUtils',
             'src/StringUtils.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

ThreadedDatabase = executable('ThreadedDatabase',
             'src/ThreadedDatabase.cpp',
             include_directories: [osmscoutmapIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscoutmap, osmscout],
             install: true,
             install_dir: testInstallDir)

SunriseSunset = executable('SunriseSunset',
             'src/SunriseSunsetTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

PolygonCenter = executable('PolygonCenter',
             'src/PolygonCenter.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

TilingTest = executable('TilingTest',
             'src/TilingTest.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

TransPolygon = executable('TransPolygon',
             'src/TransPolygon.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

WaterIndex = executable('WaterIndex',
             'src/WaterIndex.cpp',
             include_directories: [testIncDir, osmscoutIncDir, osmscoutimportIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout, osmscoutimport],
             install: true,
             install_dir: testInstallDir)

WorkQueue = executable('WorkQueue',
             'src/WorkQueue.cpp',
             include_directories: [osmscoutIncDir],
             dependencies: [mathDep, threadDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

WStringStringConversion = executable('WStringStringConversion',
             'src/WStringStringConversion.cpp',
             include_directories: [testIncDir, osmscoutIncDir],
             dependencies: [mathDep, openmpDep],
             link_with: [osmscout],
             install: true,
             install_dir: testInstallDir)

LabelPathTest = executable('LabelPathTest',
           'src/LabelPathTest.cpp',
           include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscoutmap, osmscout],
           install: true,
           install_dir: testInstallDir)

Base64Test = executable('Base64Test',
           'src/Base64.cpp',
           include_directories: [testIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscout],
           install: true,
           install_dir: testInstallDir)

CoordBufferTest = executable('CoordBufferTest',
           'src/CoordBufferTest.cpp',
           include_directories: [testIncDir, osmscoutmapIncDir, osmscoutIncDir],
           dependencies: [mathDep],
           link_with: [osmscoutmap, osmscout],
           install: true,
           install_dir: testInstallDir)

if marisaDep.found()
    TextLookupTest = executable('TextLookupTest',
               'src/TextLookupTest.cpp',
               include_directories: [osmscoutIncDir],
               dependencies: [mathDep],
               link_with: [osmscout],
               install: true,
               install_dir: testInstallDir)
endif

ostandossEnv = environment()

ostandossEnv.set('TESTS_TOP_DIR', meson.current_source_dir())

test('Check parsing of access rights', AccessParse)
test('Check parsing of time string', TimeParse)
test('Check calculation of bearing', Bearing)
test('Check encoding of numbers', BitsAndBytesNeeded)
test('Check std_byte behaviour', ByteTest)
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

if marisaDep.found()
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

test('Check SunriseSunset utility', SunriseSunset)
test('Check encoding of numbers', EncodeNumber)
test('Check label formatting', FeatureLabelTest)
test('Check File utilities', File)
test('Check File access implementation', FileScannerWriter)
test('Check handling of geo box intersection', GeoBox)
test('Check parsing of geo coordinates', GeoCoordParse)
test('Check impl. of geometric functions', Geometry)
test('Check coordinate buffer conversions', CoordBufferTest)

if (compiler.get_id()=='gcc' and target_machine.system()=='windows')
  warning('Test "Check rotation of maps" does not run under MSYS because of missing locale support')
else
  test('Check rotation of maps', MapRotate)
endif

test('Check correctness of NumberSet class', NumberSet)
test('Check PolygonCenter utility', PolygonCenter)
test('Check scan conversion code', ScanConversion)
test('Check ScreenBox functionality', ScreenBox)
test('Check string utils', StringUtils)
test('Check tiling calculation code', TilingTest)
test('Check polygon transformation code', TransPolygon)
test('Check water index import code', WaterIndex)
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
  if get_option('qtVersion') == 5
    threadingMocs = qt5.preprocess(moc_headers : ['include/ClientQtThreading.h'])

    ClientQtThreading = executable('ClientQtThreading',
                 'src/ClientQtThreading.cpp',
                 threadingMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientqtIncDir],
                 dependencies: [mathDep, threadDep, openmpDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5NetworkDep, qt5MultimediaDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclientqt],
                 install: true,
                 install_dir: testInstallDir)
  elif get_option('qtVersion') == 6
    threadingMocs = qt6.preprocess(moc_headers : ['include/ClientQtThreading.h'])

    ClientQtThreading = executable('ClientQtThreading',
                 'src/ClientQtThreading.cpp',
                 threadingMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientqtIncDir],
                 dependencies: [mathDep, threadDep, openmpDep, qt6GuiDep, qt6WidgetsDep, qt6QmlDep, qt6QuickDep, qt6NetworkDep, qt6MultimediaDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclientqt],
                 override_options : ['unity=off'], # solves multiple definition of Q_DECLARE_METATYPE(osmscout::TileRef)
                 install: true,
                 install_dir: testInstallDir)
  endif
endif

if buildClientQt
  if get_option('qtVersion') == 5
    testMocs = qt5.preprocess(moc_headers : ['include/DownloaderTest.h'])

    QtFileDownloader = executable('QtFileDownloader',
                 'src/QtFileDownloader.cpp',
                 testMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientqtIncDir],
                 dependencies: [mathDep, threadDep, openmpDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5NetworkDep, qt5MultimediaDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclientqt],
                 install: true,
                 install_dir: testInstallDir)
  elif get_option('qtVersion') == 5
    testMocs = qt6.preprocess(moc_headers : ['include/DownloaderTest.h'])

    QtFileDownloader = executable('QtFileDownloader',
                 'src/QtFileDownloader.cpp',
                 testMocs,
                 include_directories: [testIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir, osmscoutclientqtIncDir],
                 dependencies: [mathDep, threadDep, openmpDep, qt6GuiDep, qt6WidgetsDep, qt6QmlDep, qt6QuickDep, qt6NetworkDep, qt6MultimediaDep],
                 link_with: [osmscoutmapqt, osmscoutmap, osmscout, osmscoutclientqt],
                 install: true,
                 install_dir: testInstallDir)
  endif
endif

