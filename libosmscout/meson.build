cppArgs = []

if get_option('default_library')=='shared'
  cppArgs += ['-DOSMSCOUT_EXPORT_SYMBOLS']
  
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


osmscout = library('osmscout',
                   osmscoutSrc,
                   include_directories: osmscoutIncDir,
                   cpp_args: cppArgs,
                   dependencies: [mathDep, threadDep, iconvDep, marisaDep],        
                   install: true)
        
# TODO: Generate PKG_CONFIG file        
