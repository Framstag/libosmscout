osmscoutmapsvgIncDir = include_directories('.')

osmscoutmapsvgHeader = [
            'osmscoutmapsvg/MapSVGImportExport.h',
            'osmscoutmapsvg/MapPainterSVG.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapsvgHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapsvgHeader)
endif

