osmscoutmapcairoIncDir = include_directories('.')

osmscoutmapcairoHeader = [
            'osmscoutmapcairo/MapCairoImportExport.h',
            'osmscoutmapcairo/MapPainterCairo.h',
            'osmscoutmapcairo/SymbolRendererCairo.h'
          ]

if pngDep.found()
  osmscoutmapcairoHeader += ['osmscoutmapcairo/LoaderPNG.h']
endif

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapcairoHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapcairoHeader)
endif

