osmscout2Src = [
  'src/OSMScout.cpp',
  'src/AppSettings.cpp',
  'src/Theme.cpp'
]

osmscout2IncDir = include_directories('src')

subdir('translations')

if get_option('qtVersion') == 5
  osmscout2Mocs = qt5.preprocess(moc_headers : [
                                  'src/AppSettings.h',
                                  'src/Theme.h'
                                 ],
                                 include_directories: include_directories('src'),
                                 qresources: ['res.qrc'])


  OSMScout2 = executable('OSMScout2',
                         osmscout2Src,
                         osmscout2Mocs,
                         cpp_args: ['-fPIC'],
                         include_directories: [osmscout2IncDir, osmscoutgpxIncDir, osmscoutclientqtIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                         dependencies: [mathDep, openmpDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5SvgDep, qt5NetworkDep, qt5LocationDep, qt5MultimediaDep],
                         link_with: [osmscoutgpx, osmscoutclientqt, osmscoutmapqt, osmscoutmap, osmscout],
                         install: true)
elif get_option('qtVersion') == 6
  osmscout2Mocs = qt6.preprocess(moc_headers : [
                                  'src/AppSettings.h',
                                  'src/Theme.h'
                                 ],
                                 include_directories: include_directories('src'),
                                 qresources: ['res.qrc'])


  OSMScout2 = executable('OSMScout2',
                         osmscout2Src,
                         osmscout2Mocs,
                         cpp_args: ['-fPIC'],
                         include_directories: [osmscout2IncDir, osmscoutgpxIncDir, osmscoutclientqtIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                         dependencies: [mathDep, openmpDep, qt6GuiDep, qt6WidgetsDep, qt6QmlDep, qt6QuickDep, qt6SvgDep, qt6NetworkDep, qt6LocationDep, qt6MultimediaDep],
                         link_with: [osmscoutgpx, osmscoutclientqt, osmscoutmapqt, osmscoutmap, osmscout],
                         install: true)
endif

