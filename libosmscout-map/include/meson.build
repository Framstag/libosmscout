osmscoutmapIncDir = include_directories('.')

osmscoutmapHeader = [
            'osmscoutmap/MapImportExport.h',
            'osmscoutmap/oss/Scanner.h',
            'osmscoutmap/oss/Parser.h',
            'osmscoutmap/BatchMapPainter.h',
            'osmscoutmap/LabelLayouter.h',
            'osmscoutmap/LabelLayouterHelper.h',
            'osmscoutmap/MapPainter.h',
            'osmscoutmap/MapPainterStatistics.h',
            'osmscoutmap/MapParameter.h',
            'osmscoutmap/LabelProvider.h',
            'osmscoutmap/LabelPath.h',
            'osmscoutmap/StyleError.h',
            'osmscoutmap/Styles.h',
            'osmscoutmap/StyleDescription.h',
            'osmscoutmap/StyleConfig.h',
            'osmscoutmap/StyleProcessor.h',
            'osmscoutmap/DataTileCache.h',
            'osmscoutmap/MapTileCache.h',
            'osmscoutmap/MapData.h',
            'osmscoutmap/MapService.h',
            'osmscoutmap/MapPainterNoOp.h',
            'osmscoutmap/SymbolRenderer.h'
          ]

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutmapHeader,
                    preserve_path: true)
else
    install_headers(osmscoutmapHeader)
endif

