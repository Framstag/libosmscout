osmscoutmapiosxIncDir = include_directories('.')

osmscoutmapiosxHeader = [
            'osmscoutmapiosx/MapPainterIOS.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapiosxHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapiosxHeader)
endif

