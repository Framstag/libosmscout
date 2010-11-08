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
#include <osmscout/AreaIndex.h>
#include <osmscout/AreaNodeIndex.h>

// Location index
#include <osmscout/CityStreetIndex.h>

// Reverse index
#include <osmscout/NodeUseIndex.h>

// Water index
#include <osmscout/WaterIndex.h>

#include <osmscout/Route.h>

namespace osmscout {

  /**
    Parameter to influence th search result for searching for (drawable)
    objects in a given area.
    */
  class OSMSCOUT_API AreaSearchParameter
  {
  private:
    unsigned long maxWayLevel;
    unsigned long maxAreaLevel;
    unsigned long maxNodes;
    unsigned long maxWays;
    unsigned long maxAreas;

  public:
    AreaSearchParameter();

    void SetMaximumWayLevel(unsigned long maxWayLevel);
    void SetMaximumAreaLevel(unsigned long maxAreaLevel);

    void SetMaximumNodes(unsigned long maxNodes);
    void SetMaximumWays(unsigned long maxWays);
    void SetMaximumAreas(unsigned long maxAreas);

    unsigned long GetMaximumWayLevel() const;
    unsigned long GetMaximumAreaLevel() const;

    unsigned long GetMaximumNodes() const;
    unsigned long GetMaximumWays() const;
    unsigned long GetMaximumAreas() const;
  };

  class OSMSCOUT_API Database
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

    AreaIndex             areaIndex;

    AreaNodeIndex         areaNodeIndex;

    CityStreetIndex       cityStreetIndex;

    NodeUseIndex          nodeUseIndex;

    WaterIndex            waterIndex;

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

    bool GetWays(std::list<FileOffset>& offsets,
                 std::vector<Way>& ways) const;

    bool GetRelations(std::vector<FileOffset>& offsets,
                      std::vector<Relation>& relations) const;

    bool GetRelations(std::list<FileOffset>& offsets,
                      std::vector<Relation>& relations) const;

    bool GetNodes(const StyleConfig& styleConfig,
                  double lonMin, double latMin,
                  double lonMax, double latMax,
                  double magnification,
                  size_t maxPriority,
                  std::vector<Node>& nodes) const;

  public:
    Database();
    virtual ~Database();

    bool Open(const std::string& path,
              std::string (*hashFunction) (std::string) = NULL);
    bool IsOpen() const;
    void Close();

    void FlushCache();

    TypeConfig* GetTypeConfig() const;

    bool GetBoundingBox(double& minLat,double& minLon,
                        double& maxLat,double& maxLon) const;

    bool GetObjects(const StyleConfig& styleConfig,
                    double lonMin, double latMin,
                    double lonMax, double latMax,
                    double magnification,
                    const AreaSearchParameter& parameter,
                    std::vector<Node>& nodes,
                    std::vector<Way>& ways,
                    std::vector<Way>& areas,
                    std::vector<Relation>& relationWays,
                    std::vector<Relation>& realtionAreas) const;

    bool GetGroundTiles(double lonMin, double latMin,
                        double lonMax, double latMax,
                        std::list<GroundTile>& tiles) const;

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
