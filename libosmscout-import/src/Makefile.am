SUBDIRS = protobuf

AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCOUTIMPORTDLL \
              -I$(top_srcdir)/include \
              -I$(top_srcdir)/include/osmscout/import/pbf \
              $(LIBOSMSCOUT_CFLAGS) \
              $(XML2_CFLAGS) \
              $(PROTOBUF_CFLAGS) \
              $(ZLIB_CFLAGS)

lib_LTLIBRARIES = libosmscoutimport.la
libosmscoutimport_la_LDFLAGS  = -no-undefined \
                                $(LIBOSMSCOUT_LIBS) \
                                $(XML2_LIBS) \
                                $(PROTOBUF_LIBS) \
                                $(ZLIB_LIBS)

libosmscoutimport_la_SOURCES = osmscout/import/RawCoastline.cpp \
                               osmscout/import/RawNode.cpp \
                               osmscout/import/RawRelation.cpp \
                               osmscout/import/RawWay.cpp \
                               osmscout/import/GenAreaAreaIndex.cpp \
                               osmscout/import/GenAreaNodeIndex.cpp \
                               osmscout/import/GenAreaWayIndex.cpp \
                               osmscout/import/GenCityStreetIndex.cpp \
                               osmscout/import/GenNumericIndex.cpp \
                               osmscout/import/GenNodeDat.cpp \
                               osmscout/import/GenOptimizeLowZoom.cpp \
                               osmscout/import/GenRelationDat.cpp \
                               osmscout/import/GenRouteDat.cpp \
                               osmscout/import/GenTurnRestrictionDat.cpp \
                               osmscout/import/GenTypeDat.cpp \
                               osmscout/import/GenWaterIndex.cpp \
                               osmscout/import/GenWayDat.cpp \
                               osmscout/import/RenumberRawDats.cpp \
                               osmscout/import/Import.cpp \
                               osmscout/import/Preprocess.cpp

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
