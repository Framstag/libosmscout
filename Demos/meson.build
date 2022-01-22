subdir('src')
demoIncDir = include_directories('include')

Coverage = executable('Coverage',
                      'src/Coverage.cpp',
                      include_directories: [osmscoutIncDir],
                      dependencies: [mathDep, openmpDep],
                      link_with: [osmscout],
                      install: true)

DumpOSS = executable('DumpOSS',
                      'src/DumpOSS.cpp',
                      include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                      dependencies: [mathDep, openmpDep],
                      link_with: [osmscout, osmscoutmap],
                      install: true)

if marisaDep.found()
  LookupText = executable('LookupText',
                          'src/LookupText.cpp',
                          include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                          dependencies: [mathDep, openmpDep],
                          link_with: [osmscout, osmscoutmap],
                          install: true)
endif

LocationLookup = executable('LocationLookup',
                            'src/LocationLookup.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep, openmpDep],
                            link_with: [osmscout],
                            install: true)

LocationLookupForm = executable('LocationLookupForm',
                            'src/LocationLookupForm.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep, openmpDep],
                            link_with: [osmscout],
                            install: true)

Navigation = executable('Navigation',
                        'src/Navigation.cpp',
                        include_directories: [osmscoutIncDir],
                        dependencies: [mathDep, openmpDep],
                        link_with: [osmscout],
                        install: true)

NavigationSimulator = executable('NavigationSimulator',
                        'src/NavigationSimulator.cpp',
                        include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                        dependencies: [mathDep, openmpDep],
                        link_with: [osmscout, osmscoutmap],
                        install: true)

POILookupForm = executable('POILookupForm',
                            'src/POILookupForm.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep, openmpDep],
                            link_with: [osmscout],
                            install: true)

LocationDescription = executable('LocationDescription',
                                 'src/LocationDescription.cpp',
                                 include_directories: [osmscoutIncDir],
                                 dependencies: [mathDep, openmpDep],
                                 link_with: [osmscout],
                                 install: true)

ReverseLocationLookup = executable('ReverseLocationLookup',
                                   'src/ReverseLocationLookup.cpp',
                                   include_directories: [osmscoutIncDir],
                                   dependencies: [mathDep, openmpDep],
                                   link_with: [osmscout],
                                   install: true)

if buildMapCairo or buildMapQt or buildMapAgg or buildMapOpenGL
  includes = [demosIncDir, osmscoutIncDir, osmscoutmapIncDir]
  deps = [mathDep, openmpDep]
  links = [osmscout, osmscoutmap]

  if buildMapCairo and pangocairoDep.found()
    includes += osmscoutmapcairoIncDir
    deps += pangocairoDep
    links += osmscoutmapcairo
  endif

  if buildMapQt
    includes += osmscoutmapqtIncDir
    deps += [qt5WidgetsDep, qt5GuiDep]
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

ResourceConsumption = executable('ResourceConsumption',
                                 'src/ResourceConsumption.cpp',
                                 include_directories: [osmscoutmapIncDir, osmscoutIncDir],
                                 dependencies: [mathDep, openmpDep],
                                 link_with: [osmscout, osmscoutmap],
                                 install: true)

if buildMapQt
  ResourceConsumptionQt = executable('ResourceConsumptionQt',
                                     'src/ResourceConsumptionQt.cpp',
                                     cpp_args: ['-fPIC'],
                                     include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                     dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep],
                                     link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                     install: true)
endif

if buildMapQt
  RoutingAnimation = executable('RoutingAnimation',
                                'src/RoutingAnimation.cpp',
                                cpp_args: ['-fPIC'],
                                include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep],
                                link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                install: true)
endif

if buildClientQt

  demoMocs = qt5.preprocess('qt-demo',
                            moc_headers : ['include/QtDemoApp.h'],
                            qresources: ['demo.qrc', '../OSMScout2/res.qrc'])

  ElevationProfileChart = executable('ElevationProfileChart',
                                ['src/ElevationProfileChart.cpp',
                                 'src/QtDemoApp.cpp'],
                                demoMocs,
                                cpp_args: ['-fPIC'],
                                include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir, osmscoutclientqtIncDir, demoIncDir],
                                dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5SvgDep, qt5NetworkDep, qt5LocationDep, qt5MultimediaDep],
                                link_with: [osmscout, osmscoutmap, osmscoutmapqt, osmscoutclientqt],
                                override_options : ['unity=off'], # generated code for qt resources use static variables with the same name
                                install: true)

  if buildGpx
    navDemoMocs = qt5.preprocess(moc_headers : [
                                  'include/PositionSimulator.h',
                                  '../OSMScout2/src/Theme.h'
                                 ])

    NavigationSimulation = executable('NavigationSimulation',
                                  ['src/NavigationSimulation.cpp',
                                   'src/QtDemoApp.cpp',
                                   'src/PositionSimulator.cpp',
                                   '../OSMScout2/src/Theme.cpp'],
                                  [demoMocs, navDemoMocs],
                                  cpp_args: ['-fPIC'],
                                  include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir, osmscoutclientqtIncDir, osmscoutgpxIncDir, demoIncDir, include_directories('../OSMScout2/src')],
                                  dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5SvgDep, qt5NetworkDep, qt5LocationDep, qt5MultimediaDep],
                                  link_with: [osmscout, osmscoutmap, osmscoutmapqt, osmscoutclientqt, osmscoutgpx],
                                  override_options : ['unity=off'], # generated code for qt resources use static variables with the same name
                                  install: true)
  endif


  routingParamsDemoMocs = qt5.preprocess(moc_headers : [
                                '../OSMScout2/src/Theme.h'
                               ])

  RoutingParameters = executable('RoutingParameters',
                                ['src/RoutingParameters.cpp',
                                 'src/QtDemoApp.cpp',
                                 '../OSMScout2/src/Theme.cpp'],
                                [demoMocs, routingParamsDemoMocs],
                                cpp_args: ['-fPIC'],
                                include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir, osmscoutclientqtIncDir, demoIncDir, include_directories('../OSMScout2/src')],
                                dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5SvgDep, qt5NetworkDep, qt5LocationDep, qt5MultimediaDep],
                                link_with: [osmscout, osmscoutmap, osmscoutmapqt, osmscoutclientqt],
                                override_options : ['unity=off'], # generated code for qt resources use static variables with the same name
                                install: true)
