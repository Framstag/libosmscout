osmscouttestIncDir = include_directories('.')

osmscouttestHeader = [
            'osmscout-test/TestImportExport.h',
            'osmscout-test/olt/Parser.h',
            'osmscout-test/olt/Scanner.h',
            'osmscout-test/RegionList.h',
            'osmscout-test/PreprocessOLT.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscouttestHeader,
                    preserve_path: true)
else
    install_headers(osmscouttestHeader)
endif

