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

# db.json database metadata (read and written by the Import tools). Required
# here, not at the top level, so configurations that do not build the import
# library do not need it (the root's jsonDep stays optional for MCPServer).
importJsonDep = dependency('nlohmann_json', required: true)

if protobufDep.found() and protocCmd.found()
   osmscoutimportSrc += protoSrc
endif

osmscoutimport = library('osmscout_import',
                         osmscoutimportSrc,
                         include_directories: [osmscoutimportIncDir, osmscoutIncDir],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep, tbbDep, openmpDep, wsock32Dep, xml2Dep, marisaDep, protobufDep, zlibDep, importJsonDep],
                         link_with: [osmscout],
                         version: libraryVersion,
                         install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutimport,
             name: 'libosmscout_import',
             filebase: 'libosmscout_import',
             description: 'Library for importing OSM data into libosmscout databases',
             url: 'https://github.com/Framstag/libosmscout',
             license: 'GPL')
