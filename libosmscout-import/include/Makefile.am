nobase_include_HEADERS= osmscout/private/Config.h \
                        osmscout/private/ImportImportExport.h \
                        osmscout/ImportFeatures.h \
                        osmscout/import/RawCoastline.h \
                        osmscout/import/RawNode.h \
                        osmscout/import/RawRelation.h \
                        osmscout/import/RawWay.h \
                        osmscout/import/GenAreaAreaIndex.h \
                        osmscout/import/GenAreaNodeIndex.h \
                        osmscout/import/GenAreaWayIndex.h \
                        osmscout/import/GenLocationIndex.h \
                        osmscout/import/GenNumericIndex.h \
                        osmscout/import/GenNodeDat.h \
                        osmscout/import/GenOptimizeAreaWayIds.h \
                        osmscout/import/GenOptimizeAreasLowZoom.h \
                        osmscout/import/GenOptimizeWaysLowZoom.h \
                        osmscout/import/GenRelAreaDat.h \
                        osmscout/import/GenRouteDat.h \
                        osmscout/import/GenTurnRestrictionDat.h \
                        osmscout/import/GenTypeDat.h \
                        osmscout/import/GenWaterIndex.h \
                        osmscout/import/GenWayAreaDat.h \
                        osmscout/import/GenWayWayDat.h \
                        osmscout/import/SortDat.h \
                        osmscout/import/SortAreaDat.h \
                        osmscout/import/SortNodeDat.h \
                        osmscout/import/SortWayDat.h \
                        osmscout/import/Import.h \
                        osmscout/import/Preprocess.h

if HAVE_LIB_XML
nobase_include_HEADERS += osmscout/import/PreprocessOSM.h
endif

if HAVE_LIB_PROTOBUF
nobase_include_HEADERS += osmscout/import/pbf/fileformat.pb.h \
                          osmscout/import/pbf/osmformat.pb.h \
                          osmscout/import/PreprocessPBF.h
endif

if OSMSCOUT_HAVE_LIB_MARISA
nobase_include_HEADERS += osmscout/import/GenTextIndex.h
endif