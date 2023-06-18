subdir('src')
demoIncDir = include_directories('include')

demoInstallDir = 'share/osmscout/demos'

Coverage = executable('Coverage',
                      'src/Coverage.cpp',
                      include_directories: [osmscoutIncDir],
                      dependencies: [mathDep, openmpDep],
                      link_with: [osmscout],
                      install: true,
                      install_dir: demoInstallDir)

DumpOSS = executable('DumpOSS',
                      'src/DumpOSS.cpp',
                      include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                      dependencies: [mathDep, openmpDep],
                      link_with: [osmscout, osmscoutmap],
                      install: true,
                      install_dir: demoInstallDir)

if marisaDep.found()
  LookupText = executable('LookupText',
                          'src/LookupText.cpp',
                          include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                          dependencies: [mathDep, openmpDep],
                          link_with: [osmscout, osmscoutmap],
                          install: true,
                          install_dir: demoInstallDir)
endif

LocationLookup = executable('LocationLookup',
                            'src/LocationLookup.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep, openmpDep],
                            link_with: [osmscout],
                            install: true,
                            install_dir: demoInstallDir)

LocationLookupForm = executable('LocationLookupForm',
                            'src/LocationLookupForm.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep, openmpDep],
                            link_with: [osmscout],
                            install: true,
                            install_dir: demoInstallDir)

Navigation = executable('Navigation',
                        'src/Navigation.cpp',
                        include_directories: [osmscoutIncDir],
                        dependencies: [mathDep, openmpDep],
                        link_with: [osmscout],
                        install: true,
                        install_dir: demoInstallDir)

NavigationSimulator = executable('NavigationSimulator',
                        'src/NavigationSimulator.cpp',
                        include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                        dependencies: [mathDep, openmpDep],
                        link_with: [osmscout, osmscoutmap],
                        install: true,
                        install_dir: demoInstallDir)

POILookupForm = executable('POILookupForm',
                            'src/POILookupForm.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep, openmpDep],
                            link_with: [osmscout],
                            install: true,
                            install_dir: demoInstallDir)

LocationDescription = executable('LocationDescription',
                                 'src/LocationDescription.cpp',
                                 include_directories: [osmscoutIncDir],
                                 dependencies: [mathDep, openmpDep],
                                 link_with: [osmscout],
                                 install: true,
                                 install_dir: demoInstallDir)

ReverseLocationLookup = executable('ReverseLocationLookup',
                                   'src/ReverseLocationLookup.cpp',
                                   include_directories: [osmscoutIncDir],
                                   dependencies: [mathDep, openmpDep],
                                   link_with: [osmscout],
                                   install: true,
                                   install_dir: demoInstallDir)

ResourceConsumption = executable('ResourceConsumption',
                                 'src/ResourceConsumption.cpp',
                                 include_directories: [osmscoutmapIncDir, osmscoutIncDir],
                                 dependencies: [mathDep, openmpDep],
                                 link_with: [osmscout, osmscoutmap],
                                 install: true,
                                 install_dir: demoInstallDir)

if buildMapQt
  if get_option('qtVersion') == 5
    ResourceConsumptionQt = executable('ResourceConsumptionQt',
                                       'src/ResourceConsumptionQt.cpp',
                                       cpp_args: ['-fPIC'],
                                       include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                       dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep],
                                       link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                       install: true,
                                       install_dir: demoInstallDir)
  elif get_option('qtVersion') == 6
    ResourceConsumptionQt = executable('ResourceConsumptionQt',
                                       'src/ResourceConsumptionQt.cpp',
                                       cpp_args: ['-fPIC'],
                                       include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                       dependencies: [mathDep, openmpDep, qt6GuiDep, qt6WidgetsDep],
                                       link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                       install: true,
                                       install_dir: demoInstallDir)
  endif
endif

if buildMapQt
  if get_option('qtVersion') == 5
    RoutingAnimation = executable('RoutingAnimation',
                                  'src/RoutingAnimation.cpp',
                                  cpp_args: ['-fPIC'],
                                  include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                  dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep],
                                  link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                  install: true,
                                  install_dir: demoInstallDir)
  elif get_option('qtVersion') == 6
    RoutingAnimation = executable('RoutingAnimation',
                                  'src/RoutingAnimation.cpp',
                                  cpp_args: ['-fPIC'],
                                  include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                  dependencies: [mathDep, openmpDep, qt6GuiDep, qt6WidgetsDep],
                                  link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                  install: true,
                                  install_dir: demoInstallDir)
  endif

