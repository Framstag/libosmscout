cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutTest_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout-test')
subdir('include/osmscout-test/private')
subdir('src')

osmscouttest = library('osmscout_test',
                       osmscouttestSrc,
                       include_directories: [osmscoutimportIncDir, osmscouttestIncDir, osmscoutIncDir],
                       cpp_args: cppArgs,
                       dependencies: [mathDep, threadDep],
                       link_with: [osmscout, osmscoutimport],
                       version: libraryVersion,
                       install: true)

# TODO: Generate PKG_CONFIG file
