DumpData = executable('WellScoutedRoute',
                      'src/WellScoutedRoute.cpp',
                      include_directories: [osmscoutIncDir, osmscoutgpxIncDir],
                      dependencies: [mathDep, openmpDep],
                      link_with: [osmscout, osmscoutgpx],
                      install: true)

