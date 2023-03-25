cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutImport_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutimport')
subdir('include/osmscoutimport/private')
subdir('src')
subdir('src/protobuf')

osmscoutimportSrc = [osmscoutimportSrc]

if protobufDep.found() and protocCmd.found()
   osmscoutimportSrc += protoSrc
endif

osmscoutimport = library('osmscout_import',
                         osmscoutimportSrc,
                         include_directories: [osmscoutimportIncDir, osmscoutIncDir],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep, tbbDep, openmpDep, wsock32Dep, xml2Dep, marisaDep, protobufDep, zlibDep],
                         link_with: [osmscout],
                         version: libraryVersion,
                         install: true)

# TODO: Generate PKG_CONFIG file
