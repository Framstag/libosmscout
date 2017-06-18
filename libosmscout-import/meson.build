cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_IMPORT_EXPORT_SYMBOLS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')
subdir('src/protobuf')

osmscoutimportSrc = [osmscoutimportSrc]

if protobufDep.found() and protocCmd.found()
   osmscoutimportSrc += protoSrc
endif

osmscoutimport = library('osmscoutimport',
                         osmscoutimportSrc,
                         include_directories: [osmscoutimportIncDir, osmscoutIncDir],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep, wsock32Dep, xml2Dep, marisaDep, protobufDep, zlibDep],
                         link_with: [osmscout],
                         install: true)

# TODO: Generate PKG_CONFIG file
