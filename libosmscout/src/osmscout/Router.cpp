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

#include <osmscout/RoutingProfile.h>
#include <osmscout/TypeConfigLoader.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING

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

    if (!wayDataFile.Open(path,FileScanner::Normal,true,FileScanner::Normal,false)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!routeNodeDataFile.Open(path,FileScanner::Normal,true,FileScanner::Normal,true)) {
      std::cerr << "Cannot open 'route.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

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

  void Router::GetClosestForwardRouteNode(const WayRef& way,
                                          Id nodeId,
                                          RouteNodeRef& routeNode,
                                          size_t& pos)
  {
    routeNode=NULL;

    size_t index=0;
    while (index<way->nodes.size()) {
      if (way->nodes[index].GetId()==nodeId) {
        break;
      }

      index++;
    }

    for (size_t i=index; i<way->nodes.size(); i++) {
      routeNodeDataFile.Get(way->nodes[i].GetId(),
                            routeNode);

      if (routeNode.Valid()) {
        pos=i;
        return;
      }
    }
  }

  void Router::GetClosestBackwardRouteNode(const WayRef& way,
                                           Id nodeId,
                                           RouteNodeRef& routeNode,
                                           size_t& pos)
  {
    routeNode=NULL;

    size_t index=0;
    while (index<way->nodes.size()) {
      if (way->nodes[index].GetId()==nodeId) {
        break;
      }

      index++;
    }

    if (index>=way->nodes.size()) {
      return;
    }

    if (!way->IsOneway()) {
      for (int i=index-1; i>=0; i--) {
        routeNodeDataFile.Get(way->nodes[i].GetId(),routeNode);

        if (routeNode.Valid()) {
          pos=(size_t)i;
          return;
        }
      }
    }
  }

  bool Router::ResolveRNodesToList(const RNodeRef& end,
                                   const CloseMap& closeMap,
                                   std::list<RNodeRef>& nodes)
  {
    CloseMap::const_iterator current=closeMap.find(end->nodeOffset);

    while (current->second->prev!=0) {
      CloseMap::const_iterator prev=closeMap.find(current->second->prev);

      nodes.push_back(current->second);

      current=prev;
    }

    nodes.push_back(current->second);

    std::reverse(nodes.begin(),nodes.end());

    return true;
  }

  bool Router::AddNodes(RouteData& route,
                        const std::vector<Path>& startPaths,
                        Id startNodeId,
                        Id wayId,
                        Id targetNodeId)
  {
    WayRef way;

    // Load the way to get all nodes between current route node and the next route node
    if (!wayDataFile.Get(wayId,way)) {
      std::cerr << "Cannot load way " << wayId << std::endl;
      return false;
    }

    size_t start=0;
    while (start<way->nodes.size() &&
        way->nodes[start].GetId()!=startNodeId) {
      start++;
    }

    size_t end=0;
    while (end<way->nodes.size() &&
        way->nodes[end].GetId()!=targetNodeId) {
      end++;
    }

    if (start>=way->nodes.size() ||
        end>=way->nodes.size()) {
      return false;
    }

    if (std::max(start,end)-std::min(start,end)==1) {
      route.AddEntry(way->nodes[start].GetId(),
                     startPaths,
                     way->GetId(),
                     targetNodeId);
    }
    else if (start<end) {
      route.AddEntry(way->nodes[start].GetId(),
                     startPaths,
                     way->GetId(),
                     way->nodes[start+1].GetId());

      for (size_t i=start+1; i<end-1; i++) {
        route.AddEntry(way->nodes[i].GetId(),
                       way->GetId(),
                       way->nodes[i+1].GetId());
      }

      route.AddEntry(way->nodes[end-1].GetId(),
                     way->GetId(),
                     targetNodeId);
    }
    else if (way->IsOneway()) {
      size_t pos=start+1;
      size_t next;

      if (pos>=way->nodes.size()) {
        pos=0;
      }

      next=pos+1;
      if (next>=way->nodes.size()) {
        next=0;
      }

      route.AddEntry(startNodeId,
                     startPaths,
                     way->GetId(),
                     way->nodes[pos].GetId());

      while (way->nodes[next].GetId()!=way->nodes[end].GetId()) {
        route.AddEntry(way->nodes[pos].GetId(),
                       way->GetId(),
                       way->nodes[next].GetId());

        pos++;
        if (pos>=way->nodes.size()) {
          pos=0;
        }

        next=pos+1;
        if (next>=way->nodes.size()) {
          next=0;
        }
      }

      route.AddEntry(way->nodes[pos].GetId(),
                     way->GetId(),
                     targetNodeId);
    }
    else {
      route.AddEntry(way->nodes[start].GetId(),
                     startPaths,
                     way->GetId(),
                     way->nodes[start-1].GetId());

      for (int i=start-1; i>(int)end+1; i--) {
        route.AddEntry(way->nodes[i].GetId(),
                       way->GetId(),
                       way->nodes[i-1].GetId());
      }

      route.AddEntry(way->nodes[end+1].GetId(),
                     way->GetId(),
                     targetNodeId);
    }

    return true;
  }

 std::vector<Path> Router::TransformPaths(const RoutingProfile& profile,
                                          const RouteNode& node)
 {
    std::vector<osmscout::Path> result;

    for (size_t i=0; i<node.paths.size(); i++) {
      bool traversable=profile.CanUse(node,i);

      result.push_back(osmscout::Path(node.ways[node.paths[i].wayIndex],
                                      node.paths[i].id,
                                      traversable));
    }

    return result;
  }

  bool Router::ResolveRNodesToRouteData(const RoutingProfile& profile,
                                        const std::list<RNodeRef>& nodes,
                                        Id startWayId,
                                        Id startNodeId,
                                        Id targetWayId,
                                        Id targetNodeId,
                                        RouteData& route)
  {
    if (nodes.empty()) {
      AddNodes(route,
               std::vector<Path>(),
               startNodeId,
               startWayId,
               targetNodeId);

      route.AddEntry(targetNodeId,
                     0,
                     0);
      return true;
    }

    RouteNodeRef initialNode;

    // TODO: Optimize node=nextNode of the last step!
    if (!routeNodeDataFile.GetByOffset(nodes.front()->nodeOffset,initialNode)) {
      std::cerr << "Cannot load route node with offset " << nodes.front()->nodeOffset << std::endl;
      return false;
    }

    if (startNodeId!=initialNode->GetId()) {
      // Start node to initial route node
      AddNodes(route,
               std::vector<Path>(),
               startNodeId,
               startWayId,
               initialNode->GetId());
    }

    for (std::list<RNodeRef>::const_iterator n=nodes.begin();
        n!=nodes.end();
        n++) {
      std::list<RNodeRef>::const_iterator nn=n;

      nn++;

      RouteNodeRef node;

      // TODO: Optimize node=nextNode of the last step!
      if (!routeNodeDataFile.GetByOffset((*n)->nodeOffset,node)) {
        std::cerr << "Cannot load route node with offset " << (*n)->nodeOffset << std::endl;
        return false;
      }


      // We do not have any follower node, push the final entry (leading nowhere)
      // to the route
      if (nn==nodes.end()) {
        if (node->GetId()!=targetNodeId) {
          AddNodes(route,
                   TransformPaths(profile,node),
                   node->GetId(),
                   targetWayId,
                   targetNodeId);

          route.AddEntry(targetNodeId,
                         0,
                         0);
        }
        else {
          route.AddEntry(node->GetId(),
                         0,
                         0);
        }

        break;
      }

      RouteNodeRef nextNode;
      WayRef       way;
      size_t       pathIndex=0;

      if (!routeNodeDataFile.GetByOffset((*nn)->nodeOffset,nextNode)) {
        std::cerr << "Cannot load route node with offst " << (*nn)->nodeOffset << std::endl;
        return false;
      }

      // Find the path we need to go to reach the next route node
      for (size_t i=0; i<node->paths.size(); i++) {
        if (node->ways[node->paths[i].wayIndex]==(*nn)->wayId) {
          pathIndex=i;
          break;
        }
      }

      AddNodes(route,
               TransformPaths(profile,node),
               node->id,
               node->ways[node->paths[pathIndex].wayIndex],
               nextNode->id);
    }

    return true;
  }

  bool Router::CalculateRoute(const RoutingProfile& profile,
                              Id startWayId, Id startNodeId,
                              Id targetWayId, Id targetNodeId,
                              RouteData& route)
  {
    WayRef                   startWay;
    double                   startLon=0.0L,startLat=0.0L;

    RouteNodeRef             startForwardRouteNode;
    size_t                   startForwardNodePos;
    FileOffset               startForwardOffset;
    RouteNodeRef             startBackwardRouteNode;
    size_t                   startBackwardNodePos;
    FileOffset               startBackwardOffset;

    WayRef                   targetWay;
    double                   targetLon=0.0L,targetLat=0.0L;

    RouteNodeRef             targetForwardRouteNode;
    size_t                   targetForwardNodePos;
    FileOffset               targetForwardOffset;
    RouteNodeRef             targetBackwardRouteNode;
    size_t                   targetBackwardNodePos;
    FileOffset               targetBackwardOffset;

    WayRef                   currentWay;

    // Sorted list (smallest cost first) of ways to check (we are using a std::set)
    OpenList                 openList;
    // Map routing nodes by id
    OpenMap                  openMap;
    CloseMap                 closeMap;

    size_t                   nodesLoadedCount=0;
    size_t                   nodesIgnoredCount=0;
    size_t                   maxOpenList=0;
    size_t                   maxCloseMap=0;

    StopClock clock;

    route.Clear();

#if defined(OSMSCOUT_HASHMAP_HAS_RESERVE)
    openMap.reserve(10000);
    closeMap.reserve(250000);
#endif

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
      if (startWay->nodes[index].GetId()==startNodeId) {
        startLon=startWay->nodes[index].GetLon();
        startLat=startWay->nodes[index].GetLat();
        break;
      }

      index++;
    }

    if (index>=startWay->nodes.size()) {
      std::cerr << "Given start node is not part of start way" << std::endl;
      return false;
    }

    GetClosestForwardRouteNode(startWay,
                               startNodeId,
                               startForwardRouteNode,
                               startForwardNodePos);
    GetClosestBackwardRouteNode(startWay,
                                startNodeId,
                                startBackwardRouteNode,
                                startBackwardNodePos);

    if (startForwardRouteNode.Invalid() &&
        startBackwardRouteNode.Invalid()) {
      std::cerr << "No route node found for start way" << std::endl;
      return false;
    }

    index=0;
    while (index<targetWay->nodes.size()) {
      if (targetWay->nodes[index].GetId()==targetNodeId) {
        targetLon=targetWay->nodes[index].GetLon();
        targetLat=targetWay->nodes[index].GetLat();
        break;
      }

      index++;
    }

    if (index>=targetWay->nodes.size()) {
      std::cerr << "Given target node is not part of target way" << std::endl;
      return false;
    }

    GetClosestForwardRouteNode(targetWay,
                               targetNodeId,
                               targetForwardRouteNode,
                               targetForwardNodePos);
    GetClosestBackwardRouteNode(targetWay,
                                targetNodeId,
                                targetBackwardRouteNode,
                                targetBackwardNodePos);

    if (targetForwardRouteNode.Invalid() &&
        targetBackwardRouteNode.Invalid()) {
      std::cerr << "No route node found for target way" << std::endl;
      return false;
    }

    if (targetForwardRouteNode.Valid()) {
      if (!routeNodeDataFile.GetOffset(targetForwardRouteNode->id,
                                       targetForwardOffset)) {
        std::cerr << "Cannot get offset of targetForwardRouteNode" << std::endl;
      }
    }

    if (targetBackwardRouteNode.Valid()) {
      if (!routeNodeDataFile.GetOffset(targetBackwardRouteNode->id,
                                       targetBackwardOffset)) {
        std::cerr << "Cannot get offset of targetBackwardRouteNode" << std::endl;
      }
    }

    if (startForwardRouteNode.Valid()) {
      if (!routeNodeDataFile.GetOffset(startForwardRouteNode->id,
                                       startForwardOffset)) {
        std::cerr << "Cannot get offset of startForwardRouteNode" << std::endl;
      }

      RNodeRef node=new RNode(startForwardOffset,
                              startWay->GetId());

      node->currentCost=profile.GetCosts(startWay,
                                         GetSphericalDistance(startLon,
                                                              startLat,
                                                              startWay->nodes[startForwardNodePos].GetLon(),
                                                              startWay->nodes[startForwardNodePos].GetLat()));
      node->estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                               startLat,
                                                               targetLon,
                                                               targetLat));

      node->overallCost=node->currentCost+node->estimateCost;

      std::pair<OpenListRef,bool> result=openList.insert(node);
      openMap[node->nodeOffset]=result.first;
    }

    if (startBackwardRouteNode.Valid()) {
      if (!routeNodeDataFile.GetOffset(startBackwardRouteNode->id,
                                       startBackwardOffset)) {
        std::cerr << "Cannot get offset of startBackwardRouteNode" << std::endl;
      }

      RNodeRef node=new RNode(startBackwardOffset,
                              startWay->GetId());

      node->currentCost=profile.GetCosts(startWay,
                                         GetSphericalDistance(startLon,
                                                              startLat,
                                                              startWay->nodes[startBackwardNodePos].GetLon(),
                                                              startWay->nodes[startBackwardNodePos].GetLat()));
      node->estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                               startLat,
                                                               targetLon,
                                                               targetLat));

      node->overallCost=node->currentCost+node->estimateCost;

      std::pair<OpenListRef,bool> result=openList.insert(node);
      openMap[node->nodeOffset]=result.first;
    }

    currentWay=NULL;

    RNodeRef     current;
    RouteNodeRef currentRouteNode;

    do {
      //
      // Take entry from open list with lowest cost
      //

      current=*openList.begin();

      openMap.erase(current->nodeOffset);
      openList.erase(openList.begin());

      if (!routeNodeDataFile.GetByOffset(current->nodeOffset,
                                         currentRouteNode)) {
        std::cerr << "Cannot load route node with id " << current->nodeOffset << std::endl;
        return false;
      }

      nodesLoadedCount++;

      // Get potential follower in the current way

#if defined(DEBUG_ROUTING)
      std::cout << "Analysing follower of node " << currentRouteNode->GetId() << " " << current->currentCost << " " << current->estimateCost << " " << current->overallCost << std::endl;
#endif
      size_t i=0;
      for (std::vector<osmscout::RouteNode::Path>::const_iterator path=currentRouteNode->paths.begin();
           path!=currentRouteNode->paths.end();
           ++path,
           ++i) {
        if (path->offset==current->prev) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route from " << currentRouteNode->id << " to " << path->id << " (back the the last node visited)" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!profile.CanUse(*currentRouteNode,i)) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route from " << currentRouteNode->id << " to " << path->id << " (wrong type " << typeConfig->GetTypeInfo(path->type).GetName()  << ")" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!current->access &&
            path->HasAccess()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route from " << currentRouteNode->id << " to " << path->id << " (moving from non-accessible way back to accessible way)" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!currentRouteNode->excludes.empty()) {
          bool canTurnedInto=true;
          for (size_t e=0; e<currentRouteNode->excludes.size(); e++) {
            if (currentRouteNode->excludes[e].sourceWay==current->wayId &&
                currentRouteNode->excludes[e].targetPath==i) {
#if defined(DEBUG_ROUTING)
              WayRef sourceWay;
              WayRef targetWay;

              wayDataFile.Get(current->wayId,sourceWay);
              wayDataFile.Get(currentRouteNode->ways[path->wayIndex],targetWay);

              std::cout << "  Node " <<  currentRouteNode->id << ": ";
              std::cout << "Cannot turn from " << current->wayId << " " << sourceWay->GetName() << " (" << sourceWay->GetRefName()  << ")";
              std::cout << " into ";
              std::cout << currentRouteNode->ways[currentRouteNode->paths[i].wayIndex] << " " << targetWay->GetName() << " (" << targetWay->GetRefName()  << ")" << std::endl;
#endif
              canTurnedInto=false;
              break;
            }
          }

          if (!canTurnedInto) {
            nodesIgnoredCount++;
            continue;
          }
        }

        if (closeMap.find(path->offset)!=closeMap.end()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route node " << path->id << "/" << currentRouteNode->paths[i].offset << " (closed)" << std::endl;
#endif
          continue;
        }

        double currentCost=current->currentCost+
                           profile.GetCosts(*currentRouteNode,i);

        // TODO: Turn costs

        OpenMap::iterator openEntry=openMap.find(path->offset);

        // Check, if we already have a cheaper path to the new node. If yes, do not put the new path
        // into the open list
        if (openEntry!=openMap.end() &&
            (*openEntry->second)->currentCost<=currentCost) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route node " << currentRouteNode->paths[i].id << "/" << currentRouteNode->paths[i].offset << " (cheaper route exists " << currentCost << "<=>" << (*openEntry->second)->currentCost << ")" << std::endl;
#endif
          continue;
        }

        double distanceToTarget=GetSphericalDistance(path->lon,
                                                     path->lat,
                                                     targetLon,
                                                     targetLat);
        // Estimate costs for the rest of the distance to the target
        double estimateCost=profile.GetCosts(distanceToTarget);
        double overallCost=currentCost+estimateCost;

        // If we already have the node in the open list, but the new path is cheaper,
        // update the existing entry
        if (openEntry!=openMap.end()) {
          RNodeRef node=*openEntry->second;

          node->prev=current->nodeOffset;
          node->wayId=currentRouteNode->ways[path->wayIndex];

          node->currentCost=currentCost;
          node->estimateCost=estimateCost;
          node->overallCost=overallCost;
          node->access=currentRouteNode->paths[i].HasAccess();

#if defined(DEBUG_ROUTING)
          std::cout << "  Updating route " << current->nodeOffset << " via way " << node->wayId << " " << currentCost << " " << estimateCost << " " << overallCost << " " << currentRouteNode->id << std::endl;
#endif

          openList.erase(openEntry->second);

          std::pair<OpenListRef,bool> result=openList.insert(node);
          openEntry->second=result.first;
        }
        else {
          RNodeRef node=new RNode(path->offset,
                                  currentRouteNode->ways[path->wayIndex],
                                  current->nodeOffset);

          node->currentCost=currentCost;
          node->estimateCost=estimateCost;
          node->overallCost=overallCost;
          node->access=path->HasAccess();

#if defined(DEBUG_ROUTING)
          std::cout << "  Inserting route " << current->nodeOffset <<  " via way " << node->wayId  << " " << currentCost << " " << estimateCost << " " << overallCost << " " << currentRouteNode->id << std::endl;
#endif

          std::pair<OpenListRef,bool> result=openList.insert(node);
          openMap[node->nodeOffset]=result.first;
        }
      }

      //
      // Added current node to close map
      //

      closeMap[current->nodeOffset]=current;

      maxOpenList=std::max(maxOpenList,openMap.size());
      maxCloseMap=std::max(maxCloseMap,closeMap.size());

    } while (!openList.empty() &&
             (targetForwardRouteNode.Invalid() || current->nodeOffset!=targetForwardOffset) &&
             (targetBackwardRouteNode.Invalid() || current->nodeOffset!=targetBackwardOffset));

    clock.Stop();

    std::cout << "From:                " << startWayId << "[" << startNodeId << "]" << std::endl;
    std::cout << "To:                  " << targetWayId << "[" << targetNodeId << "]" << std::endl;
    std::cout << "Time:                " << clock << std::endl;
