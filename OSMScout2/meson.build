osmscout2Src = [
  'src/OSMScout.cpp',
  'src/AppSettings.cpp',
  'src/Theme.cpp'
]

osmscout2IncDir = include_directories('src')

osmscout2Mocs = qt5.preprocess(moc_headers : ['src/AppSettings.h', 'src/Theme.h'],
                               qresources: ['res.qrc'])

OSMScout2 = executable('OSMScout2',
                       osmscout2Src,
                       osmscout2Mocs,
                       cpp_args: ['-fPIC'],
                       include_directories: [osmscout2IncDir, osmscoutclientqtIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                       dependencies: [mathDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5SvgDep, qt5NetworkDep, qt5LocationDep],        
                       link_with: [osmscoutclientqt, osmscoutmapqt, osmscoutmap, osmscout],
                       install: true)

