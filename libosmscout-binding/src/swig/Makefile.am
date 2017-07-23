BUILT_SOURCES=$(top_srcdir)/src/java/libosmscout.cpp

clean-local:
	rm -f $(top_srcdir)/src/java/libosmscout.cpp
	rm -f $(top_srcdir)/src/java/net/sf/libosmscout/osmscout/*.java

SWIG_SOURCES=osmscout/util/FileScanner.i\
             osmscout/util/FileWriter.i\
             osmscout/util/GeoBox.i\
             osmscout/util/Magnification.i\
             osmscout/util/Parsing.i\
             osmscout/util/Progress.i\
             osmscout/AreaAreaIndex.i\
             osmscout/AreaDataFile.i\
             osmscout/Area.i\
             osmscout/AreaNodeIndex.i\
             osmscout/AreaWayIndex.i\
             osmscout/Database.i\
             osmscout/GeoCoord.i\
             osmscout/GroundTile.i\
             osmscout/Location.i\
             osmscout/LocationIndex.i\
             osmscout/LocationService.i\
             osmscout/NodeDataFile.i\
             osmscout/Node.i\
             osmscout/ObjectRef.i\
             osmscout/OptimizeAreasLowZoom.i\
             osmscout/OptimizeWaysLowZoom.i\
             osmscout/Point.i\
             osmscout/routing/DBFileOffset.i\
             osmscout/routing/Route.i\
             osmscout/routing/RouteData.i\
             osmscout/routing/RouteNode.i\
             osmscout/routing/RoutePostprocessor.i\
             osmscout/routing/RoutingProfile.i\
             osmscout/routing/RoutingService.i\
             osmscout/Tag.i\
             osmscout/TypeConfig.i\
             osmscout/Types.i\
             osmscout/WaterIndex.i\
             osmscout/WayDataFile.i\
             osmscout/Way.i\
             osmscout/libosmscout.i

$(top_srcdir)/src/java/libosmscout.cpp: $(SWIG_SOURCES)
	$(SWIG) -java -cppext cpp -I$(top_srcdir)/../libosmscout/include -package net.sf.libosmscout.osmscout -outdir $(top_srcdir)/src/java/net/sf/libosmscout/osmscout -o $(top_srcdir)/src/java/libosmscout.cpp osmscout/libosmscout.i
