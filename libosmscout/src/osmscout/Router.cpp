/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <osmscout/Router.h>

#include <algorithm>
#include <cassert>
#include <iostream>

#include <osmscout/ObjectRef.h>
#include <osmscout/RoutingProfile.h>
#include <osmscout/TypeConfigLoader.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/private/Math.h>

#include <iomanip>
#include <limits>
namespace osmscout {

  RouterParameter::RouterParameter()
  : wayIndexCacheSize(10000),
    wayCacheSize(0),
    debugPerformance(false)
  {
    // no code
  }

  void RouterParameter::SetWayIndexCacheSize(unsigned long wayIndexCacheSize)
  {
    this->wayIndexCacheSize=wayIndexCacheSize;
  }

  void RouterParameter::SetWayCacheSize(unsigned long wayCacheSize)
  {
    this->wayCacheSize=wayCacheSize;
  }

  void RouterParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  unsigned long RouterParameter::GetWayIndexCacheSize() const
  {
    return wayIndexCacheSize;
  }

  unsigned long RouterParameter::GetWayCacheSize() const
  {
    return wayCacheSize;
  }

  bool RouterParameter::IsDebugPerformance() const
  {
    return debugPerformance;
  }

  Router::Router(const RouterParameter& parameter)
   : isOpen(false),
     debugPerformance(parameter.IsDebugPerformance()),
     wayDataFile("ways.dat",
                 "way.idx",
                  parameter.GetWayCacheSize(),
                  parameter.GetWayIndexCacheSize()),
     routeNodeDataFile("route.dat",
                       "route.idx",
                       0,
                       6000),
     typeConfig(NULL)
  {
    // no code
  }

  Router::~Router()
  {
    delete typeConfig;
  }

  bool Router::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    typeConfig=new TypeConfig();

