osmscoutmapaggIncDir = include_directories('.')

osmscoutmapaggHeader = [
            'osmscoutmapagg/MapAggImportExport.h',
            'osmscoutmapagg/MapPainterAgg.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapaggHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapaggHeader)
endif

