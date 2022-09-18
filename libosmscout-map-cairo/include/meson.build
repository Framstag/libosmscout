osmscoutmapcairoIncDir = include_directories('.')

osmscoutmapcairoHeader = [
            'osmscoutmapcairo/MapCairoImportExport.h',
            'osmscoutmapcairo/MapPainterCairo.h',
            'osmscoutmapcairo/SymbolRendererCairo.h'
          ]

if pngDep.found()
  osmscoutmapcairoHeader += ['osmscoutmapcairo/LoaderPNG.h']
endif

install_headers(osmscoutmapcairoHeader)
