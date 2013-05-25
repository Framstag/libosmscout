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

#include <osmscout/OptimizeLowZoom.h>

// In area index
#include <osmscout/AreaAreaIndex.h>
#include <osmscout/AreaNodeIndex.h>
#include <osmscout/AreaWayIndex.h>

// Location index
#include <osmscout/CityStreetIndex.h>

// Water index
#include <osmscout/WaterIndex.h>

#include <osmscout/Route.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/StopClock.h>

namespace osmscout {

  /**
    Database instance initialisation parameter to influence the behaviour of the database
    instance.

    The following groups attributes are currently available:
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
    Parameter to influence the search result for searching for (drawable)
    objects in a given area.
    */
  class OSMSCOUT_API AreaSearchParameter
  {
  private:
    unsigned long maxAreaLevel;
    unsigned long maxNodes;
    unsigned long maxWays;
    unsigned long maxAreas;
    bool          useLowZoomOptimization;
    BreakerRef    breaker;
    bool          useMultithreading;

  public:
    AreaSearchParameter();

    void SetMaximumAreaLevel(unsigned long maxAreaLevel);

    void SetMaximumNodes(unsigned long maxNodes);
    void SetMaximumWays(unsigned long maxWays);
    void SetMaximumAreas(unsigned long maxAreas);

    void SetUseLowZoomOptimization(bool useLowZoomOptimization);

    void SetUseMultithreading(bool useMultithreading);

    void SetBreaker(const BreakerRef& breaker);

    unsigned long GetMaximumAreaLevel() const;

    unsigned long GetMaximumNodes() const;
    unsigned long GetMaximumWays() const;
    unsigned long GetMaximumAreas() const;

    bool GetUseLowZoomOptimization() const;

    bool GetUseMultithreading() const;

    bool IsAborted() const;
  };

  class OSMSCOUT_API Database
  {
  private:
    bool                  isOpen;          //! true, if opened
    bool                  debugPerformance;

    double                minLon;          //! bounding box of data
    double                minLat;          //! bounding box of data
    double                maxLon;          //! bounding box of data
    double                maxLat;          //! bounding box of data

    AreaNodeIndex         areaNodeIndex;
    AreaWayIndex          areaWayIndex;
    AreaAreaIndex         areaAreaIndex;

    CityStreetIndex       cityStreetIndex;

    WaterIndex            waterIndex;

    std::string           path;             //! Path to the directory containing all files

    NodeDataFile          nodeDataFile;     //! Cached access to the 'nodes.dat' file
    AreaDataFile          areaDataFile;     //! Cached access to the 'areas.dat' file
    WayDataFile           wayDataFile;      //! Cached access to the 'ways.dat' file

    OptimizeLowZoom       optimizeLowZoom;  //! Optimized data for low zoom situations

    TypeConfig            *typeConfig;      //! Type config for the currently opened map

    std::string           (*hashFunction) (std::string);

  private:
    bool GetObjectsNodes(const AreaSearchParameter& parameter,
                         const TypeSet &nodeTypes,
                         double lonMin, double latMin,
                         double lonMax, double latMax,
                         std::string& nodeIndexTime,
                         std::string& nodesTime,
                         std::vector<NodeRef>& nodes) const;

    bool GetObjectsWayOffsets(const AreaSearchParameter& parameter,
                              const std::vector<TypeSet>& wayTypes,
                              const Magnification& magnification,
                              double lonMin, double latMin,
                              double lonMax, double latMax,
                              std::string& wayOptimizedTime,
                              std::string& wayIndexTime,
                              std::vector<FileOffset>& wayWayOffsets,
                              std::vector<WayRef>& ways) const;

    bool GetObjectsAreaOffsets(const AreaSearchParameter& parameter,
                               const TypeSet& areaTypes,
                               const Magnification& magnification,
                               double lonMin, double latMin,
                               double lonMax, double latMax,
                               std::string& areaIndexTime,
                               std::vector<FileOffset>& areaOffsets) const;

    bool GetObjectsWaysAndAreas(const AreaSearchParameter& parameter,
                                const std::vector<FileOffset>& wayWayOffsets,
                                const std::vector<FileOffset>& wayAreaOffsets,
                                std::string& waysTime,
                                std::string& areasTime,
                                std::vector<WayRef>& ways,
                                std::vector<AreaRef>& areas) const;

  public:
    Database(const DatabaseParameter& parameter);
    virtual ~Database();

    bool Open(const std::string& path,
              std::string (*hashFunction) (std::string) = NULL);
    bool IsOpen() const;
    void Close();

    void FlushCache();

    TypeConfig* GetTypeConfig() const;

    bool GetBoundingBox(double& minLat,double& minLon,
                        double& maxLat,double& maxLon) const;

    bool GetObjects(const TypeSet &nodeTypes,
                    const std::vector<TypeSet>& wayTypes,
                    const TypeSet& areaTypes,
                    double lonMin, double latMin,
                    double lonMax, double latMax,
                    const Magnification& magnification,
                    const AreaSearchParameter& parameter,
                    std::vector<NodeRef>& nodes,
                    std::vector<WayRef>& ways,
                    std::vector<AreaRef>& areas) const;

    bool GetObjects(double lonMin, double latMin,
                    double lonMax, double latMax,
                    const TypeSet& types,
                    std::vector<NodeRef>& nodes,
                    std::vector<WayRef>& ways,
                    std::vector<AreaRef>& areas) const;

    bool GetGroundTiles(double lonMin, double latMin,
                        double lonMax, double latMax,
                        const Magnification& magnification,
                        std::list<GroundTile>& tiles) const;

    bool GetNodeByOffset(const FileOffset& offset,
                         NodeRef& node) const;
    bool GetNodesByOffset(const std::vector<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::set<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;
    bool GetNodesByOffset(const std::list<FileOffset>& offsets,
                          std::vector<NodeRef>& nodes) const;

    bool GetAreaByOffset(const FileOffset& offset,
                         AreaRef& area) const;
    bool GetAreasByOffset(const std::vector<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;
    bool GetAreasByOffset(const std::set<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;
    bool GetAreasByOffset(const std::list<FileOffset>& offsets,
                          std::vector<AreaRef>& areas) const;

    bool GetWayByOffset(const FileOffset& offset,
                        WayRef& way) const;
    bool GetWaysByOffset(const std::vector<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;
    bool GetWaysByOffset(const std::set<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;
    bool GetWaysByOffset(const std::list<FileOffset>& offsets,
                         std::vector<WayRef>& ways) const;

    bool GetMatchingAdminRegions(const std::string& name,
                                 std::list<AdminRegion>& regions,
                                 size_t limit,
                                 bool& limitReached,
                                 bool startWith) const;

    bool GetMatchingLocations(const AdminRegion& region,
                              const std::string& name,
                              std::list<Location>& locations,
                              size_t limit,
                              bool& limitReached,
                              bool startWith) const;

    void DumpStatistics();
  };
}

#endif
