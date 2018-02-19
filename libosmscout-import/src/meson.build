osmscoutimportSrc = [
            'src/osmscout/import/RawCoastline.cpp',
            'src/osmscout/import/RawCoord.cpp',
            'src/osmscout/import/RawNode.cpp',
            'src/osmscout/import/RawRelation.cpp',
            'src/osmscout/import/RawRelIndexedDataFile.cpp',
            'src/osmscout/import/RawWay.cpp',
            'src/osmscout/import/RawWayIndexedDataFile.cpp',
            'src/osmscout/import/WaterIndexProcessor.cpp',
            'src/osmscout/import/GenAreaAreaIndex.cpp',
            'src/osmscout/import/GenAreaNodeIndex.cpp',
            'src/osmscout/import/GenAreaWayIndex.cpp',
            'src/osmscout/import/GenCoordDat.cpp',
            'src/osmscout/import/GenCoverageIndex.cpp',
            'src/osmscout/import/GenIntersectionIndex.cpp',
            'src/osmscout/import/GenLocationIndex.cpp',
            'src/osmscout/import/GenMergeAreas.cpp',
            'src/osmscout/import/GenNumericIndex.cpp',
            'src/osmscout/import/GenRawNodeIndex.cpp',
            'src/osmscout/import/GenRawWayIndex.cpp',
            'src/osmscout/import/GenRawRelIndex.cpp',
            'src/osmscout/import/GenNodeDat.cpp',
            'src/osmscout/import/GenOptimizeAreaWayIds.cpp',
            'src/osmscout/import/GenOptimizeAreasLowZoom.cpp',
            'src/osmscout/import/GenOptimizeWaysLowZoom.cpp',
            'src/osmscout/import/GenRelAreaDat.cpp',
            'src/osmscout/import/GenRouteDat.cpp',
            'src/osmscout/import/GenTypeDat.cpp',
            'src/osmscout/import/GenWaterIndex.cpp',
            'src/osmscout/import/GenWayAreaDat.cpp',
            'src/osmscout/import/GenWayWayDat.cpp',
            'src/osmscout/import/MergeAreaData.cpp',
            'src/osmscout/import/ShapeFileScanner.cpp',
            'src/osmscout/import/SortDat.cpp',
            'src/osmscout/import/SortNodeDat.cpp',
            'src/osmscout/import/SortWayDat.cpp',
            'src/osmscout/import/Import.cpp',
            'src/osmscout/import/ImportErrorReporter.cpp',
            'src/osmscout/import/Preprocessor.cpp',
            'src/osmscout/import/Preprocess.cpp',
            'src/osmscout/import/PreprocessPoly.cpp'
          ]

if xml2Dep.found()
  osmscoutimportSrc += ['src/osmscout/import/PreprocessOSM.cpp']
endif

if protocCmd.found() and protobufDep.found()
  osmscoutimportSrc += ['src/osmscout/import/PreprocessPBF.cpp']
endif

if marisaDep.found()
  osmscoutimportSrc += ['src/osmscout/import/GenTextIndex.cpp']
endif

