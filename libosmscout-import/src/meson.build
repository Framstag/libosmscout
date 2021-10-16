osmscoutimportSrc = [
            'src/osmscoutimport/AreaIndexGenerator.cpp',
            'src/osmscoutimport/RawCoastline.cpp',
            'src/osmscoutimport/RawCoord.cpp',
            'src/osmscoutimport/RawNode.cpp',
            'src/osmscoutimport/RawRelation.cpp',
            'src/osmscoutimport/RawRelIndexedDataFile.cpp',
            'src/osmscoutimport/RawWay.cpp',
            'src/osmscoutimport/RawWayIndexedDataFile.cpp',
            'src/osmscoutimport/WaterIndexProcessor.cpp',
            'src/osmscoutimport/GenAreaAreaIndex.cpp',
            'src/osmscoutimport/GenAreaNodeIndex.cpp',
            'src/osmscoutimport/GenAreaWayIndex.cpp',
            'src/osmscoutimport/GenCoordDat.cpp',
            'src/osmscoutimport/GenCoverageIndex.cpp',
            'src/osmscoutimport/GenIntersectionIndex.cpp',
            'src/osmscoutimport/GenLocationIndex.cpp',
            'src/osmscoutimport/GenMergeAreas.cpp',
            'src/osmscoutimport/GenNumericIndex.cpp',
            'src/osmscoutimport/GenRawNodeIndex.cpp',
            'src/osmscoutimport/GenAreaRouteIndex.cpp',
            'src/osmscoutimport/GenRawWayIndex.cpp',
            'src/osmscoutimport/GenRawRelIndex.cpp',
            'src/osmscoutimport/GenNodeDat.cpp',
            'src/osmscoutimport/GenOptimizeAreaWayIds.cpp',
            'src/osmscoutimport/GenOptimizeAreasLowZoom.cpp',
            'src/osmscoutimport/GenOptimizeWaysLowZoom.cpp',
            'src/osmscoutimport/GenPTRouteDat.cpp',
            'src/osmscoutimport/GenRelAreaDat.cpp',
            'src/osmscoutimport/GenRouteDat.cpp',
            'src/osmscoutimport/GenRoute2Dat.cpp',
            'src/osmscoutimport/GenTypeDat.cpp',
            'src/osmscoutimport/GenWaterIndex.cpp',
            'src/osmscoutimport/GenWayAreaDat.cpp',
            'src/osmscoutimport/GenWayWayDat.cpp',
            'src/osmscoutimport/MergeAreaData.cpp',
            'src/osmscoutimport/ShapeFileScanner.cpp',
            'src/osmscoutimport/SortDat.cpp',
            'src/osmscoutimport/SortNodeDat.cpp',
            'src/osmscoutimport/SortWayDat.cpp',
            'src/osmscoutimport/Import.cpp',
            'src/osmscoutimport/ImportErrorReporter.cpp',
            'src/osmscoutimport/ImportModule.cpp',
            'src/osmscoutimport/ImportParameter.cpp',
            'src/osmscoutimport/ImportProgress.cpp',
            'src/osmscoutimport/Preprocessor.cpp',
            'src/osmscoutimport/Preprocess.cpp',
            'src/osmscoutimport/PreprocessPoly.cpp'
          ]

if xml2Dep.found()
  osmscoutimportSrc += ['src/osmscoutimport/PreprocessOSM.cpp']
endif

if protocCmd.found() and protobufDep.found()
  osmscoutimportSrc += ['src/osmscoutimport/PreprocessPBF.cpp']
endif

if marisaDep.found()
  osmscoutimportSrc += ['src/osmscoutimport/GenTextIndex.cpp']
endif

