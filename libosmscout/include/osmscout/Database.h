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

#include <osmscout/Cache.h>

#include <osmscout/StyleConfig.h>
#include <osmscout/TypeConfig.h>

// Datafiles
#include <osmscout/NodeDataFile.h>
#include <osmscout/RelationDataFile.h>
#include <osmscout/WayDataFile.h>

// Fileoffset by Id index
#include <osmscout/NodeIndex.h>
#include <osmscout/WayIndex.h>

// In area index
#include <osmscout/AreaNodeIndex.h>
#include <osmscout/AreaWayIndex.h>
#include <osmscout/AreaWayRelIndex.h>
#include <osmscout/AreaAreaIndex.h>
#include <osmscout/AreaAreaRelIndex.h>

// Location index
#include <osmscout/CityStreetIndex.h>

// Reverse index
#include <osmscout/NodeUseIndex.h>

#include <osmscout/Route.h>

namespace osmscout {
  class Database
  {
  public: // Fix this
    struct NodeUse
    {
      Id              id;
      std::vector<Id> references;
    };

    typedef Cache<size_t,std::vector<NodeUse> > NodeUseCache;

  private:                               //! true, if opened
    bool                  isOpen;

    double                minLon;        //! bounding box of data
    double                minLat;        //! bounding box of data
    double                maxLon;        //! bounding box of data
    double                maxLat;        //! bounding box of data

    AreaNodeIndex         areaNodeIndex;
    AreaAreaIndex         areaAreaIndex;
    AreaWayIndex          areaWayIndex;

    AreaAreaRelIndex      areaAreaRelIndex;
    AreaWayRelIndex       areaWayRelIndex;

    CityStreetIndex       cityStreetIndex;

    NodeUseIndex          nodeUseIndex;

    std::string           path;          //! Path to the directory containing all files

    NodeDataFile          nodeDataFile;  //! Cached access to the 'nodes.dat' file
    RelationDataFile      relationDataFile;//! Cached access to the 'relations.dat' file
    WayDataFile           wayDataFile;   //! Cached access to the 'ways.dat' file

    mutable NodeUseCache  nodeUseCache;  //! Cache for node use data

    mutable FileScanner   nodeUseScanner;//! File stream to the nodeuse.idx file

    TypeConfig            *typeConfig;   //! Type config for the currently opened map

    std::string           (*hashFunction) (std::string);

  private:
    size_t GetMaximumPriority(const StyleConfig& styleConfig,
                              double minlon, double minlat,
                              double maxlon, double maxlat,
                              double magnification,
                              size_t maxNodes) const;

    bool GetWays(std::vector<FileOffset>& offsets,
                 std::vector<Way>& ways) const;

    bool GetRelations(std::vector<FileOffset>& offsets,
                      std::vector<Relation>& relations) const;

    bool GetNodes(const StyleConfig& styleConfig,
                  double lonMin, double latMin,
                  double lonMax, double latMax,
                  double magnification,
                  size_t maxPriority,
                  std::vector<Node>& nodes) const;

    bool GetWays(const StyleConfig& styleConfig,
                 double lonMin, double latMin,
                 double lonMax, double latMax,
                 const std::vector<TypeId>& types,
                 size_t maxCount,
                 std::vector<Way>& ways) const;

    bool GetAreas(const StyleConfig& styleConfig,
                  double lonMin, double latMin,
                  double lonMax, double latMax,
                  size_t maxLevel,
                  size_t maxCount,
                  std::vector<Way>& areas) const;

    bool GetRelationWays(const StyleConfig& styleConfig,
                         double lonMin, double latMin,
                         double lonMax, double latMax,
                         const std::vector<TypeId>& types,
                         size_t maxCount,
                         std::vector<Relation>& relationWays) const;

    bool GetRelationAreas(const StyleConfig& styleConfig,
                          double lonMin, double latMin,
                          double lonMax, double latMax,
                          size_t maxLevel,
                          size_t maxCount,
                          std::vector<Relation>& relationAreas) const;

  public:
    Database();
    virtual ~Database();

    bool Open(const std::string& path, std::string (*hashFunction) (std::string) = NULL);
    bool IsOpen() const;
    void Close();

    TypeConfig* GetTypeConfig() const;

    bool GetBoundingBox(double& minLat,double& minLon,
                        double& maxLat,double& maxLon) const;

    bool GetObjects(const StyleConfig& styleConfig,
                    double lonMin, double latMin,
                    double lonMax, double latMax,
                    double magnification,
                    size_t maxAreaLevel,
                    size_t maxNodes,
                    size_t maxWays,
                    size_t maxAreas,
                    std::vector<Node>& nodes,
                    std::vector<Way>& ways,
                    std::vector<Way>& areas,
                    std::vector<Relation>& relationWays,
                    std::vector<Relation>& realtionAreas) const;

    bool GetNode(const Id& id,
                 Node& node) const;
    bool GetNodes(const std::vector<Id>& ids,
                  std::vector<Node>& nodes) const;

    bool GetWay(const Id& id,
                Way& way) const;
    bool GetWays(const std::vector<Id>& ids,
                 std::vector<Way>& ways) const;
    bool GetWays(const std::set<Id>& ids,
                 std::vector<Way>& ways) const;

    bool GetRelation(const Id& id,
                     Relation& relation) const;
    bool GetRelations(const std::vector<Id>& ids,
                      std::vector<Relation>& relations) const;

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

    bool GetJoints(Id id,
                   std::set<Id>& wayIds) const;
    bool GetJoints(const std::set<Id>& ids,
                   std::set<Id>& wayIds) const;

    bool CalculateRoute(Id startWayId, Id startNodeId,
                        Id targetWayId, Id targetNodeId,
                        RouteData& route);

    bool TransformRouteDataToRouteDescription(const RouteData& data,
                                              RouteDescription& description);
    bool TransformRouteDataToWay(const RouteData& data,
                                 Way& way);

    void DumpStatistics();
  };
}

#endif