endif

if buildClientQt
  if get_option('qtVersion') == 5
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
                                  install: true,
                                  install_dir: demoInstallDir)
  elif get_option('qtVersion') == 6
    demoMocs = qt6.preprocess('qt-demo',
                              moc_headers : ['include/QtDemoApp.h'],
                              qresources: ['demo.qrc', '../OSMScout2/res.qrc'])

    ElevationProfileChart = executable('ElevationProfileChart',
                                  ['src/ElevationProfileChart.cpp',
                                   'src/QtDemoApp.cpp'],
                                  demoMocs,
                                  cpp_args: ['-fPIC'],
                                  include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir, osmscoutclientqtIncDir, demoIncDir],
                                  dependencies: [mathDep, openmpDep, qt6GuiDep, qt6WidgetsDep, qt6QmlDep, qt6QuickDep, qt6SvgDep, qt6NetworkDep, qt6LocationDep, qt6MultimediaDep],
                                  link_with: [osmscout, osmscoutmap, osmscoutmapqt, osmscoutclientqt],
                                  override_options : ['unity=off'], # generated code for qt resources use static variables with the same name
                                  install: true,
                                  install_dir: demoInstallDir)
  endif

  if buildGpx
    if get_option('qtVersion') == 5
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
                                    install: true,
                                    install_dir: demoInstallDir)
    elif get_option('qtVersion') == 6
      navDemoMocs = qt6.preprocess(moc_headers : [
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
                                    dependencies: [mathDep, openmpDep, qt6GuiDep, qt6WidgetsDep, qt6QmlDep, qt6QuickDep, qt6SvgDep, qt6NetworkDep, qt6LocationDep, qt6MultimediaDep],
                                    link_with: [osmscout, osmscoutmap, osmscoutmapqt, osmscoutclientqt, osmscoutgpx],
                                    override_options : ['unity=off'], # generated code for qt resources use static variables with the same name
                                    install: true,
                                    install_dir: demoInstallDir)
    endif
  endif


  if get_option('qtVersion') == 5
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
                                  install: true,
                                  install_dir: demoInstallDir)
  elif get_option('qtVersion') == 6
    routingParamsDemoMocs = qt6.preprocess(moc_headers : [
                                  '../OSMScout2/src/Theme.h'
                                 ])

    RoutingParameters = executable('RoutingParameters',
                                  ['src/RoutingParameters.cpp',
                                   'src/QtDemoApp.cpp',
                                   '../OSMScout2/src/Theme.cpp'],
                                  [demoMocs, routingParamsDemoMocs],
                                  cpp_args: ['-fPIC'],
                                  include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir, osmscoutclientqtIncDir, demoIncDir, include_directories('../OSMScout2/src')],
                                  dependencies: [mathDep, openmpDep, qt6GuiDep, qt6WidgetsDep, qt6QmlDep, qt6QuickDep, qt6SvgDep, qt6NetworkDep, qt6LocationDep, qt6MultimediaDep],
                                  link_with: [osmscout, osmscoutmap, osmscoutmapqt, osmscoutclientqt],
                                  override_options : ['unity=off'], # generated code for qt resources use static variables with the same name
                                  install: true,
                                  install_dir: demoInstallDir)
  endif
endif

Routing = executable('Routing',
                     'src/Routing.cpp',
                     include_directories: [osmscoutIncDir],
                     dependencies: [mathDep, openmpDep],
                     link_with: [osmscout],
                     install: true,
                     install_dir: demoInstallDir)

LookupPOI = executable('LookupPOI',
                       'src/LookupPOI.cpp',
                       include_directories: [osmscoutIncDir],
                       dependencies: [mathDep, openmpDep],
                       link_with: [osmscout],
                       install: true,
                       install_dir: demoInstallDir)

Srtm = executable('Srtm',
                  'src/Srtm.cpp',
                  include_directories: [osmscoutIncDir],
                  dependencies: [mathDep, openmpDep],
                  link_with: [osmscout],
                  install: true,
                  install_dir: demoInstallDir)

