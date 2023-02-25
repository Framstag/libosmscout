osmscoutmapqtIncDir = include_directories('.')

osmscoutmapqtHeader = [
            'osmscoutmapqt/MapQtImportExport.h',
            'osmscoutmapqt/MapPainterQt.h',
            'osmscoutmapqt/SymbolRendererQt.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapqtHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapqtHeader)
endif

