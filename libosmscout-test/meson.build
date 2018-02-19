cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_TEST_EXPORT_SYMBOLS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

osmscouttest = library('osmscout_test',
                       osmscouttestSrc,
                       include_directories: [osmscoutimportIncDir, osmscouttestIncDir, osmscoutIncDir],
                       cpp_args: cppArgs,
                       dependencies: [mathDep, threadDep],
                       link_with: [osmscout, osmscoutimport],
                       install: true)

# TODO: Generate PKG_CONFIG file
