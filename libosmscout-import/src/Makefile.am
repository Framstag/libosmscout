SUBDIRS = protobuf

AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTIMPORTDLL \
              -I$(top_srcdir)/include \
              -I$(top_srcdir)/include/osmscout/import/pbf \
              $(LIBOSMSCOUT_CFLAGS) \
              $(XML2_CFLAGS) \
              $(PROTOBUF_CFLAGS) \
              $(ZLIB_CFLAGS) \
              $(MARISA_CFLAGS)

lib_LTLIBRARIES = libosmscoutimport.la
libosmscoutimport_la_LDFLAGS  = -no-undefined \
                                $(LIBOSMSCOUT_LIBS) \
                                $(XML2_LIBS) \
                                $(PROTOBUF_LIBS) \
                                $(ZLIB_LIBS) \
                                $(MARISA_LIBS)

libosmscoutimport_la_SOURCES = osmscout/import/RawCoastline.cpp \
                               osmscout/import/RawCoord.cpp \
                               osmscout/import/RawNode.cpp \
                               osmscout/import/RawRelation.cpp \
                               osmscout/import/RawRelIndexedDataFile.cpp \
                               osmscout/import/RawWay.cpp \
                               osmscout/import/RawWayIndexedDataFile.cpp \
                               osmscout/import/WaterIndexProcessor.cpp \
                               osmscout/import/GenAreaAreaIndex.cpp \
                               osmscout/import/GenAreaNodeIndex.cpp \
                               osmscout/import/GenAreaWayIndex.cpp \
                               osmscout/import/GenCoordDat.cpp \
                               osmscout/import/GenIntersectionIndex.cpp \
                               osmscout/import/GenLocationIndex.cpp \
                               osmscout/import/GenMergeAreas.cpp \
                               osmscout/import/GenNumericIndex.cpp \
                               osmscout/import/GenRawNodeIndex.cpp \
                               osmscout/import/GenRawWayIndex.cpp \
                               osmscout/import/GenRawRelIndex.cpp \
                               osmscout/import/GenNodeDat.cpp \
                               osmscout/import/GenOptimizeAreaWayIds.cpp \
                               osmscout/import/GenOptimizeAreasLowZoom.cpp \
                               osmscout/import/GenOptimizeWaysLowZoom.cpp \
                               osmscout/import/GenRelAreaDat.cpp \
                               osmscout/import/GenRouteDat.cpp \
                               osmscout/import/GenTypeDat.cpp \
                               osmscout/import/GenWaterIndex.cpp \
                               osmscout/import/GenWayAreaDat.cpp \
                               osmscout/import/GenWayWayDat.cpp \
                               osmscout/import/MergeAreaData.cpp \
                               osmscout/import/ShapeFileScanner.cpp \
                               osmscout/import/SortDat.cpp \
                               osmscout/import/SortNodeDat.cpp \
                               osmscout/import/SortWayDat.cpp \
                               osmscout/import/Import.cpp \
                               osmscout/import/ImportErrorReporter.cpp \
                               osmscout/import/Preprocessor.cpp \
                               osmscout/import/Preprocess.cpp \
                               osmscout/import/PreprocessPoly.cpp

if HAVE_LIB_XML
libosmscoutimport_la_SOURCES += osmscout/import/PreprocessOSM.cpp
endif

if HAVE_LIB_PROTOBUF
if HAVE_PROG_PROTOC
libosmscoutimport_la_SOURCES += osmscout/import/pbf/fileformat.pb.cc \
                                osmscout/import/pbf/osmformat.pb.cc \
                                osmscout/import/PreprocessPBF.cpp
endif
endif

if OSMSCOUT_HAVE_LIB_MARISA
libosmscoutimport_la_SOURCES += osmscout/import/GenTextIndex.cpp
endif
