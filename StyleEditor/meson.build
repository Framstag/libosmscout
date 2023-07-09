styleeditorSrc = [
  'src/MainWindow.cpp',
  'src/SettingsDialog.cpp',
  'src/FileIO.cpp',
  'src/Highlighter.cpp',
  'src/StyleEditor.cpp',
  'src/StyleAnalyser.cpp'
]

styleeditorIncDir = include_directories('src')

styleeditorMocs = qt.preprocess(moc_headers : [
                                  'src/MainWindow.h',
                                  'src/SettingsDialog.h',
                                  'src/FileIO.h',
                                  'src/Highlighter.h',
                                  'src/StyleAnalyser.h'
                                  ],
                               include_directories: include_directories('src'),
                               qresources: ['StyleEditor.qrc'])

OSMScout2 = executable('StyleEditor',
                     styleeditorSrc,
                     styleeditorMocs,
                     cpp_args: ['-fPIC'],
                     include_directories: [styleeditorIncDir, osmscoutclientqtIncDir, osmscoutclientIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                     dependencies: [mathDep, openmpDep, qtClientDep],
                     link_with: [osmscoutclientqt, osmscoutclient, osmscoutmapqt, osmscoutmap, osmscout],
                     install: true)
