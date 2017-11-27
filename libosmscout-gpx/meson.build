cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_GPX_EXPORT_SYMBOLS']
  
  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscoutgpx = library('osmscout_gpx',
                      osmscoutgpxSrc,
                      include_directories: [osmscoutgpxIncDir, osmscoutIncDir],
                      cpp_args: cppArgs,
                      dependencies: [mathDep, threadDep, xml2Dep],
                      link_with: [osmscout],
                      install: true)
        
# TODO: Generate PKG_CONFIG file        
