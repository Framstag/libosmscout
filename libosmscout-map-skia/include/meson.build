osmscoutmapskiaIncDir = include_directories('.', '../../include-patch')

osmscoutmapskiaHeader = [
            'osmscoutmapskia/MapSkiaImportExport.h',
            'osmscoutmapskia/MapPainterSkia.h',
            'osmscoutmapskia/SymbolRendererSkia.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapskiaHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapskiaHeader)
endif