#if defined(DEBUG_ROUTING)
    std::cout << "Cost:                " << current->currentCost << " " << current->estimateCost << " " << current->overallCost << std::endl;
#endif
    std::cout << "Route nodes loaded:  " << nodesLoadedCount << std::endl;
    std::cout << "Route nodes ignored: " << nodesIgnoredCount << std::endl;
    std::cout << "Max. OpenList size:  " << maxOpenList << std::endl;
    std::cout << "Max. CloseMap size:  " << maxCloseMap << std::endl;

    if (!((targetForwardRouteNode.Invalid() || currentRouteNode->GetId()==targetForwardRouteNode->id) ||
          (targetBackwardRouteNode.Invalid() || currentRouteNode->GetId()==targetBackwardRouteNode->id))) {
      std::cout << "No route found!" << std::endl;
      return false;
    }

    std::list<RNodeRef> nodes;

    if (!ResolveRNodesToList(current,
                             closeMap,
                             nodes)) {
      std::cerr << "Cannot resolve route nodes from routed path" << std::endl;
      return false;
    }

    if (!ResolveRNodesToRouteData(profile,
                                  nodes,
                                  startWayId,
                                  startNodeId,
                                  targetWayId,
                                  targetNodeId,
                                  route)) {
      //std::cerr << "Cannot convert routing result to route data" << std::endl;
      return false;
    }

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
      if (iter->GetPathWayId()!=0) {
        WayRef w;

        if (!wayDataFile.Get(iter->GetPathWayId(),w)) {
          return false;
        }

        // Initial starting point
        if (iter==data.Entries().begin()) {
          for (size_t i=0; i<w->nodes.size(); i++) {
            if (w->nodes[i].GetId()==iter->GetCurrentNodeId()) {
              way.nodes.push_back(w->nodes[i]);
              break;
            }
          }
        }

        for (size_t i=0; i<w->nodes.size(); i++) {
          if (w->nodes[i].GetId()==iter->GetTargetNodeId()) {
            way.nodes.push_back(w->nodes[i]);
            break;
          }
        }
      }
    }

    return true;
  }

  bool Router::TransformRouteDataToPoints(const RouteData& data,
                                          std::list<Point>& points)
  {
    WayRef w;

    points.clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      if (iter->GetPathWayId()!=0) {

        if (w.Invalid() ||
            w->GetId()!=iter->GetPathWayId()) {
          if (!wayDataFile.Get(iter->GetPathWayId(),w)) {
            std::cerr << "Cannot load way with id " << iter->GetPathWayId() << std::endl;
            return false;
          }
        }

        // Initial starting point
        if (iter==data.Entries().begin()) {
          for (size_t i=0; i<w->nodes.size(); i++) {
            if (w->nodes[i].GetId()==iter->GetCurrentNodeId()) {
              points.push_back(Point(w->nodes[i].GetId(),
                                     w->nodes[i].GetLat(),
                                     w->nodes[i].GetLon()));
              break;
            }
          }
        }

        // target node of current path
        for (size_t i=0; i<w->nodes.size(); i++) {
          if (w->nodes[i].GetId()==iter->GetTargetNodeId()) {
            Point point;

            point.Set(w->nodes[i].GetId(),
                      w->nodes[i].GetLat(),
                      w->nodes[i].GetLon());

            points.push_back(point);
            break;
          }
        }
      }
    }

    return true;
  }

  bool Router::TransformRouteDataToRouteDescription(const RouteData& data,
                                                    RouteDescription& description)
  {
    description.Clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      description.AddNode(iter->GetCurrentNodeId(),
                          iter->GetPaths(),
                          iter->GetPathWayId(),
                          iter->GetTargetNodeId());
    }

    return true;
  }

  bool PostprocessRouteDescription(RouteDescription& description,
                                   const std::string& startDescription,
                                   const std::string& targetDescription)
  {
    return true;
  }

  void Router::DumpStatistics()
  {
    wayDataFile.DumpStatistics();
    routeNodeDataFile.DumpStatistics();
  }
}

