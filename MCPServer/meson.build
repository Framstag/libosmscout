MCPServer = executable('MCPServer',
                      'src/MCPServer.cpp',
                      'src/LocationDescriptionMapper.cpp',
                      'src/VersionTool.cpp',
                      'src/LocationDescriptionTool.cpp',
                      include_directories: [osmscoutIncDir],
                      dependencies: [mathDep, openmpDep, jsonDep],
                      link_with: [osmscout],
                      install: true)