    if (!LoadTypeData(path,*typeConfig)) {
      std::cerr << "Cannot load 'types.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    std::cout << "Opening 'ways.dat'..." << std::endl;
    if (!wayDataFile.Open(path,true,false)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }
    std::cout << "Opening 'ways.dat' done." << std::endl;

    std::cout << "Opening 'route.dat'..." << std::endl;
    if (!routeNodeDataFile.Open(path,true,false)) {
      std::cerr << "Cannot open 'route.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }
    std::cout << "Opening 'route.dat' done." << std::endl;

    isOpen=true;

    return true;
  }

  bool Router::IsOpen() const
  {
    return isOpen;
  }


  void Router::Close()
  {
    routeNodeDataFile.Close();
    wayDataFile.Close();

    isOpen=false;
  }

  void Router::FlushCache()
  {
    wayDataFile.FlushCache();
  }

  TypeConfig* Router::GetTypeConfig() const
  {
    return typeConfig;
  }

  /**
   * A node in the routing graph, normally a node as part of a way
   */
  struct RNode
  {
    Id        nodeId;
    Id        wayId;
    double    currentCost;
    double    estimateCost;
    double    overallCost;
    Id        prev;

    RNode()
     : nodeId(0),
       wayId(0)
    {
    }

    RNode(Id nodeId,
          Id wayId,
          Id prev)
     : nodeId(nodeId),
       wayId(wayId),
       currentCost(0),
       estimateCost(0),
       overallCost(0),
       prev(prev)
    {
      // no code
    }

    inline bool operator==(const RNode& other)
    {
      return nodeId==other.nodeId;
    }

    inline bool operator<(const RNode& other) const
    {
      return nodeId<other.nodeId;
    }
  };

  struct RNodeCostCompare
  {
    inline bool operator()(const RNode& a, const RNode& b) const
    {
      if (a.overallCost==b.overallCost) {
       return a.nodeId<b.nodeId;
      }
      else {
        return a.overallCost<b.overallCost;
      }
    }
  };

  bool CanBeTurnedInto(const Way& way, Id via, Id to)
  {
    if (way.restrictions.empty()) {
      return true;
    }

    for (std::vector<Way::Restriction>::const_iterator iter=way.restrictions.begin();
         iter!=way.restrictions.end();
         ++iter) {
      if (iter->type==Way::rstrAllowTurn) {
        // If our "to" is restriction "to" and our via is in the list of restriction "vias"
        // we can turn, else not.
        // If our !"to" is not the "to" of our restriction we also cannot turn.
        if (iter->members[0]==to) {
          for (size_t i=1; i<iter->members.size(); i++) {
            if (iter->members[i]==via) {
              return true;
            }
          }

          return false;
        }
        else {
          return false;
        }
      }
      else if (iter->type==Way::rstrForbitTurn) {
        // If our "to" is the restriction "to" and our "via" is in the list of the restriction "vias"
        // we cannot turn.
        if (iter->members[0]==to) {
          for (size_t i=1; i<iter->members.size(); i++) {
            if (iter->members[i]==via) {
              return false;
            }
          }
        }
      }
    }

    return true;
  }

  typedef std::set<RNode,RNodeCostCompare> OpenList;
  typedef OpenList::iterator               RNodeRef;

  struct RouteStep
  {
    Id wayId;
    Id nodeId;
  };

  void Router::GetClosestRouteNode(const WayRef& way, Id nodeId, RouteNodeRef& routeNode)
  {
    routeNode=NULL;

    size_t index=0;
    while (index<way->nodes.size()) {
      if (way->nodes[index].id==nodeId) {
        break;
      }

      index++;
    }

    if (index>=way->nodes.size()) {
      return;
    }

    for (size_t i=index; i<way->nodes.size(); i++) {
      routeNodeDataFile.Get(way->nodes[i].GetId(),routeNode);

      if (routeNode.Valid()) {
        return;
      }
    }

    if (!way->IsOneway()) {
      for (int i=index-1; i>=0; i--) {
        routeNodeDataFile.Get(way->nodes[i].GetId(),routeNode);

        if (routeNode.Valid()) {
          return;
        }
      }
    }
  }

  bool Router::CalculateRoute(const RoutingProfile& profile,
                              Id startWayId, Id startNodeId,
                              Id targetWayId, Id targetNodeId,
                              RouteData& route)
  {
    WayRef                startWay;
    WayRef                currentWay;
    double                startLon=0.0L,startLat=0.0L;
    WayRef                targetWay;
    double                targetLon=0.0L,targetLat=0.0L;

    RouteNodeRef          startRouteNode;
    RouteNodeRef          targetRouteNode;

    // Sorted list (smallest cost first) of ways to check (we are using a std::set)
    OpenList              openList;
    // Map routing nodes by id
    std::map<Id,RNodeRef> openMap;
    std::map<Id,RNode>    closeMap;

    size_t                nodesVisitedCount=0;
    size_t                turnsUsedCount=0;
    size_t                turnsDroppedCount=0;

    std::cout << startWayId << "[" << startNodeId << "] => " << targetWayId << "[" << targetNodeId << "]" << std::endl;

    std::cout << "=========== Routing start =============" << std::endl;

    StopClock clock;

    route.Clear();

    if (!wayDataFile.Get(startWayId,
                         startWay)) {
      std::cerr << "Cannot get start way!" << std::endl;
      return false;
    }

    if (!wayDataFile.Get(targetWayId,
                         targetWay)) {
      std::cerr << "Cannot get end way!" << std::endl;
      return false;
    }

    size_t index=0;
    while (index<startWay->nodes.size()) {
      if (startWay->nodes[index].id==startNodeId) {
        startLon=startWay->nodes[index].lon;
        startLat=startWay->nodes[index].lat;
        break;
      }

      index++;
    }

    if (index>=startWay->nodes.size()) {
      std::cerr << "Given start node is not part of start way" << std::endl;
      return false;
    }

    GetClosestRouteNode(startWay,startNodeId,startRouteNode);

    if (!startRouteNode.Valid()) {
      std::cerr << "No route node found for start way" << std::endl;
    }

    index=0;
    while (index<targetWay->nodes.size()) {
      if (targetWay->nodes[index].id==targetNodeId) {
        targetLon=targetWay->nodes[index].lon;
        targetLat=targetWay->nodes[index].lat;
        break;
      }

      index++;
    }

    if (index>=targetWay->nodes.size()) {
      std::cerr << "Given target node is not part of target way" << std::endl;
      return false;
    }

    GetClosestRouteNode(targetWay,targetNodeId,targetRouteNode);

    if (!targetRouteNode.Valid()) {
      std::cerr << "No route node found for target way" << std::endl;
    }

    // Start node, we do not have any cost up to now
    // The estimated costs for the rest of the way are cost of the the spherical distance (shortest
    // way) using the fasted way type available.
    RNode node=RNode(startRouteNode->id,
                     startWay->GetId(),
                     0);

    node.currentCost=GetSphericalDistance(startLon,
                      startLat,
                      startLon,
                      startLat);
    node.estimateCost=profile.GetMinCostFactor()*
                      GetSphericalDistance(startLon,
                                           startLat,
                                           targetLon,
                                           targetLat);
    node.overallCost=node.currentCost+node.estimateCost;

    openList.insert(node);
    openMap[node.nodeId]=openList.begin();

    currentWay=NULL;

    do {
      //
      // Take entry from open list with lowest cost
      //

      RNode        current=*openList.begin();
      RouteNodeRef currentRouteNode;

      if (!routeNodeDataFile.Get(current.nodeId,currentRouteNode) || !currentRouteNode.Valid()) {
        std::cerr << "Cannot load route node with id " << current.nodeId << std::endl;
        turnsDroppedCount++;
        return false;
      }

      //std::cout << "Visiting route node " << currentRouteNode->id << " " << currentRouteNode->lat << "," << currentRouteNode->lon << std::endl;

      nodesVisitedCount++;
/*
      std::cout << "S:   " << openList.size() << std::endl;
      std::cout << "ID:  " << current.id << std::endl;
      std::cout << "REF: " << current.ref.id << std::endl;
      std::cout << "PRV: " << current.prev << std::endl;
      std::cout << "CC:  " << current.currentCost << std::endl;
      std::cout << "EC:  " << current.estimateCost << std::endl;
      std::cout << "OC:  " << current.overallCost << std::endl;
      std::cout << "DST: " << GetSphericalDistance(current.lon,current.lat,targetLon,targetLat) << std::endl;*/

      openList.erase(openList.begin());
      openMap.erase(current.nodeId);

      // Get potential follower in the current way

      for (size_t i=0; i<currentRouteNode->paths.size(); i++) {
        if (!profile.CanUse(currentRouteNode->paths[i].type)) {
          //std::cout << "skipping route node " << currentRouteNode->paths[i].id << " (wrong type)" << std::endl;
          turnsDroppedCount++;
          continue;
        }

        if (!currentRouteNode->excludes.empty()) {
          bool canTurnedInto=true;
          for (size_t e=0; e<currentRouteNode->excludes.size(); e++) {
            if (currentRouteNode->excludes[e].sourceWay==current.wayId &&
                currentRouteNode->excludes[e].targetPath==i) {
              //std::cout << "Cannot drive from " << current.wayId << " into " << currentRouteNode->paths[i].wayId << std::endl;
              canTurnedInto=false;
              break;
            }
          }

          if (!canTurnedInto) {
            turnsDroppedCount++;
            continue;
          }
        }

        std::map<Id,RNode>::iterator closeEntry=closeMap.find(currentRouteNode->paths[i].id);

        if (closeEntry!=closeMap.end()) {
          //std::cout << "skipping route node " << currentRouteNode->paths[i].id << " (closed)" << std::endl;
          continue;
        }

        double currentCost=current.currentCost+
                           profile.GetCostFactor(currentRouteNode->paths[i].type)*
                           currentRouteNode->paths[i].distance;

        // TODO: Turn costs

        std::map<Id,RNodeRef>::iterator openEntry=openMap.find(currentRouteNode->paths[i].id);

        // Check, if we already have a cheaper path to the new node.If yes, do not put the new path
        // into the open list
        if (openEntry!=openMap.end() &&
            openEntry->second->currentCost<=currentCost) {
          //std::cout << "skipping route node " << currentRouteNode->paths[i].id << " (cheaper route exists " << currentCost << "<=>" << openEntry->second->currentCost << ")" << std::endl;
          continue;
        }

        turnsUsedCount++;

        // Estimate costs for the rest of the distance to the target
        double estimateCost=profile.GetMinCostFactor()*
                            GetSphericalDistance(currentRouteNode->paths[i].lon,currentRouteNode->paths[i].lat,
                                                 targetLon,targetLat);
        double overallCost=currentCost+estimateCost;

        RNode node(currentRouteNode->paths[i].id,
                   currentRouteNode->paths[i].wayId,
                   currentRouteNode->id);

        node.currentCost=currentCost;
        node.estimateCost=estimateCost;
        node.overallCost=overallCost;

        // If we already have the node in the open list, but the new path is cheaper,
        // update the existing entry
        if (openEntry!=openMap.end()) {
          //std::cout << "  updating route node " << node.id << " " << std::setprecision (std::numeric_limits<double>::digits10 + 1) << currentRouteNode->paths[i].distance << std::endl;

          openList.erase(openEntry->second);

          std::pair<RNodeRef,bool> result=openList.insert(node);

          openEntry->second=result.first;
        }
        else {
          //std::cout << "  inserting route node " << node.id << " " << std::setprecision (std::numeric_limits<double>::digits10 + 1) << currentRouteNode->paths[i].distance << std::endl;

          std::pair<RNodeRef,bool> result=openList.insert(node);
          openMap[node.nodeId]=result.first;
        }
      }

      //
      // Added current node to close map
      //

      closeMap[current.nodeId]=current;

      //
      // Check if finished
      //

      if (current.nodeId==targetRouteNode->id) {
        std::list<RouteStep> steps;

        while (current.prev!=0) {
          RouteStep step;

          step.wayId=current.wayId;
          step.nodeId=current.nodeId;

          steps.push_back(step);

          std::map<Id,RNode>::const_iterator previous=closeMap.find(current.prev);

          assert(previous!=closeMap.end());

          current=previous->second;
        }

        route.AddEntry(startWayId,startNodeId);
        for (std::list<RouteStep>::reverse_iterator step=steps.rbegin();
             step!=steps.rend();
             ++step) {
          route.AddEntry(step->wayId,step->nodeId);

          /*
          std::cout << "node " << step->nodeId << "( way " << step->wayId << ")";

          Way way;

          GetWay(step->wayId,way);

          for (size_t i=0; i<way.tags.size(); i++) {
            if (way.tags[i].key==tagName) {
              std::cout << " " << way.tags[i].value;
            }
            else if (way.tags[i].key==tagRef) {
              std::cout << " " << way.tags[i].value;
            }
          }

          std::cout << std::endl;*/
        }

        clock.Stop();

        std::cout << "Time:                " << clock << std::endl;
        std::cout << "Route nodes visited: " << nodesVisitedCount << std::endl;
        std::cout << "Turns evaluated:     " << turnsUsedCount << std::endl;
        std::cout << "Turns dropped:       " << turnsDroppedCount << std::endl;

        std::cout << "=========== Routing end ==============" << std::endl;
        return true;
      }
    } while (!openList.empty());

    std::cout << "No route found!" << std::endl;
    std::cout << "=========== Routing end ==============" << std::endl;

    return false;
  }

  bool Router::TransformRouteDataToRouteDescription(const RouteData& data,
                                                    RouteDescription& description)
  {
    if (!IsOpen()) {
      return false;
    }

    WayRef                                           way,newWay;
    Id                                               node=0,newNode=0;
    std::list<RouteData::RouteEntry>::const_iterator iter;
    double                                           distance=0.0;
    double                                           lastDistance=0.0;

    description.Clear();

    if (data.Entries().empty()) {
      return true;
    }

    iter=data.Entries().begin();

    if (!wayDataFile.Get(iter->GetWayId(),way)) {
      return false;
    }

    // Find the starting node
    for (size_t i=0; i<way->nodes.size(); i++) {
      if (way->nodes[i].id==iter->GetNodeId()) {
        node=i;
        break;
      }
    }

    // Lets start at the starting node (suprise, suprise ;-))
    description.AddStep(0.0,0.0,RouteDescription::start,way->GetName(),way->GetRefName());
    description.AddStep(0.0,0.0,RouteDescription::drive,way->GetName(),way->GetRefName());

    iter++;

    // For every step in the route...
    for ( /* no code */ ;iter!=data.Entries().end(); ++iter, way=newWay, node=newNode) {
      // Find the corresponding way (which may be the old way?)
      if (iter->GetWayId()!=way->GetId()) {
        if (!wayDataFile.Get(iter->GetWayId(),newWay)) {
          return false;
        }
      }
      else {
        newWay=way;
      }

      // Find the current node in the new way and calculate the distance
      // between the old point and the new point
      for (size_t i=0; i<newWay->nodes.size(); i++) {
        if (newWay->nodes[i].id==iter->GetNodeId()) {
          distance+=GetEllipsoidalDistance(way->nodes[node].lon,way->nodes[node].lat,
                                           newWay->nodes[i].lon,newWay->nodes[i].lat);
          newNode=i;
        }
      }

      // We skip steps where street does not have any names
      if (newWay->GetName().empty() &&
          newWay->GetRefName().empty()) {
        continue;
      }

      // We didn't change street name, so we do not create a new entry...
      if (!way->GetName().empty() &&
          way->GetName()==newWay->GetName()) {
        continue;
      }

      // We didn't change ref name, so we do not create a new entry...
      if (!way->GetRefName().empty()
          && way->GetRefName()==newWay->GetRefName()) {
        continue;
      }

      description.AddStep(distance,distance-lastDistance,RouteDescription::switchRoad,newWay->GetName(),newWay->GetRefName());
      description.AddStep(distance,distance-lastDistance,RouteDescription::drive,newWay->GetName(),newWay->GetRefName());
      lastDistance=distance;
    }

    // We reached the destination!
    description.AddStep(distance,distance-lastDistance,RouteDescription::reachTarget,newWay->GetName(),newWay->GetRefName());

    return true;
  }

  bool Router::TransformRouteDataToWay(const RouteData& data,
                                         Way& way)
  {
    TypeId routeType;
    Way    tmp;

    routeType=typeConfig->GetWayTypeId("_route");

    assert(routeType!=typeIgnore);

    way=tmp;

    way.SetId(0);
    way.SetType(routeType);
    way.nodes.reserve(data.Entries().size());

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      WayRef w;

      if (!wayDataFile.Get(iter->GetWayId(),w)) {
        return false;
      }

      for (size_t i=0; i<w->nodes.size(); i++) {
        if (w->nodes[i].id==iter->GetNodeId()) {
          way.nodes.push_back(w->nodes[i]);
          break;
        }
      }
    }

    return true;
  }
  void Router::DumpStatistics()
  {
    wayDataFile.DumpStatistics();
  }
}

