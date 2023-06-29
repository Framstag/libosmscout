osmscout2Src = [
  'src/OSMScout.cpp',
  'src/AppSettings.cpp',
  'src/Theme.cpp'
]

osmscout2IncDir = include_directories('src')

subdir('translations')

osmscout2Mocs = qt.preprocess(moc_headers : [
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
                     dependencies: [mathDep, openmpDep, qtClientDep],
                     link_with: [osmscoutgpx, osmscoutclientqt, osmscoutmapqt, osmscoutmap, osmscout],
                     install: true)

