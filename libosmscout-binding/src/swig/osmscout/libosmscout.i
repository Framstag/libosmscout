%module libosmscout

%include <typemaps.i>

%include <stdint.i>
%include <stl.i>

%include <std_string.i>
%include <std_wstring.i>
%include <std_shared_ptr.i>

%include <std_list.i>
%include <std_set.i>
%include <std_unordered_map.i>
//%include <std_unordered_set.i>
%include <std_vector.i>

%include <osmscout/CoreImportExport.i>

%include <osmscout/OSMScoutTypes.i>

%include <osmscout/util/Color.i>
%include <osmscout/util/Bearing.i>
%include <osmscout/util/Distance.i>
%include <osmscout/util/StringMatcher.i>
%include <osmscout/util/Time.i>
%include <osmscout/util/Breaker.i>

%include <osmscout/ObjectRef.i>

%include <osmscout/util/Magnification.i>

%include <osmscout/GeoCoord.i>

%include <osmscout/util/GeoBox.i>
%include <osmscout/Point.i>
%include <osmscout/Pixel.i>

%include <osmscout/util/Geometry.i>

%include <osmscout/util/Tiling.i>
%include <osmscout/util/TileId.i>

%include <osmscout/util/Projection.i>
%include <osmscout/util/Transformation.i>

%include <osmscout/util/FileScanner.i>
%include <osmscout/util/FileWriter.i>
%include <osmscout/util/Parsing.i>
%include <osmscout/util/Progress.i>

%include <osmscout/Tag.i>

%include <osmscout/util/TagErrorReporter.i>

%include <osmscout/TypeConfig.i>
%include <osmscout/TypeInfoSet.i>

%include <osmscout/BoundingBoxDataFile.i>

%include <osmscout/Node.i>
%include <osmscout/Way.i>
%include <osmscout/Area.i>
%include <osmscout/GroundTile.i>

%include <osmscout/DataFile.i>

%include <osmscout/NodeDataFile.i>
%include <osmscout/WayDataFile.i>
%include <osmscout/AreaDataFile.i>

%include <osmscout/AreaNodeIndex.i>
%include <osmscout/AreaWayIndex.i>
%include <osmscout/AreaAreaIndex.i>

%include <osmscout/OptimizeWaysLowZoom.i>
%include <osmscout/OptimizeAreasLowZoom.i>

%include <osmscout/Location.i>
%include <osmscout/LocationIndex.i>

%include <osmscout/WaterIndex.i>

%include <osmscout/Database.i>
%include <osmscout/LocationService.i>

%include <osmscout/routing/DBFileOffset.i>
%include <osmscout/routing/Route.i>
%include <osmscout/routing/RouteData.i>
%include <osmscout/routing/RouteNode.i>
%include <osmscout/routing/RoutingProfile.i>
%include <osmscout/routing/RoutePostprocessor.i>
%include <osmscout/routing/RoutingService.i>
%include <osmscout/routing/AbstractRoutingService.i>
%include <osmscout/routing/SimpleRoutingService.i>

