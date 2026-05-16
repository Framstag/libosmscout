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

pkg = import('pkgconfig')
pkg.generate(osmscouttest,
             name: 'libosmscout_map_test',
             filebase: 'libosmscout_map_test',
             description: 'Library with test functionality for testing libosmscout',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')
