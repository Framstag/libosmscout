osmscoutmapgdiIncDir = include_directories('.')

osmscoutmapgdiHeader = [
            'osmscoutmapgdi/MapGDIImportExport.h',
            'osmscoutmapgdi/MapPainterGDI.h',
            'osmscoutmapgdi/MapPainterGDIWindow.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapgdiHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapgdiHeader)
endif

