SUBDIRS = protobuf

AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTDLL \
              -I$(top_srcdir)/include \
              -I$(top_srcdir)/include/osmscout/pbf \
              $(XML2_CFLAGS) \
              $(PROTOBUF_CFLAGS) \
              $(ZLIB_CFLAGS)

lib_LTLIBRARIES = libosmscout.la
libosmscout_la_LDFLAGS  = $(XML2_LIBS) \
                          $(PROTOBUF_LIBS) \
                          $(ZLIB_LIBS)

libosmscout_la_SOURCES= osmscout/util/Cache.cpp \
                        osmscout/util/FileScanner.cpp \
                        osmscout/util/FileWriter.cpp \
                        osmscout/util/Geometry.cpp \
                        osmscout/util/NumberSet.cpp \
                        osmscout/util/Reference.cpp \
                        osmscout/util/StopClock.cpp \
                        osmscout/util/String.cpp \
                        osmscout/Types.cpp \
                        osmscout/Util.cpp \
                        osmscout/Progress.cpp \
                        osmscout/StyleConfig.cpp \
                        osmscout/StyleConfigLoader.cpp \
                        osmscout/TypeConfig.cpp \
                        osmscout/oss/Scanner.cpp \
                        osmscout/oss/Parser.cpp \
                        osmscout/ost/Scanner.cpp \
                        osmscout/ost/Parser.cpp \
                        osmscout/TypeConfigLoader.cpp \
                        osmscout/Tiles.cpp \
                        osmscout/RawNode.cpp \
                        osmscout/RawRelation.cpp \
                        osmscout/RawWay.cpp \
                        osmscout/AdminRegion.cpp \
                        osmscout/GroundTile.cpp \
                        osmscout/Location.cpp \
                        osmscout/Node.cpp \
                        osmscout/Point.cpp \
                        osmscout/Relation.cpp \
                        osmscout/SegmentAttributes.cpp \
                        osmscout/Tag.cpp \
                        osmscout/Way.cpp \
                        osmscout/ObjectRef.cpp \
                        osmscout/NumericIndex.cpp \
                        osmscout/Import.cpp \
                        osmscout/Preprocess.cpp \
                        osmscout/GenAreaAreaIndex.cpp \
                        osmscout/GenAreaNodeIndex.cpp \
                        osmscout/GenAreaWayIndex.cpp \
                        osmscout/GenCityStreetIndex.cpp \
                        osmscout/GenNumericIndex.cpp \
                        osmscout/GenNodeDat.cpp \
                        osmscout/GenNodeUseIndex.cpp \
                        osmscout/GenRelationDat.cpp \
                        osmscout/GenTypeDat.cpp \
                        osmscout/GenWaterIndex.cpp \
                        osmscout/GenWayDat.cpp \
                        osmscout/AreaAreaIndex.cpp \
                        osmscout/AreaNodeIndex.cpp \
                        osmscout/AreaWayIndex.cpp \
                        osmscout/CityStreetIndex.cpp \
                        osmscout/NodeIndex.cpp \
                        osmscout/NodeUseIndex.cpp \
                        osmscout/WaterIndex.cpp \
                        osmscout/WayIndex.cpp \
                        osmscout/Route.cpp \
                        osmscout/RoutingProfile.cpp \
                        osmscout/Database.cpp

if HAVE_LIB_XML
libosmscout_la_SOURCES += osmscout/PreprocessOSM.cpp
endif

if HAVE_LIB_PROTOBUF
if HAVE_PROG_PROTOC
libosmscout_la_SOURCES += osmscout/pbf/fileformat.pb.cc \
                          osmscout/pbf/osmformat.pb.cc \
                          osmscout/PreprocessPBF.cpp
endif
endif

