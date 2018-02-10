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

// Type and style sheet configuration
#include <osmscout/Types.h>
#include <osmscout/TypeConfig.h>

// Datafiles
#include <osmscout/AreaDataFile.h>
#include <osmscout/BoundingBoxDataFile.h>
#include <osmscout/NodeDataFile.h>
#include <osmscout/WayDataFile.h>

#include <osmscout/OptimizeAreasLowZoom.h>
#include <osmscout/OptimizeWaysLowZoom.h>

// In area index
#include <osmscout/AreaAreaIndex.h>
#include <osmscout/AreaNodeIndex.h>
#include <osmscout/AreaWayIndex.h>

// Location index
#include <osmscout/LocationIndex.h>

// Water index
#include <osmscout/WaterIndex.h>

#include <osmscout/routing/Route.h>

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
    unsigned long areaAreaIndexCacheSize;

    unsigned long nodeDataCacheSize;
    unsigned long wayDataCacheSize;
    unsigned long areaDataCacheSize;

    bool routerDataMMap;
    bool nodesDataMMap;
    bool areasDataMMap;
    bool waysDataMMap;
    bool optimizeLowZoomMMap;
    bool indexMMap;
  public:
    DatabaseParameter();

    void SetAreaAreaIndexCacheSize(unsigned long areaAreaIndexCacheSize);
    void SetNodeDataCacheSize(unsigned long  size);
    void SetWayDataCacheSize(unsigned long  size);
    void SetAreaDataCacheSize(unsigned long  size);

    void SetRouterDataMMap(bool mmap);
    void SetNodesDataMMap(bool mmap);
    void SetAreasDataMMap(bool mmap);
    void SetWaysDataMMap(bool mmap);
    void SetOptimizeLowZoomMMap(bool mmap);
    void SetIndexMMap(bool mmap);

    unsigned long GetAreaAreaIndexCacheSize() const;
    unsigned long GetNodeDataCacheSize() const;
    unsigned long GetWayDataCacheSize() const;
    unsigned long GetAreaDataCacheSize() const;

    bool GetRouterDataMMap() const;
    bool GetNodesDataMMap() const;
    bool GetAreasDataMMap() const;
    bool GetWaysDataMMap() const;
    bool GetOptimizeLowZoomMMap() const;
    bool GetIndexMMap() const;
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
    bool                            isOpen;                   //!< true, if opened

    TypeConfigRef                   typeConfig;               //!< Type config for the currently opened map

    mutable BoundingBoxDataFileRef  boundingBoxDataFile;      //!< Cached access to the bounding box data file
    mutable std::mutex              boundingBoxDataFileMutex; //!< Mutex to make lazy initialisation of node DataFile thread-safe

    mutable NodeDataFileRef         nodeDataFile;             //!< Cached access to the 'nodes.dat' file
    mutable std::mutex              nodeDataFileMutex;        //!< Mutex to make lazy initialisation of node DataFile thread-safe

    mutable AreaDataFileRef         areaDataFile;             //!< Cached access to the 'areas.dat' file
    mutable std::mutex              areaDataFileMutex;        //!< Mutex to make lazy initialisation of area DataFile thread-safe

    mutable WayDataFileRef          wayDataFile;              //!< Cached access to the 'ways.dat' file
    mutable std::mutex              wayDataFileMutex;         //!< Mutex to make lazy initialisation of way DataFile thread-safe

    mutable AreaNodeIndexRef        areaNodeIndex;            //!< Index of nodes by containing area
    mutable std::mutex              areaNodeIndexMutex;       //!< Mutex to make lazy initialisation of area node index thread-safe

    mutable AreaWayIndexRef         areaWayIndex;             //!< Index of areas by containing area
    mutable std::mutex              areaWayIndexMutex;        //!< Mutex to make lazy initialisation of area way index thread-safe

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

  public:
    Database(const DatabaseParameter& parameter);
    virtual ~Database();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    std::string GetPath() const;
    TypeConfigRef GetTypeConfig() const;

    inline const DatabaseParameter& GetParameter() const
    {
      return parameter;
    }

    BoundingBoxDataFileRef GetBoundingBoxDataFile() const;

    NodeDataFileRef GetNodeDataFile() const;
    AreaDataFileRef GetAreaDataFile() const;
    WayDataFileRef GetWayDataFile() const;

    AreaNodeIndexRef GetAreaNodeIndex() const;
    AreaAreaIndexRef GetAreaAreaIndex() const;
    AreaWayIndexRef GetAreaWayIndex() const;

    LocationIndexRef GetLocationIndex() const;

    WaterIndexRef GetWaterIndex() const;

    OptimizeAreasLowZoomRef GetOptimizeAreasLowZoom() const;
    OptimizeWaysLowZoomRef GetOptimizeWaysLowZoom() const;

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
    bool GetAreasByOffset(const std::vector<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;
    bool GetAreasByOffset(const std::set<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;
    bool GetAreasByOffset(const std::list<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;
    bool GetAreasByOffset(const std::set<FileOffset>& offsets,
                          std::unordered_map<FileOffset,AreaRef>& dataMap) const;
    bool GetAreasByBlockSpan(const DataBlockSpan& span,
                             std::vector<AreaRef>& area) const;
    bool GetAreasByBlockSpans(const std::vector<DataBlockSpan>& spans,
                              std::vector<AreaRef>& areas) const;


    bool GetWayByOffset(const FileOffset& offset,
                        WayRef& way) const;
    bool GetWaysByOffset(const std::vector<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;
    bool GetWaysByOffset(const std::set<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;
    bool GetWaysByOffset(const std::list<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;
    bool GetWaysByOffset(const std::set<FileOffset>& offsets,
                         std::unordered_map<FileOffset,WayRef>& dataMap) const;

    void DumpStatistics();
  };

  //! Reference counted reference to an Database instance
  typedef std::shared_ptr<Database> DatabaseRef;

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
