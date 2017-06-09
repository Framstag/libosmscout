AM_CPPFLAGS = $(LIB_CXXFLAGS) \
              $(PTHREAD_CFLAGS) \
              $(OPENMP_CXXFLAGS) \
              $(SIMD_FLAGS) \
              $(MARISA_CFLAGS) \
              -DOSMSCOUTDLL -I$(top_srcdir)/include

lib_LTLIBRARIES = libosmscout.la

libosmscout_la_LDFLAGS = -no-undefined \
                         $(PTHREAD_CFLAGS) \
                         $(PTHREAD_LIBS) \
                         $(OPENMP_CXXFLAGS) \
                         $(LTLIBICONV) \
                         $(MARISA_LIBS)

libosmscout_la_SOURCES= osmscout/util/Breaker.cpp \
                        osmscout/util/Cache.cpp \
                        osmscout/util/Color.cpp \
                        osmscout/util/Exception.cpp \
                        osmscout/util/File.cpp \
                        osmscout/util/FileScanner.cpp \
                        osmscout/util/FileWriter.cpp \
                        osmscout/util/HTMLWriter.cpp \
                        osmscout/util/GeoBox.cpp \
                        osmscout/util/Geometry.cpp \
                        osmscout/util/Logger.cpp \
                        osmscout/util/Magnification.cpp \
                        osmscout/util/MemoryMonitor.cpp \
                        osmscout/util/NodeUseMap.cpp \
                        osmscout/util/Number.cpp \
                        osmscout/util/NumberSet.cpp \
                        osmscout/util/Parsing.cpp \
                        osmscout/util/Progress.cpp \
                        osmscout/util/Projection.cpp \
                        osmscout/util/StopClock.cpp \
                        osmscout/util/String.cpp \
                        osmscout/util/Tiling.cpp \
                        osmscout/util/Transformation.cpp \
                        osmscout/util/WorkQueue.cpp \
                        osmscout/util/TagErrorReporter.cpp \
                        osmscout/Types.cpp \
                        osmscout/TypeConfig.cpp \
                        osmscout/TypeFeatures.cpp \
                        osmscout/ost/Scanner.cpp \
                        osmscout/ost/Parser.cpp \
                        osmscout/GroundTile.cpp \
                        osmscout/Intersection.cpp \
                        osmscout/Location.cpp \
                        osmscout/Coord.cpp \
                        osmscout/CoordDataFile.cpp \
                        osmscout/GeoCoord.cpp \
                        osmscout/Pixel.cpp \
                        osmscout/Area.cpp \
                        osmscout/AreaDataFile.cpp \
                        osmscout/Node.cpp \
                        osmscout/NodeDataFile.cpp \
                        osmscout/Path.cpp \
                        osmscout/Point.cpp \
                        osmscout/Tag.cpp \
                        osmscout/TurnRestriction.cpp \
                        osmscout/Way.cpp \
                        osmscout/WayDataFile.cpp \
                        osmscout/ObjectRef.cpp \
                        osmscout/NumericIndex.cpp \
                        osmscout/AreaAreaIndex.cpp \
                        osmscout/AreaNodeIndex.cpp \
                        osmscout/AreaWayIndex.cpp \
                        osmscout/LocationIndex.cpp \
                        osmscout/OptimizeAreasLowZoom.cpp \
                        osmscout/OptimizeWaysLowZoom.cpp \
                        osmscout/WaterIndex.cpp \
                        osmscout/ObjectVariantDataFile.cpp \
                        osmscout/Route.cpp \
                        osmscout/RouteData.cpp \
                        osmscout/RouteNode.cpp \
                        osmscout/RoutePostprocessor.cpp \
                        osmscout/RoutingProfile.cpp \
                        osmscout/BoundingBoxDataFile.cpp \
                        osmscout/TypeDistributionDataFile.cpp \
                        osmscout/Database.cpp \
                        osmscout/DebugDatabase.cpp \
                        osmscout/BasemapDatabase.cpp \
                        osmscout/SRTM.cpp \
                        osmscout/LocationService.cpp \
                        osmscout/POIService.cpp \
                        osmscout/RoutingService.cpp

if OSMSCOUT_HAVE_SSE2
libosmscout_la_SOURCES+=osmscout/system/SSEMath.cpp
endif

if OSMSCOUT_HAVE_LIB_MARISA
libosmscout_la_SOURCES+=osmscout/TextSearchIndex.cpp
endif
