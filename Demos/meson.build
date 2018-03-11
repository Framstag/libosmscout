subdir('src')

Coverage = executable('Coverage',
                      'src/Coverage.cpp',
                      include_directories: [osmscoutIncDir],
                      dependencies: [mathDep],
                      link_with: [osmscout],
                      install: true)

DumpOSS = executable('DumpOSS',
                      'src/DumpOSS.cpp',
                      include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                      dependencies: [mathDep],
                      link_with: [osmscout, osmscoutmap],
                      install: true)

if marisaDep.found()
  LookupText = executable('LookupText',
                          'src/LookupText.cpp',
                          include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                          dependencies: [mathDep],
                          link_with: [osmscout, osmscoutmap],
                          install: true)
endif

LocationLookup = executable('LocationLookup',
                            'src/LocationLookup.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep],
                            link_with: [osmscout],
                            install: true)

LocationLookupForm = executable('LocationLookupForm',
                            'src/LocationLookupForm.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep],
                            link_with: [osmscout],
                            install: true)

Navigation = executable('Navigation',
                        'src/Navigation.cpp',
                        include_directories: [osmscoutIncDir],
                        dependencies: [mathDep],
                        link_with: [osmscout],
                        install: true)

POILookupForm = executable('POILookupForm',
                            'src/POILookupForm.cpp',
                            include_directories: [osmscoutIncDir],
                            dependencies: [mathDep],
                            link_with: [osmscout],
                            install: true)

LocationDescription = executable('LocationDescription',
                                 'src/LocationDescription.cpp',
                                 include_directories: [osmscoutIncDir],
                                 dependencies: [mathDep],
                                 link_with: [osmscout],
                                 install: true)

ReverseLocationLookup = executable('ReverseLocationLookup',
                                   'src/ReverseLocationLookup.cpp',
                                   include_directories: [osmscoutIncDir],
                                   dependencies: [mathDep],
                                   link_with: [osmscout],
                                   install: true)

if buildMapCairo or buildMapQt or buildMapAgg or buildMapOpenGL
  includes = [demosIncDir, osmscoutIncDir, osmscoutmapIncDir]
  deps = [mathDep]
  links = [osmscout, osmscoutmap]

  if buildMapCairo and pangocairoDep.found()
    includes += osmscoutmapcairoIncDir
    deps += pangocairoDep
    links += osmscoutmapcairo
  endif

  if buildMapQt
    includes += osmscoutmapqtIncDir
    deps += qt5WidgetsDep
    links += osmscoutmapqt
  endif

  if buildMapAgg
    includes += osmscoutmapaggIncDir
    deps += aggDep
    links += osmscoutmapagg
  endif

  # TODO: This build is not working, maybe a missing dependency?... (cmake counterpart works)
  if buildMapOpenGL
    includes += osmscoutmapopenglIncDir
    deps += openGLDep
    deps += glfwDep
    links += osmscoutmapopengl
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
                                 dependencies: [mathDep],
                                 link_with: [osmscout, osmscoutmap],
                                 install: true)

if buildMapQt
  ResourceConsumptionQt = executable('ResourceConsumptionQt',
                                     'src/ResourceConsumptionQt.cpp',
                                     cpp_args: ['-fPIC'],
                                     include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                     dependencies: [mathDep, qt5GuiDep, qt5WidgetsDep],
                                     link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                     install: true)
endif

if buildMapQt
  RoutingAnimation = executable('RoutingAnimation',
                                'src/RoutingAnimation.cpp',
                                cpp_args: ['-fPIC'],
                                include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                dependencies: [mathDep, qt5GuiDep, qt5WidgetsDep],
                                link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                install: true)
endif

Routing = executable('Routing',
                     'src/Routing.cpp',
                     include_directories: [osmscoutIncDir],
                     dependencies: [mathDep],
                     link_with: [osmscout],
                     install: true)

LookupPOI = executable('LookupPOI',
                       'src/LookupPOI.cpp',
                       include_directories: [osmscoutIncDir],
                       dependencies: [mathDep],
                       link_with: [osmscout],
                       install: true)

Srtm = executable('Srtm',
                  'src/Srtm.cpp',
                  include_directories: [osmscoutIncDir],
                  dependencies: [mathDep],
                  link_with: [osmscout],
                  install: true)

DrawMapSVG = executable('DrawMapSVG',
                        'src/DrawMapSVG.cpp',
                        include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapsvgIncDir],
                        dependencies: [mathDep, pangoDep],
                        link_with: [osmscout, osmscoutmap, osmscoutmapsvg],
                        install: true)

if buildMapAgg
  DrawMapAgg = executable('DrawMapAgg',
                          'src/DrawMapAgg.cpp',
                          include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapaggIncDir],
                          dependencies: [mathDep, aggDep, ftDep],
                          link_with: [osmscout, osmscoutmap, osmscoutmapagg],
                          install: true)

  Tiler = executable('Tiler',
                     'src/Tiler.cpp',
                     include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapaggIncDir],
                     dependencies: [mathDep, aggDep, ftDep],
                     link_with: [osmscout, osmscoutmap, osmscoutmapagg],
                     install: true)
endif

if buildMapCairo
  DrawMapCairo = executable('DrawMapCairo',
                            'src/DrawMapCairo.cpp',
                            include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapcairoIncDir],
                            dependencies: [mathDep, cairoDep, pangoDep, pangocairoDep],
                            link_with: [osmscout, osmscoutmap, osmscoutmapcairo],
                            install: true)
endif

if buildMapQt
  DrawMapQt = executable('DrawMapQt',
                         'src/DrawMapQt.cpp',
                         cpp_args: ['-fPIC'],
                         include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                         dependencies: [mathDep, qt5GuiDep, qt5WidgetsDep],
                         link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                         install: true)
endif

if buildMapOpenGL and glfwDep.found() and ftDep.found()
  DrawMapOpenGL = executable('DrawMapOpenGL',
                         'src/DrawMapOpenGL.cpp',
                         cpp_args: ['-fPIC'],
                         include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapopenglIncDir],
                         dependencies: [mathDep, openGLDep, ftDep, glfwDep],
                         link_with: [osmscout, osmscoutmap, osmscoutmapopengl],
                         install: true)
endif
