cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutGPX_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

subdir('include')
subdir('include/osmscoutgpx')
subdir('include/osmscoutgpx/private')
subdir('src')

osmscoutgpx = library('osmscout_gpx',
                      osmscoutgpxSrc,
                      include_directories: [osmscoutgpxIncDir, osmscoutIncDir],
                      cpp_args: cppArgs,
                      dependencies: [mathDep, threadDep, xml2Dep, iconvDep, zlibDep],
                      link_with: [osmscout],
                      install: true)

# TODO: Generate PKG_CONFIG file
