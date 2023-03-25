cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMScout_EXPORTS']

  if haveVisibility
    cppArgs += ['-fvisibility=hidden']
  endif
endif

# TODO: Check for OpenMP support
# TODO: Check for SEE support
# TODO: Generate Config.h

subdir('include')
subdir('include/osmscout')
subdir('include/osmscout/private')
subdir('src')

link_args = []
if compiler.get_id()=='gcc' and compiler.version().version_compare('<=8.3.0')
  # GCC <= 8.3 needs explicit filesystem library linking
  link_args += '-lstdc++fs'
endif

osmscout = library('osmscout',
                   osmscoutSrc,
                   include_directories: osmscoutIncDir,
                   cpp_args: cppArgs,
                   dependencies: [mathDep, threadDep, openmpDep, iconvDep, marisaDep],
                   link_args: link_args,
                   version: libraryVersion,
                   install: true)

# TODO: Generate PKG_CONFIG file
