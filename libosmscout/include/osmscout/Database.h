#ifndef OSMSCOUT_DATABASE_H
#define OSMSCOUT_DATABASE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>
#include <string_view>

// Type and style sheet configuration
#include <osmscout/OSMScoutTypes.h>
#include <osmscout/TypeConfig.h>

// Datafiles
#include <osmscout/AreaDataFile.h>
#include <osmscout/BoundingBoxDataFile.h>
#include <osmscout/NodeDataFile.h>
#include <osmscout/WayDataFile.h>
#include <osmscout/RouteDataFile.h>

#include <osmscout/OptimizeAreasLowZoom.h>
#include <osmscout/OptimizeWaysLowZoom.h>

// In area index
#include <osmscout/AreaAreaIndex.h>
#include <osmscout/AreaNodeIndex.h>
#include <osmscout/AreaWayIndex.h>
#include <osmscout/AreaRouteIndex.h>

// Location index
#include <osmscout/LocationIndex.h>

// Water index
#include <osmscout/WaterIndex.h>

// SRTM index
#include <osmscout/SRTM.h>

#include <osmscout/routing/RouteDescription.h>

#include <osmscout/util/GeoBox.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \defgroup Database Database
   *
   * Classes and functions for storing and indexing of OSM objects into
   * database-like on-disk data structures.
   */

  /**
    Database instance initialization parameter to influence the behavior of the database
    instance.

    The following attributes are currently available:
    * cache sizes.
    */
  class OSMSCOUT_API DatabaseParameter CLASS_FINAL
  {
  private:
    unsigned long areaAreaIndexCacheSize=5000;

    unsigned long nodeDataCacheSize=5000;
    unsigned long wayDataCacheSize=40000;
    unsigned long areaDataCacheSize=5000;
    unsigned long routeDataCacheSize=1500;

    bool routerDataMMap=true;
    bool nodesDataMMap=true;
    bool areasDataMMap=true;
    bool waysDataMMap=true;
    bool routesDataMMap=true;
    bool optimizeLowZoomMMap=true;
    bool indexMMap=true;

    // temporary, until we have our own database file
    std::string srtmDirectory;

  public:
    DatabaseParameter() = default;

    void SetAreaAreaIndexCacheSize(unsigned long areaAreaIndexCacheSize);
    void SetNodeDataCacheSize(unsigned long  size);
    void SetWayDataCacheSize(unsigned long  size);
    void SetAreaDataCacheSize(unsigned long  size);
    void SetRouteDataCacheSize(unsigned long  size);

    void SetRouterDataMMap(bool mmap);
    void SetNodesDataMMap(bool mmap);
    void SetAreasDataMMap(bool mmap);
    void SetWaysDataMMap(bool mmap);
    void SetRoutesDataMMap(bool mmap);
    void SetOptimizeLowZoomMMap(bool mmap);
    void SetIndexMMap(bool mmap);

    // Temporary
    void SetSRTMDirectory(const std::string& directory)
    {
      this->srtmDirectory=directory;
    }

    unsigned long GetAreaAreaIndexCacheSize() const;
    unsigned long GetNodeDataCacheSize() const;
    unsigned long GetWayDataCacheSize() const;
    unsigned long GetRouteDataCacheSize() const;
    unsigned long GetAreaDataCacheSize() const;

    bool GetRouterDataMMap() const;
    bool GetNodesDataMMap() const;
    bool GetAreasDataMMap() const;
    bool GetWaysDataMMap() const;
    bool GetRoutesDataMMap() const;
    bool GetOptimizeLowZoomMMap() const;
    bool GetIndexMMap() const;

    // Temporary
    std::string GetSRTMDirectory() const
    {
      return srtmDirectory;
    }
  };

  class Database;

  class OSMSCOUT_API NodeRegionSearchResultEntry
  {
  private:
    NodeRef  node;
    Distance distance;

  private:
    explicit NodeRegionSearchResultEntry(const NodeRef& node,
                                         const Distance &distance);

  public:
    friend Database;

    NodeRef GetNode() const
    {
      return node;
    }

    Distance GetDistance() const
    {
      return distance;
    }
  };

  class OSMSCOUT_API NodeRegionSearchResult
  {
  private:
    std::list<NodeRegionSearchResultEntry> nodeResults;

  public:
    friend Database;

    std::list<NodeRegionSearchResultEntry> GetNodeResults() const
    {
      return nodeResults;
    }
  };


  class OSMSCOUT_API WayRegionSearchResultEntry
  {
  private:
    WayRef   way;
    Distance distance;
    GeoCoord closestPoint;

  private:
    explicit WayRegionSearchResultEntry(const WayRef &way,
                                        const Distance &distance,
                                        const GeoCoord &closestPoint);

  public:
    friend Database;

    WayRef GetWay() const
    {
      return way;
    }

    Distance GetDistance() const
    {
      return distance;
    }

    GeoCoord GetClosestPoint() const
    {
      return closestPoint;
    }
  };

  class OSMSCOUT_API WayRegionSearchResult
  {
  private:
    std::list<WayRegionSearchResultEntry> wayResults;

  public:
    friend Database;

    std::list<WayRegionSearchResultEntry> GetWayResults() const
    {
      return wayResults;
    }
  };

  class OSMSCOUT_API AreaRegionSearchResultEntry
  {
  private:
    AreaRef  area;
    Distance distance;
    GeoCoord closestPoint;
    bool     inArea;

  private:
    explicit AreaRegionSearchResultEntry(const AreaRef &area,
                                         const Distance &distance,
                                         const GeoCoord &closestPoint,
                                         bool inArea);

  public:
    friend Database;

    AreaRef GetArea() const
    {
      return area;
    }

    Distance GetDistance() const
    {
      return distance;
    }

    GeoCoord GetClosestPoint() const
    {
      return closestPoint;
    }

    bool IsInArea() const
    {
      return inArea;
    }
  };

  class OSMSCOUT_API AreaRegionSearchResult
  {
  private:
    std::list<AreaRegionSearchResultEntry> areaResults;

  public:
    friend Database;

    inline std::list<AreaRegionSearchResultEntry> GetAreaResults() const
    {
      return areaResults;
    }
  };

  /**
   * \ingroup Database
   *
   * Central access class to all the individual data files and indexes.
   *
   * A database is mainly initialized with a number of optional but performance
   * relevant parameters.
   *
   * The Database is opened by passing the directory that contains
   * all database files.
   */
  class OSMSCOUT_API Database CLASS_FINAL
  {
  private:
    DatabaseParameter               parameter;                //!< Parameterization of this database object

    std::string                     path;                     //!< Path to the directory containing all files
    bool                            isOpen=false;             //!< true, if opened

    TypeConfigRef                   typeConfig;               //!< Type config for the currently opened map

    mutable BoundingBoxDataFileRef  boundingBoxDataFile;      //!< Cached access to the bounding box data file
    mutable std::mutex              boundingBoxDataFileMutex; //!< Mutex to make lazy initialisation of node DataFile thread-safe

    mutable NodeDataFileRef         nodeDataFile;             //!< Cached access to the 'nodes.dat' file
    mutable std::mutex              nodeDataFileMutex;        //!< Mutex to make lazy initialisation of node DataFile thread-safe

    mutable AreaDataFileRef         areaDataFile;             //!< Cached access to the 'areas.dat' file
    mutable std::mutex              areaDataFileMutex;        //!< Mutex to make lazy initialisation of area DataFile thread-safe

    mutable WayDataFileRef          wayDataFile;              //!< Cached access to the 'ways.dat' file
    mutable std::mutex              wayDataFileMutex;         //!< Mutex to make lazy initialisation of way DataFile thread-safe

    mutable RouteDataFileRef        routeDataFile;            //!< Cached access to the 'routes.dat' file
    mutable std::mutex              routeDataFileMutex;       //!< Mutex to make lazy initialisation of route DataFile thread-safe

    mutable AreaNodeIndexRef        areaNodeIndex;            //!< Index of nodes by containing area
    mutable std::mutex              areaNodeIndexMutex;       //!< Mutex to make lazy initialisation of area node index thread-safe

    mutable AreaWayIndexRef         areaWayIndex;             //!< Index of areas by containing area
    mutable std::mutex              areaWayIndexMutex;        //!< Mutex to make lazy initialisation of area way index thread-safe

    mutable AreaRouteIndexRef       areaRouteIndex;           //!< Index of routes by containing area
    mutable std::mutex              areaRouteIndexMutex;      //!< Mutex to make lazy initialisation of area route index thread-safe

    mutable AreaAreaIndexRef        areaAreaIndex;            //!< Index of ways by containing area
    mutable std::mutex              areaAreaIndexMutex;       //!< Mutex to make lazy initialisation of area area index thread-safe

    mutable LocationIndexRef        locationIndex;            //!< Location-based index
    mutable std::mutex              locationIndexMutex;       //!< Mutex to make lazy initialisation of location index thread-safe

    mutable WaterIndexRef           waterIndex;               //!< Index of land/sea tiles
    mutable std::mutex              waterIndexMutex;          //!< Mutex to make lazy initialisation of water index thread-safe

    mutable OptimizeAreasLowZoomRef optimizeAreasLowZoom;     //!< Optimized data for low zoom situations
    mutable std::mutex              optimizeAreasMutex;       //!< Mutex to make lazy initialisation of optimized areas index thread-safe

    mutable OptimizeWaysLowZoomRef  optimizeWaysLowZoom;      //!< Optimized data for low zoom situations
    mutable std::mutex              optimizeWaysMutex;        //!< Mutex to make lazy initialisation of optimized ways index thread-safe

    mutable SRTMRef                 srtmIndex;
    mutable std::mutex              srtmIndexMutex;           //!< Mutex to make lazy initialisation of optimized ways index thread-safe

  private:
    template<typename DataFile, typename OffsetsCol, typename DataCol>
    bool GetObjectsByOffset(DataFile dataFile,
                            const OffsetsCol& offsets,
                            DataCol& objects,
                            const std::string_view &typeName) const
    {
      if (!dataFile) {
        return false;
      }

      StopClock time;

      bool result=dataFile->GetByOffset(offsets.begin(), offsets.end(), offsets.size(), objects);

      if (time.GetMilliseconds()>100) {
        log.Warn() << "Retrieving " << objects.size() << " " << typeName << " by offset took " << time.ResultString();
      }

      return result;
    }

  public:
    explicit Database(const DatabaseParameter& parameter);
    ~Database();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    std::string GetPath() const;
    TypeConfigRef GetTypeConfig() const;

    const DatabaseParameter& GetParameter() const
    {
      return parameter;
    }

    BoundingBoxDataFileRef GetBoundingBoxDataFile() const;

    NodeDataFileRef GetNodeDataFile() const;
    AreaDataFileRef GetAreaDataFile() const;
    WayDataFileRef GetWayDataFile() const;
    RouteDataFileRef GetRouteDataFile() const;

    AreaNodeIndexRef GetAreaNodeIndex() const;
    AreaAreaIndexRef GetAreaAreaIndex() const;
    AreaWayIndexRef GetAreaWayIndex() const;
    AreaRouteIndexRef GetAreaRouteIndex() const;

    LocationIndexRef GetLocationIndex() const;

    WaterIndexRef GetWaterIndex() const;

    OptimizeAreasLowZoomRef GetOptimizeAreasLowZoom() const;
    OptimizeWaysLowZoomRef GetOptimizeWaysLowZoom() const;

    SRTMRef GetSRTMIndex() const;

    bool GetBoundingBox(GeoBox& boundingBox) const;

    bool GetNodeByOffset(const FileOffset& offset,
                         NodeRef& node) const;
    bool GetNodesByOffset(const std::vector<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::vector<FileOffset>& offsets,
                          const GeoBox& boundingBox,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::set<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::list<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::set<FileOffset>& offsets,
                          std::unordered_map<FileOffset,NodeRef>& dataMap) const;

    bool GetAreaByOffset(const FileOffset& offset,
                         AreaRef& area) const;

    template<typename OffsetsCol, typename DataCol>
    bool GetAreasByOffset(const OffsetsCol& offsets,
                          DataCol& areas) const
    {
      using namespace std::string_view_literals;
      return GetObjectsByOffset(GetAreaDataFile(), offsets, areas, "areas"sv);
    }

    bool GetAreasByBlockSpan(const DataBlockSpan& span,
                             std::vector<AreaRef>& area) const;
    bool GetAreasByBlockSpans(const std::vector<DataBlockSpan>& spans,
                              std::vector<AreaRef>& areas) const;


    bool GetWayByOffset(const FileOffset& offset,
                        WayRef& way) const;

    template<typename OffsetsCol, typename DataCol>
    bool GetWaysByOffset(const OffsetsCol& offsets,
                         DataCol& ways) const
    {
      using namespace std::string_view_literals;
      return GetObjectsByOffset(GetWayDataFile(), offsets, ways, "ways"sv);
    }

    template<typename OffsetsCol, typename DataCol>
    bool GetRoutesByOffset(const OffsetsCol& offsets,
                           DataCol& routes) const
    {
      using namespace std::string_view_literals;
      return GetObjectsByOffset(GetRouteDataFile(), offsets, routes, "routes"sv);
    }

    /**
     * Load nodes of given types with maximum distance to the given coordinate.
     *
     * @param location
     *    Geo coordinate in the center of the given circle
     * @param types
     *    Set of type to load conadidates for
     * @param maxDistance - lookup distance in meters
     *    Maximum radius from center to search for
     * @return result object
     * @throws OSMScoutException in case of errors
     */
    NodeRegionSearchResult LoadNodesInRadius(const GeoCoord& location,
                                             const TypeInfoSet& types,
                                             Distance maxDistance=Distance::Of<Meter>(100)) const;

    /**
     * Load ways of given types with maximum distance to the given coordinate.
     *
     * @param location
     *    Geo coordinate in the center of the given circle
     * @param types
     *    Set of type to load conadidates for
     * @param maxDistance - lookup distance in meters
     *    Maximum radius from center to search for
     * @return result object
     * @throws OSMScoutException in case of errors
     */
    WayRegionSearchResult LoadWaysInRadius(const GeoCoord& location,
                                           const TypeInfoSet& types,
                                           Distance maxDistance=Distance::Of<Meter>(100)) const;

    /**
     * Load areas of given types with maximum distance to the given coordinate.
     *
     * @param location
     *    Geo coordinate in the center of the given circle
     * @param types
     *    Set of type to load conadidates for
     * @param maxDistance - lookup distance in meters
     *    Maximum radius from center to search for
     * @return result object
     * @throws OSMScoutException in case of errors
     */
    AreaRegionSearchResult LoadAreasInRadius(const GeoCoord& location,
                                             const TypeInfoSet& types,
                                             Distance maxDistance=Distance::Of<Meter>(100)) const;

    /**
     * Load nodes of given types in the given geo box
     * Distance is measured in relation to the center of the bounding box
     *
     * @param types
     *    Set of type to load candidates for
     * @param boundingBox
     *    Geographic area to search in
     * @return result object
     * @throws OSMScoutException in case of errors
     */
    NodeRegionSearchResult LoadNodesInArea(const TypeInfoSet& types,
                                           const GeoBox& boundingBox) const;

    /**
     * Load ways of given types in the given geo box.
     * Distance is measured in relation to the center of the bounding box
     *
     * @param types
     *    Set of type to load candidates for
     * @param boundingBox
     *    Geographic area to search in
     * @return result object
     * @throws OSMScoutException in case of errors
     */
    WayRegionSearchResult LoadWaysInArea(const TypeInfoSet& types,
                                         const GeoBox& boundingBox) const;

    /**
     * Load areas of given types in the given geo box.
     * Distance is measured in relation to the center of the bounding box
     *
     * @param types
     *    Set of type to load candidates for
     * @param boundingBox
     *    Geographic area to search in
     * @return result object
     * @throws OSMScoutException in case of errors
     */
    AreaRegionSearchResult LoadAreasInArea(const TypeInfoSet& types,
                                           const GeoBox& boundingBox) const;

    void DumpStatistics() const;

    void FlushCache();
  };

  //! Reference counted reference to an Database instance
  using DatabaseRef = std::shared_ptr<Database>;

  /**
   * \defgroup Service High level services
   *
   * Services offer a application developer targeted interface for certain topics
   * like handling of POIs, location search, routing,...
   *
   * In general they need at least a reference to a Database objects since they
   * are just convenience APIs on top of the existing data files and indexes.
   *
   */
}

#endif
