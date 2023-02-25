osmscoutimportIncDir = include_directories('.')

osmscoutimportHeader = [
            'osmscoutimport/AreaIndexGenerator.h',
            'osmscoutimport/ImportImportExport.h',
            'osmscoutimport/RawCoastline.h',
            'osmscoutimport/RawCoord.h',
            'osmscoutimport/RawNode.h',
            'osmscoutimport/RawRelation.h',
            'osmscoutimport/RawRelIndexedDataFile.h',
            'osmscoutimport/RawWay.h',
            'osmscoutimport/RawWayIndexedDataFile.h',
            'osmscoutimport/WaterIndexProcessor.h',
            'osmscoutimport/GenAreaAreaIndex.h',
            'osmscoutimport/GenAreaNodeIndex.h',
            'osmscoutimport/GenAreaRouteIndex.h',
            'osmscoutimport/GenAreaWayIndex.h',
            'osmscoutimport/GenCoordDat.h',
            'osmscoutimport/GenCoverageIndex.h',
            'osmscoutimport/GenIntersectionIndex.h',
            'osmscoutimport/GenLocationIndex.h',
            'osmscoutimport/GenMergeAreas.h',
            'osmscoutimport/GenNumericIndex.h',
            'osmscoutimport/GenRawNodeIndex.h',
            'osmscoutimport/GenRawWayIndex.h',
            'osmscoutimport/GenRawRelIndex.h',
            'osmscoutimport/GenNodeDat.h',
            'osmscoutimport/GenOptimizeAreaWayIds.h',
            'osmscoutimport/GenOptimizeAreasLowZoom.h',
            'osmscoutimport/GenOptimizeWaysLowZoom.h',
            'osmscoutimport/GenPTRouteDat.h',
            'osmscoutimport/GenRelAreaDat.h',
            'osmscoutimport/GenRouteDat.h',
            'osmscoutimport/GenRoute2Dat.h',
            'osmscoutimport/GenTypeDat.h',
            'osmscoutimport/GenWaterIndex.h',
            'osmscoutimport/GenWayAreaDat.h',
            'osmscoutimport/GenWayWayDat.h',
            'osmscoutimport/MergeAreaData.h',
            'osmscoutimport/ShapeFileScanner.h',
            'osmscoutimport/SortDat.h',
            'osmscoutimport/SortNodeDat.h',
            'osmscoutimport/SortWayDat.h',
            'osmscoutimport/Import.h',
            'osmscoutimport/ImportErrorReporter.h',
            'osmscoutimport/ImportModule.h',
            'osmscoutimport/ImportParameter.h',
            'osmscoutimport/ImportProgress.h',
            'osmscoutimport/Preprocessor.h',
            'osmscoutimport/Preprocess.h',
            'osmscoutimport/PreprocessPoly.h'
          ]

if xml2Dep.found()
  osmscoutimportHeader += ['osmscoutimport/PreprocessOSM.h']
endif

if protocCmd.found() and protobufDep.found()
  osmscoutimportHeader += ['osmscoutimport/PreprocessPBF.h']
endif

if marisaDep.found()
  osmscoutimportHeader += ['osmscoutimport/GenTextIndex.h']
endif

if meson.version().version_compare('>=0.63.0')
    install_headers(osmscoutimportHeader,
                    preserve_path: true)
else
    install_headers(osmscoutimportHeader)
endif

