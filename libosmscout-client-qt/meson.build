cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutClientQt_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutclientqt')
subdir('include/osmscoutclientqt/private')
subdir('src')

mocHeaders = []

headerTemplate='@0@/@1@'
foreach hdr : osmscoutclientqtHeader
  mocHeaders += headerTemplate.format('include/',hdr)
endforeach

if get_option('qtVersion') == 5
  mocFiles = qt5.preprocess(moc_headers : mocHeaders,
                            include_directories: include_directories('include'))

  osmscoutclientqt = library('osmscout_client_qt',
                             mocFiles,
                             osmscoutclientqtSrc,
                             include_directories: [osmscoutclientqtIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                             cpp_args: cppArgs,
                             dependencies: [mathDep, threadDep, qt5GuiDep, qt5QmlDep, qt5QuickDep, qt5WidgetsDep, qt5NetworkDep, qt5SvgDep, qt5MultimediaDep],
                             link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                             version: libraryVersion,
                             install: true)
elif get_option('qtVersion') == 6
  mocFiles = qt6.preprocess(moc_headers : mocHeaders,
                            include_directories: include_directories('include'))

  osmscoutclientqt = library('osmscout_client_qt',
                             mocFiles,
                             osmscoutclientqtSrc,
                             include_directories: [osmscoutclientqtIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                             cpp_args: cppArgs,
                             dependencies: [mathDep, threadDep, qt6GuiDep, qt6QmlDep, qt6QuickDep, qt6WidgetsDep, qt6NetworkDep, qt6SvgDep, qt6MultimediaDep],
                             link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                             version: libraryVersion,
                             install: true)
endif

# TODO: Generate PKG_CONFIG file
