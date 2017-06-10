subdir('src')

DumpOSS = executable('DumpOSS',
                      'src/DumpOSS.cpp',
                      include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                      dependencies: [mathDep],        
                      link_with: [osmscout, osmscoutmap],
                      install: true)

LookupText = executable('LookupText',
                        'src/LookupText.cpp',
                        include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                        dependencies: [mathDep],        
                        link_with: [osmscout, osmscoutmap],
                        install: true)                      

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

PerformanceTest = executable('PerformanceTest',
                             'src/PerformanceTest.cpp',
                             cpp_args: ['-fPIC'],
                             include_directories: [demosIncDir, osmscoutIncDir, osmscoutmapIncDir, osmscoutmapcairoIncDir, osmscoutmapqtIncDir],
                             dependencies: [mathDep, pangocairoDep, qt5WidgetsDep],        
                             link_with: [osmscout, osmscoutmap, osmscoutmapcairo, osmscoutmapqt],
                             install: true)

ResourceConsumption = executable('ResourceConsumption',
                                 'src/ResourceConsumption.cpp',
                                 include_directories: [osmscoutIncDir, osmscoutmapIncDir],
                                 dependencies: [mathDep],        
                                 link_with: [osmscout, osmscoutmap],
                                 install: true)

ResourceConsumptionQt = executable('ResourceConsumptionQt',
                                   'src/ResourceConsumptionQt.cpp',
                                   cpp_args: ['-fPIC'],
                                   include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                                   dependencies: [mathDep, qt5WidgetsDep],        
                                   link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                                   install: true)

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

# TODO: DrawMapSvg
# TODO: DrawMapAgg
# TODO: Tiler

DrawMapCairo = executable('DrawMapCairo',
                          'src/DrawMapCairo.cpp',
                          include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapcairoIncDir],
                          dependencies: [mathDep, cairoDep, pangoDep, pangocairoDep],        
                          link_with: [osmscout, osmscoutmap, osmscoutmapcairo],
                          install: true)
                          
DrawMapQt = executable('DrawMapQt',
                       'src/DrawMapQt.cpp',
                       cpp_args: ['-fPIC'],
                       include_directories: [osmscoutIncDir, osmscoutmapIncDir, osmscoutmapqtIncDir],
                       dependencies: [mathDep, qt5GuiDep, qt5WidgetsDep],        
                       link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                       install: true)                          

