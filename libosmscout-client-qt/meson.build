cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_CLIENT_QT_EXPORT_SYMBOLS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

mocHeaders = []

headerTemplate='@0@/@1@'
foreach hdr : osmscoutclientqtHeader
  mocHeaders += headerTemplate.format('include/',hdr)
endforeach  

mocFiles = qt5.preprocess(moc_headers : mocHeaders)

osmscoutclientqt = library('osmscout_client_qt',
                           mocFiles,
                           osmscoutclientqtSrc,
                           include_directories: [osmscoutclientqtIncDir, osmscoutmapqtIncDir, osmscoutmapIncDir, osmscoutIncDir],
                           cpp_args: cppArgs,
                           dependencies: [mathDep, threadDep, qt5GuiDep, qt5QmlDep, qt5QuickDep, qt5WidgetsDep, qt5NetworkDep],
                           link_with: [osmscout, osmscoutmap, osmscoutmapqt],
                           install: true)
        
# TODO: Generate PKG_CONFIG file        