endif

Routing = executable('Routing',
                     'src/Routing.cpp',
                     include_directories: [osmscoutIncDir],
                     dependencies: [mathDep, openmpDep],
                     link_with: [osmscout],
                     install: true)

LookupPOI = executable('LookupPOI',
                       'src/LookupPOI.cpp',
                       include_directories: [osmscoutIncDir],
                       dependencies: [mathDep, openmpDep],
                       link_with: [osmscout],
                       install: true)

Srtm = executable('Srtm',
                  'src/Srtm.cpp',
                  include_directories: [osmscoutIncDir],
                  dependencies: [mathDep, openmpDep],
                  link_with: [osmscout],
                  install: true)

if buildMapSVG
    DrawMapSVG = executable('DrawMapSVG',
                            'src/DrawMapSVG.cpp',
                            include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapsvgIncDir, demoIncDir],
                            dependencies: [mathDep, openmpDep, pangoDep],
                            link_with: [osmscout, osmscoutmap, osmscoutmapsvg],
                            install: true)
endif

if buildMapAgg
  DrawMapAgg = executable('DrawMapAgg',
                          'src/DrawMapAgg.cpp',
                          include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapaggIncDir, demoIncDir],
                          dependencies: [mathDep, openmpDep, aggDep, ftDep],
                          link_with: [osmscout, osmscoutmap, osmscoutmapagg],
                          install: true)

  Tiler = executable('Tiler',
                     'src/Tiler.cpp',
                     include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapaggIncDir],
                     dependencies: [mathDep, openmpDep, aggDep, ftDep],
                     link_with: [osmscout, osmscoutmap, osmscoutmapagg],
                     install: true)
endif

if buildMapCairo
  DrawMapCairo = executable('DrawMapCairo',
                            'src/DrawMapCairo.cpp',
                            include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapcairoIncDir, demoIncDir],
                            dependencies: [mathDep, openmpDep, cairoDep, pangoDep, pangocairoDep],
                            link_with: [osmscout, osmscoutmap, osmscoutmapcairo],
                            install: true)
endif

if buildMapQt
  DrawMapQt = executable('DrawMapQt',
                         'src/DrawMapQt.cpp',
                         cpp_args: ['-fPIC'],
                         include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir, demoIncDir],
                         dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep],
                         link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                         install: true)
endif

if buildMapOpenGL and glfwDep.found() and ftDep.found() and glewDep.found()
  DrawMapOpenGL = executable('DrawMapOpenGL',
                         'src/DrawMapOpenGL.cpp',
                         cpp_args: ['-fPIC'],
                         include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapopenglIncDir, demoIncDir],
                         dependencies: [mathDep, openmpDep, openGLDep, ftDep, glfwDep, threadDep, glewDep],
                         link_with: [osmscout, osmscoutmap, osmscoutmapopengl],
                         install: true)
endif

if buildMapDirectX
  DrawMapDirectX = executable('DrawMapDirectX',
                              'src/DrawMapDirectX.cpp',
                              include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapdirectxIncDir, demoIncDir],
                              dependencies: [mathDep, openmpDep, d2d1Dep, dwriteDep, winCodecsDep],
                              link_with: [osmscout, osmscoutmap, osmscoutmapdirectx],
                              gui_app: true,
                              install: true)
endif

if buildMapGDI
    DrawMapGDI = executable('DrawMapGDI',
                            'src/DrawMapGDI.cpp',
                            include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapgdiIncDir, demoIncDir],
                            dependencies: [mathDep, openmpDep, gdiplusDep],
                            link_with: [osmscout, osmscoutmap, osmscoutmapgdi],
                            gui_app: true,
                            install: true)
endif

if buildGpx
  GpxPipe = executable('GpxPipe',
                       'src/GpxPipe.cpp',
                       include_directories: [osmscoutIncDir, osmscoutgpxIncDir],
                       dependencies: [mathDep, openmpDep],
                       link_with: [osmscout, osmscoutgpx],
                       install: true)
endif

if buildGpx
  ElevationProfile = executable('ElevationProfile',
                                'src/ElevationProfile.cpp',
                                include_directories: [osmscoutIncDir, osmscoutgpxIncDir],
                                dependencies: [mathDep, openmpDep],
                                link_with: [osmscout, osmscoutgpx],
                                install: true)
endif
