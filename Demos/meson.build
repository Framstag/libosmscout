subdir('src')

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

if buildMapCairo or buildMapQt
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
