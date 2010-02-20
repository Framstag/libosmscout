AM_CPPFLAGS = $(LIB_CXXFLAGS) -DOSMSCDLL -I$(top_srcdir)/include \
              $(XML2_CFLAGS) $(CAIRO_CFLAGS)

# We disable $(X_PRE_LIBS) until we need them somewhere
AM_LDFLAGS  = $(XML2_LIBS) $(CAIRO_LIBS)

lib_LTLIBRARIES = libosmscout.la
libosmscout_la_SOURCES= osmscout/Types.cpp \
                        osmscout/Util.cpp \
                        osmscout/Progress.cpp \
                        osmscout/FileReader.cpp \
                        osmscout/FileScanner.cpp \
                        osmscout/FileWriter.cpp \
                        osmscout/Cache.cpp \
                        osmscout/StyleConfig.cpp \
                        osmscout/StyleConfigLoader.cpp \
                        osmscout/TypeConfig.cpp \
                        osmscout/TypeConfigLoader.cpp \
                        osmscout/Tiles.cpp \
                        osmscout/RawNode.cpp \
                        osmscout/RawRelation.cpp \
                        osmscout/RawWay.cpp \
                        osmscout/AdminRegion.cpp \
                        osmscout/Node.cpp \
                        osmscout/Point.cpp \
                        osmscout/Location.cpp \
                        osmscout/Tag.cpp \
                        osmscout/Way.cpp \
                        osmscout/Reference.cpp \
                        osmscout/NumericIndex.cpp \
                        osmscout/Import.cpp \
                        osmscout/Preprocess.cpp \
                        osmscout/GenAreaAreaIndex.cpp \
                        osmscout/GenAreaNodeIndex.cpp \
                        osmscout/GenAreaWayIndex.cpp \
                        osmscout/GenCityStreetIndex.cpp \
                        osmscout/GenNodeDat.cpp \
                        osmscout/GenNodeUseIndex.cpp \
                        osmscout/GenWayDat.cpp \
                        osmscout/AreaAreaIndex.cpp \
                        osmscout/AreaNodeIndex.cpp \
                        osmscout/AreaWayIndex.cpp \
                        osmscout/CityStreetIndex.cpp \
                        osmscout/NodeUseIndex.cpp \
                        osmscout/Route.cpp \
                        osmscout/RoutingProfile.cpp \
                        osmscout/Database.cpp

