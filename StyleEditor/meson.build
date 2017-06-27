styleeditorSrc = [
  'src/MainWindow.cpp',
  'src/SettingsDialog.cpp',
  'src/FileIO.cpp',
  'src/Highlighter.cpp',
  'src/StyleEditor.cpp'
]

styleeditorIncDir = include_directories('src')

styleeditorMocs = qt5.preprocess(moc_headers : ['src/MainWindow.h','src/SettingsDialog.h', 'src/FileIO.h','src/Highlighter.h'],
                                 qresources: ['StyleEditor.qrc'])

OSMScout2 = executable('StyleEditor',
                       styleeditorSrc,
                       styleeditorMocs,
                       cpp_args: ['-fPIC'],
                       include_directories: [styleeditorIncDir, osmscoutclientqtIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                       dependencies: [mathDep, qt5GuiDep, qt5WidgetsDep, qt5QmlDep, qt5QuickDep, qt5SvgDep, qt5NetworkDep, qt5LocationDep],
                       link_with: [osmscoutclientqt, osmscout],
                       install: true)

