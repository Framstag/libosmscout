osmscoutimportIncDir = include_directories('.')

osmscoutimportHeader = [
            'osmscout/private/ImportImportExport.h',
            'osmscout/import/RawCoastline.h',
            'osmscout/import/RawCoord.h',
            'osmscout/import/RawNode.h',
            'osmscout/import/RawRelation.h',
            'osmscout/import/RawRelIndexedDataFile.h',
            'osmscout/import/RawWay.h',
            'osmscout/import/RawWayIndexedDataFile.h',
            'osmscout/import/WaterIndexProcessor.h',
            'osmscout/import/GenAreaAreaIndex.h',
            'osmscout/import/GenAreaNodeIndex.h',
            'osmscout/import/GenAreaWayIndex.h',
            'osmscout/import/GenCoordDat.h',
            'osmscout/import/GenCoverageIndex.h',
            'osmscout/import/GenIntersectionIndex.h',
            'osmscout/import/GenLocationIndex.h',
            'osmscout/import/GenMergeAreas.h',
            'osmscout/import/GenNumericIndex.h',
            'osmscout/import/GenRawNodeIndex.h',
            'osmscout/import/GenRawWayIndex.h',
            'osmscout/import/GenRawRelIndex.h',
            'osmscout/import/GenNodeDat.h',
            'osmscout/import/GenOptimizeAreaWayIds.h',
            'osmscout/import/GenOptimizeAreasLowZoom.h',
            'osmscout/import/GenOptimizeWaysLowZoom.h',
            'osmscout/import/GenRelAreaDat.h',
            'osmscout/import/GenRouteDat.h',
            'osmscout/import/GenTypeDat.h',
            'osmscout/import/GenWaterIndex.h',
            'osmscout/import/GenWayAreaDat.h',
            'osmscout/import/GenWayWayDat.h',
            'osmscout/import/MergeAreaData.h',
            'osmscout/import/ShapeFileScanner.h',
            'osmscout/import/SortDat.h',
            'osmscout/import/SortNodeDat.h',
            'osmscout/import/SortWayDat.h',
            'osmscout/import/Import.h',
            'osmscout/import/ImportErrorReporter.h',
            'osmscout/import/Preprocessor.h',
            'osmscout/import/Preprocess.h',
            'osmscout/import/PreprocessPoly.h'
          ]

if xml2Dep.found()
  osmscoutimportHeader += ['osmscout/import/PreprocessOSM.h']
endif

if protocCmd.found() and protobufDep.found()
  osmscoutimportHeader += ['osmscout/import/PreprocessPBF.h']
endif

if marisaDep.found()
  osmscoutimportHeader += ['osmscout/import/GenTextIndex.h']
endif

install_headers(osmscoutimportHeader)
