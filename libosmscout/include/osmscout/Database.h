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
    bool                  isOpen;              //! true, if opened
    bool                  debugPerformance;

    double                minLon;              //! bounding box of data
    double                minLat;              //! bounding box of data
    double                maxLon;              //! bounding box of data
    double                maxLat;              //! bounding box of data

    AreaNodeIndex         areaNodeIndex;
    AreaWayIndex          areaWayIndex;
    AreaAreaIndex         areaAreaIndex;

    LocationIndex       cityStreetIndex;

    WaterIndex            waterIndex;

    std::string           path;                 //! Path to the directory containing all files

    NodeDataFile          nodeDataFile;         //! Cached access to the 'nodes.dat' file
    AreaDataFile          areaDataFile;         //! Cached access to the 'areas.dat' file
    WayDataFile           wayDataFile;          //! Cached access to the 'ways.dat' file

    OptimizeAreasLowZoom  optimizeAreasLowZoom; //! Optimized data for low zoom situations
    OptimizeWaysLowZoom   optimizeWaysLowZoom;  //! Optimized data for low zoom situations

    TypeConfig            *typeConfig;          //! Type config for the currently opened map

  private:
    bool GetObjectsNodes(const AreaSearchParameter& parameter,
                         const TypeSet &nodeTypes,
                         double lonMin, double latMin,
                         double lonMax, double latMax,
                         std::string& nodeIndexTime,
                         std::string& nodesTime,
                         std::vector<NodeRef>& nodes) const;

    bool GetObjectsWays(const AreaSearchParameter& parameter,
                        const std::vector<TypeSet>& wayTypes,
                        const Magnification& magnification,
                        double lonMin, double latMin,
                        double lonMax, double latMax,
                        std::string& wayOptimizedTime,
                        std::string& wayIndexTime,
                        std::string& waysTime,
                        std::vector<WayRef>& ways) const;

    bool GetObjectsAreas(const AreaSearchParameter& parameter,
                               const TypeSet& areaTypes,
                               const Magnification& magnification,
                               double lonMin, double latMin,
                               double lonMax, double latMax,
                               std::string& areaOptimizedTime,
                               std::string& areaIndexTime,
                               std::string& areasTime,
                               std::vector<AreaRef>& areas) const;

    bool HandleAdminRegion(const LocationSearch& search,
                           const LocationSearch::Entry& searchEntry,
                           const osmscout::AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                           LocationSearchResult& result) const;

    bool HandleAdminRegionLocation(const LocationSearch& search,
                                   const LocationSearch::Entry& searchEntry,
                                   const osmscout::AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                   const osmscout::LocationMatchVisitor::LocationResult& locationResult,
                                   LocationSearchResult& result) const;

    bool HandleAdminRegionPOI(const LocationSearch& search,
                              const osmscout::AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                              const osmscout::LocationMatchVisitor::POIResult& poiResult,
                              LocationSearchResult& result) const;

    bool HandleAdminRegionLocationAddress(const LocationSearch& search,
                                          const osmscout::AdminRegionMatchVisitor::AdminRegionResult& adminRegionResult,
                                          const osmscout::LocationMatchVisitor::LocationResult& locationResult,
                                          const osmscout::AddressMatchVisitor::AddressResult& addressResult,
                                          LocationSearchResult& result) const;

  public:
    Database(const DatabaseParameter& parameter);
    virtual ~Database();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    void FlushCache();

    std::string GetPath() const;
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

    bool GetObjects(const AreaSearchParameter& parameter,
                    const Magnification& magnification,
                    const TypeSet &nodeTypes,
                    double nodeLonMin, double nodeLatMin,
                    double nodeLonMax, double nodeLatMax,
                    std::vector<NodeRef>& nodes,
                    const std::vector<TypeSet>& wayTypes,
                    double wayLonMin, double wayLatMin,
                    double wayLonMax, double wayLatMax,
                    std::vector<WayRef>& ways,
                    const TypeSet& areaTypes,
                    double areaLonMin, double areaLatMin,
                    double areaLonMax, double areaLatMax,
                    std::vector<AreaRef>& areas) const;

    bool GetObjects(double lonMin, double latMin,
                    double lonMax, double latMax,
                    const TypeSet& types,
                    std::vector<NodeRef>& nodes,
                    std::vector<WayRef>& ways,
                    std::vector<AreaRef>& areas) const;

    bool GetObjects(const std::set<ObjectFileRef>& objects,
                    OSMSCOUT_HASHMAP<FileOffset,NodeRef>& nodesMap,
                    OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areasMap,
                    OSMSCOUT_HASHMAP<FileOffset,WayRef>& waysMap) const;

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

    bool VisitAdminRegions(AdminRegionVisitor& visitor) const;

    bool VisitAdminRegionLocations(const AdminRegion& region,
                                   LocationVisitor& visitor) const;

    bool VisitLocationAddresses(const Location& location,
                                AddressVisitor& visitor) const;

    bool ResolveAdminRegionHierachie(const AdminRegionRef& adminRegion,
                                     std::map<FileOffset,AdminRegionRef >& refs) const;

    bool SearchForLocations(const LocationSearch& search,
                            LocationSearchResult& result) const;

    bool GetClosestRoutableNode(double lat,
                                double lon,
                                const osmscout::Vehicle& vehicle,
                                double radius,
                                osmscout::ObjectFileRef& object,
                                size_t& nodeIndex) const;

    void DumpStatistics();
  };
}

#endif
