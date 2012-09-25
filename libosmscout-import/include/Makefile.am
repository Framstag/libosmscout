nobase_include_HEADERS= osmscout/private/Config.h \
                        osmscout/private/ImportImportExport.h \
                        osmscout/private/Math.h \
                        osmscout/ImportFeatures.h \
                        osmscout/import/RawCoastline.h \
                        osmscout/import/RawNode.h \
                        osmscout/import/RawRelation.h \
                        osmscout/import/RawWay.h \
                        osmscout/import/CoordDataFile.h \
                        osmscout/import/GenAreaAreaIndex.h \
                        osmscout/import/GenAreaNodeIndex.h \
                        osmscout/import/GenAreaWayIndex.h \
                        osmscout/import/GenCityStreetIndex.h \
                        osmscout/import/GenNumericIndex.h \
                        osmscout/import/GenNodeDat.h \
                        osmscout/import/GenOptimizeLowZoom.h \
                        osmscout/import/GenRelationDat.h \
                        osmscout/import/GenRouteDat.h \
                        osmscout/import/GenTurnRestrictionDat.h \
                        osmscout/import/GenTypeDat.h \
                        osmscout/import/GenWaterIndex.h \
                        osmscout/import/GenWayDat.h \
                        osmscout/import/RenumberRawDats.h \
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
