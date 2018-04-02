osmscoutmapcairoIncDir = include_directories('.')

osmscoutmapcairoHeader = [
            'osmscout/MapCairoImportExport.h',
            'osmscout/MapPainterCairo.h'
          ]

if pngDep.found()
  osmscoutmapcairoHeader += ['osmscout/LoaderPNG.h']
endif

install_headers(osmscoutmapcairoHeader)
