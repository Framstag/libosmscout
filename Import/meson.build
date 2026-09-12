executable('Import',
           'src/Import.cpp',
           'src/JsonWriter.cpp',
           'src/DbJsonWriter.cpp',
           include_directories: [osmscoutIncDir, osmscoutimportIncDir],
           cpp_args: '-DOSMSCOUT_IMPORT_VERSION="' + libraryVersion + '"',
           dependencies: [mathDep, openmpDep],
           link_with: [osmscout, osmscoutimport],
           install: true)

