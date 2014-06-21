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
#include <set>

// Type and style sheet configuration
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeSet.h>

// Datafiles
#include <osmscout/AreaDataFile.h>
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

#include <osmscout/Route.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/HashMap.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/Reference.h>

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
  class OSMSCOUT_API DatabaseParameter
  {
  private:
    unsigned long areaAreaIndexCacheSize;
    unsigned long areaNodeIndexCacheSize;

    unsigned long nodeCacheSize;

    unsigned long wayCacheSize;

    unsigned long areaCacheSize;

    bool          debugPerformance;

  public:
    DatabaseParameter();

    void SetAreaAreaIndexCacheSize(unsigned long areaAreaIndexCacheSize);
    void SetAreaNodeIndexCacheSize(unsigned long areaNodeIndexCacheSize);

    void SetNodeCacheSize(unsigned long nodeCacheSize);

    void SetWayCacheSize(unsigned long wayCacheSize);

    void SetAreaCacheSize(unsigned long relationCacheSize);

    void SetDebugPerformance(bool debug);

    unsigned long GetAreaAreaIndexCacheSize() const;
    unsigned long GetAreaNodeIndexCacheSize() const;

    unsigned long GetNodeCacheSize() const;

    unsigned long GetWayCacheSize() const;

    unsigned long GetAreaCacheSize() const;

    bool IsDebugPerformance() const;
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
  class OSMSCOUT_API Database : public Referencable
  {
  private:
    DatabaseParameter               parameter;            //! Parameterization of this database object

    std::string                     path;                 //! Path to the directory containing all files
    bool                            isOpen;               //! true, if opened

    TypeConfigRef                   typeConfig;           //! Type config for the currently opened map

    GeoCoord                        minCoord;             //! Bounding box
    GeoCoord                        maxCoord;             //! Bounding box

    mutable NodeDataFileRef         nodeDataFile;         //! Cached access to the 'nodes.dat' file
    mutable AreaDataFileRef         areaDataFile;         //! Cached access to the 'areas.dat' file
    mutable WayDataFileRef          wayDataFile;          //! Cached access to the 'ways.dat' file

    mutable AreaNodeIndexRef        areaNodeIndex;        //! Index of nodes by containing area
    mutable AreaWayIndexRef         areaWayIndex;         //! Index of areas by containing area
    mutable AreaAreaIndexRef        areaAreaIndex;        //! Index of ways by containing area

    mutable LocationIndexRef        locationIndex;        //! Location-based index

    mutable WaterIndexRef           waterIndex;           //! Index of land/sea tiles

    mutable OptimizeAreasLowZoomRef optimizeAreasLowZoom; //! Optimized data for low zoom situations
    mutable OptimizeWaysLowZoomRef  optimizeWaysLowZoom;  //! Optimized data for low zoom situations

  public:
    Database(const DatabaseParameter& parameter);
    virtual ~Database();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    void FlushCache();

    std::string GetPath() const;
    TypeConfigRef GetTypeConfig() const;

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

    bool GetBoundingBox(double& minLat,double& minLon,
                        double& maxLat,double& maxLon) const;

    bool GetNodeByOffset(const FileOffset& offset,
                         NodeRef& node) const;
    bool GetNodesByOffset(const std::vector<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::set<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::list<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::set<FileOffset>& offsets,
                          OSMSCOUT_HASHMAP<FileOffset,NodeRef>& dataMap) const;

    bool GetAreaByOffset(const FileOffset& offset,
                         AreaRef& area) const;
    bool GetAreasByOffset(const std::vector<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;
    bool GetAreasByOffset(const std::set<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;
    bool GetAreasByOffset(const std::list<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;
    bool GetAreasByOffset(const std::set<FileOffset>& offsets,
                          OSMSCOUT_HASHMAP<FileOffset,AreaRef>& dataMap) const;

    bool GetWayByOffset(const FileOffset& offset,
                        WayRef& way) const;
    bool GetWaysByOffset(const std::vector<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;
    bool GetWaysByOffset(const std::set<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;
    bool GetWaysByOffset(const std::list<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;
    bool GetWaysByOffset(const std::set<FileOffset>& offsets,
                         OSMSCOUT_HASHMAP<FileOffset,WayRef>& dataMap) const;

    void DumpStatistics();
  };

  //! Reference counted reference to an Database instance
  typedef Ref<Database> DatabaseRef;

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
