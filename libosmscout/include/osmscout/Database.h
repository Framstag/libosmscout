#ifndef OSMSCOUT_DATABASE_H
#define OSMSCOUT_DATABASE_H

/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <list>
#include <set>

#include <osmscout/Cache.h>

#include <osmscout/FileReader.h>

#include <osmscout/StyleConfig.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/Node.h>
#include <osmscout/Way.h>

// Fileoffset by Id index
#include <osmscout/NumericIndex.h>

// In area index
#include <osmscout/AreaNodeIndex.h>
#include <osmscout/AreaWayIndex.h>
#include <osmscout/AreaAreaIndex.h>

// Location index
#include <osmscout/CityStreetIndex.h>

// Reverse index
#include <osmscout/NodeUseIndex.h>

#include <osmscout/Route.h>

class Database
{
public: // Fix this
  struct NodeUse
  {
    Id              id;
    std::vector<Id> references;
  };

  typedef Cache<FileOffset,Node>              NodeCache;
  typedef Cache<FileOffset,Way>               WayCache;
  typedef Cache<size_t,std::vector<NodeUse> > NodeUseCache;

  typedef NumericIndex<Id,Node>               NodeIndex;
  typedef NumericIndex<Id,Way>                WayIndex;

  typedef const Node*                         NodeRef;
  typedef const Way*                          WayRef;

private:
  bool                  isOpen;
  NodeIndex             nodeIndex;
  WayIndex              wayIndex;

  AreaNodeIndex         areaNodeIndex;
  AreaAreaIndex         areaAreaIndex;
  AreaWayIndex          areaWayIndex;

  CityStreetIndex       cityStreetIndex;

  NodeUseIndex          nodeUseIndex;

  std::string           path;          //! Path to the directory containing all files

  mutable NodeCache     nodeCache;    //! Cache for node data
  mutable WayCache      wayCache;     //! Cache for way data
  mutable NodeUseCache  nodeUseCache;  //! Cache for node use data

  mutable FileReader    nodeUseReader; //! File stream to the nodeuse.idx file

  mutable FileScanner   nodeScanner;   //! File stream to the node.dat file
  mutable FileScanner   wayScanner;    //! File stream to the way.dat file

  TypeConfig            *typeConfig;   //! Type config for the currently opened map

private:
  size_t GetMaximumPriority(const StyleConfig& styleConfig,
                            double minlon, double minlat,
                            double maxlon, double maxlat,
                            double magnification,
                            size_t maxNodes) const;

  bool GetWays(std::vector<FileOffset>& offsets,
               std::vector<Way>& ways) const;

  bool GetNodes(const StyleConfig& styleConfig,
                double lonMin, double latMin,
                double lonMax, double latMax,
                double magnification,
                size_t maxPriority,
                std::vector<Node>& nodes) const;

  bool GetWays(const StyleConfig& styleConfig,
               double lonMin, double latMin,
               double lonMax, double latMax,
               double magnification,
               size_t maxPriority,
               std::vector<Way>& ways) const;

  bool GetAreas(const StyleConfig& styleConfig,
                double lonMin, double latMin,
                double lonMax, double latMax,
                size_t maxLevel,
                size_t maxCount,
                std::vector<Way>& areas) const;

public:
  Database();
  virtual ~Database();

  bool Open(const std::string& path);
  bool IsOpen() const;
  void Close();

  TypeConfig* GetTypeConfig() const;

  bool GetObjects(const StyleConfig& styleConfig,
                  double lonMin, double latMin,
                  double lonMax, double latMax,
                  double magnification,
                  size_t maxAreaLevel,
                  size_t maxNodes,
                  size_t maxAreas,
                  std::vector<Node>& nodes,
                  std::vector<Way>& ways,
                  std::vector<Way>& areas) const;

  bool GetNode(const Id& id,
               Node& node) const;
  bool GetNodes(const std::vector<Id>& ids,
                std::vector<Node>& nodes) const;
  bool GetWay(const Id& id,
              Way& way) const;
  bool GetWays(const std::vector<Id>& ids,
               std::vector<Way>& ways) const;

  bool GetMatchingCities(const std::string& name,
                         std::list<City>& cities,
                         size_t limit, bool& limitReached) const;
  bool GetMatchingStreets(Id urbanId, const std::string& name,
                          std::list<Street>& streets,
                          size_t limit, bool& limitReached) const;
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

#endif
