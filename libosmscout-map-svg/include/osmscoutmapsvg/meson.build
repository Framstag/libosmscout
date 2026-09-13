mapsvgFeaturesCfg = configuration_data()

mapsvgFeaturesCfg.set('OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO',pangoft2Dep.found(), description: 'text drawing using pango is supported')
mapsvgFeaturesCfg.set('OSMSCOUT_MAP_SVG_HAVE_LIB_FONTCONFIG',fontconfigDep.found(), description: 'font family names are resolved with fontconfig')

configure_file(output: 'MapSVGFeatures.h',
               install_dir: 'include/osmscoutmapsvg',
               configuration: mapsvgFeaturesCfg)

osmscoutmapsvgHeader = [
            'osmscoutmapsvg/MapSVGImportExport.h',
            'osmscoutmapsvg/MapPainterSVG.h',
            'osmscoutmapsvg/SymbolRendererSVG.h'
          ]