if buildMapSVG
    DrawMapSVG = executable('DrawMapSVG',
                            'src/DrawMapSVG.cpp',
                            include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapsvgIncDir, demoIncDir],
                            dependencies: [mathDep, openmpDep, pangoDep],
                            link_with: [osmscout, osmscoutmap, osmscoutmapsvg],
                            install: true,
                            install_dir: demoInstallDir)
endif

if buildMapAgg
  DrawMapAgg = executable('DrawMapAgg',
                          'src/DrawMapAgg.cpp',
                          include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapaggIncDir, demoIncDir],
                          dependencies: [mathDep, openmpDep, aggDep, ftDep],
                          link_with: [osmscout, osmscoutmap, osmscoutmapagg],
                          install: true,
                          install_dir: demoInstallDir)

  Tiler = executable('Tiler',
                     'src/Tiler.cpp',
                     include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapaggIncDir],
                     dependencies: [mathDep, openmpDep, aggDep, ftDep],
                     link_with: [osmscout, osmscoutmap, osmscoutmapagg],
                     install: true,
                     install_dir: demoInstallDir)
endif

if buildMapCairo
  DrawMapCairo = executable('DrawMapCairo',
                            'src/DrawMapCairo.cpp',
                            include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapcairoIncDir, demoIncDir],
                            dependencies: [mathDep, openmpDep, cairoDep, pangoDep, pangocairoDep],
                            link_with: [osmscout, osmscoutmap, osmscoutmapcairo],
                            install: true,
                            install_dir: demoInstallDir)
endif

if buildMapQt
  if get_option('qtVersion') == 5
    DrawMapQt = executable('DrawMapQt',
                           'src/DrawMapQt.cpp',
                           cpp_args: ['-fPIC'],
                           include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir, demoIncDir],
                           dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep],
                           link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                           install: true,
                           install_dir: demoInstallDir)
  elif get_option('qtVersion') == 6
    DrawMapQt = executable('DrawMapQt',
                           'src/DrawMapQt.cpp',
                           cpp_args: ['-fPIC'],
                           include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir, demoIncDir],
                           dependencies: [mathDep, openmpDep, qt6GuiDep, qt6WidgetsDep],
                           link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                           install: true,
                           install_dir: demoInstallDir)
  endif
endif

if buildMapOpenGL and glfwDep.found() and ftDep.found() and glewDep.found()
  DrawMapOpenGL = executable('DrawMapOpenGL',
                         'src/DrawMapOpenGL.cpp',
                         cpp_args: ['-fPIC'],
                         include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapopenglIncDir, demoIncDir],
                         dependencies: [mathDep, openmpDep, openGLDep, ftDep, glfwDep, threadDep, glewDep],
                         link_with: [osmscout, osmscoutmap, osmscoutmapopengl],
                         install: true,
                         install_dir: demoInstallDir)
endif

if buildMapDirectX
  DrawMapDirectX = executable('DrawMapDirectX',
                              'src/DrawMapDirectX.cpp',
                              include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapdirectxIncDir, demoIncDir],
                              dependencies: [mathDep, openmpDep, d2d1Dep, dwriteDep, winCodecsDep],
                              link_with: [osmscout, osmscoutmap, osmscoutmapdirectx],
                              win_subsystem: 'windows',
                              install: true,
                              install_dir: demoInstallDir)
endif

if buildMapGDI
    DrawMapGDI = executable('DrawMapGDI',
                            'src/DrawMapGDI.cpp',
                            include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapgdiIncDir, demoIncDir],
                            dependencies: [mathDep, openmpDep, gdiplusDep],
                            link_with: [osmscout, osmscoutmap, osmscoutmapgdi],
                            win_subsystem: 'windows',
                            install: true,
                            install_dir: demoInstallDir)
endif

if buildGpx
  GpxPipe = executable('GpxPipe',
                       'src/GpxPipe.cpp',
                       include_directories: [osmscoutIncDir, osmscoutgpxIncDir],
                       dependencies: [mathDep, openmpDep],
                       link_with: [osmscout, osmscoutgpx],
                       install: true,
                       install_dir: demoInstallDir)
endif

if buildGpx
  ElevationProfile = executable('ElevationProfile',
                                'src/ElevationProfile.cpp',
                                include_directories: [osmscoutIncDir, osmscoutgpxIncDir],
                                dependencies: [mathDep, openmpDep],
                                link_with: [osmscout, osmscoutgpx],
                                install: true,
                                install_dir: demoInstallDir)
endif
