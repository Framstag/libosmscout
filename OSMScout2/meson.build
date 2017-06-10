osmscout2Src = [
  'src/OSMScout.cpp',
  'src/AppSettings.cpp',
  'src/Theme.cpp'
]

osmscout2IncDir = include_directories('src')

mocFiles = qt5.preprocess(moc_headers : ['src/AppSettings.h', 'src/Theme.h'],
                          qresources: ['res.qrc'])

OSMScout2 = executable('OSMSCout2',
                       osmscout2Src,
                       mocFiles,
                       cpp_args: ['-fPIC'],
                       include_directories: [osmscout2IncDir, osmscoutclientqtIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                       dependencies: [mathDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5NetworkDep, qt5LocationDep],        
                       link_with: [osmscoutclientqt],
                       install: true)

