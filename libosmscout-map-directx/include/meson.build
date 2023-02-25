osmscoutmapdirectxIncDir = include_directories('.')

osmscoutmapdirectxHeader = [
            'osmscoutmapdirectx/MapDirectXImportExport.h',
            'osmscoutmapdirectx/MapPainterDirectX.h',
            'osmscoutmapdirectx/PathTextRenderer.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapdirectxHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapdirectxHeader)
endif

