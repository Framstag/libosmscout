cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScoutClient_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

# TODO: Check for OpenMP support
# TODO: Check for SEE support
# TODO: Generate Config.h

subdir('include')
subdir('include/osmscoutclient')
subdir('include/osmscoutclient/private')
subdir('src')

link_args = []
if compiler.get_id()=='gcc' and compiler.version().version_compare('<=8.3.0')
  # GCC <= 8.3 needs explicit filesystem library linking
  link_args += '-lstdc++fs'
endif

osmscoutclient = library('osmscout_client',
                         osmscoutclientSrc,
                         include_directories: [osmscoutclientIncDir, osmscoutIncDir],
                         cpp_args: cppArgs,
                         dependencies: [mathDep, threadDep],
                         link_args: link_args,
                         link_with: [osmscout],
                         version: libraryVersion,
                         install: true)

pkg = import('pkgconfig')
pkg.generate(osmscoutclient)
