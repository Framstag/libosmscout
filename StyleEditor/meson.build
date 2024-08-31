styleeditorSrc = [
  'src/MainWindow.cpp',
  'src/SettingsDialog.cpp',
  'src/DocumentHandler.cpp',
  'src/Highlighter.cpp',
  'src/StyleEditor.cpp',
  'src/StyleAnalyser.cpp'
]

styleeditorIncDir = include_directories('src')

if haveQt6ClientDep
  qrcFile = 'StyleEditor_640.qrc'
else
  qrcFile = 'StyleEditor_515.qrc'
endif

styleeditorMocs = qt.preprocess(moc_headers : [
                                  'src/MainWindow.h',
                                  'src/SettingsDialog.h',
                                  'src/DocumentHandler.h',
                                  'src/Highlighter.h',
                                  'src/StyleAnalyser.h'
                                  ],
                               include_directories: include_directories('src'),
                               qresources: [qrcFile])

OSMScout2 = executable('StyleEditor',
                     styleeditorSrc,
                     styleeditorMocs,
                     cpp_args: ['-fPIC'],
                     include_directories: [styleeditorIncDir, osmscoutclientqtIncDir, osmscoutclientIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                     dependencies: [mathDep, openmpDep, qtClientDep],
                     link_with: [osmscoutclientqt, osmscoutclient, osmscoutmapqt, osmscoutmap, osmscout],
                     install: true)
