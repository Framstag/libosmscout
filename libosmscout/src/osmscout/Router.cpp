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

//#define DEBUG_ROUTING

namespace osmscout {

  RoutePostprocessor::Postprocessor::~Postprocessor()
  {
    // no code
  }

  RoutePostprocessor::StartPostprocessor::StartPostprocessor(const std::string& startDescription)
  : startDescription(startDescription)
  {
    // no code
  }

  bool RoutePostprocessor::StartPostprocessor::Process(const RoutingProfile& profile,
                                                       RouteDescription& description,
                                                       Database& database)
  {
    description.Nodes().front().AddDescription(RouteDescription::NODE_START_DESC,
                                               new RouteDescription::StartDescription(startDescription));

    return true;
  }

  RoutePostprocessor::TargetPostprocessor::TargetPostprocessor(const std::string& targetDescription)
  : targetDescription(targetDescription)
  {
    // no code
  }

  bool RoutePostprocessor::TargetPostprocessor::Process(const RoutingProfile& profile,
                                                        RouteDescription& description,
                                                        Database& database)
  {
    description.Nodes().back().AddDescription(RouteDescription::NODE_TARGET_DESC,
                                              new RouteDescription::TargetDescription(targetDescription));

    return true;
  }

  bool RoutePostprocessor::DistanceAndTimePostprocessor::Process(const RoutingProfile& profile,
                                                                 RouteDescription& description,
                                                                 Database& database)
  {
    WayRef prevWay,nextWay;
    Id     prevNode,nextNode;
    double distance=0.0;
    double time=0.0;

    for (std::list<RouteDescription::Node>::iterator iter=description.Nodes().begin();
         iter!=description.Nodes().end();
         ++iter) {
      // The last node does not have a pathWayId set, since we are not going anywhere!
      if (iter->GetPathWayId()!=0) {
        // Only load the next way, if it is different from the old one
        if (prevWay.Invalid() || prevWay->GetId()!=iter->GetPathWayId()) {
          if (!database.GetWay(iter->GetPathWayId(),nextWay)) {
            std::cout << "Error while loading way " << iter->GetPathWayId() << std::endl;
            return false;
          }
        }
        else {
          nextWay=prevWay;
        }

        for (size_t i=0; i<nextWay->nodes.size(); i++) {
          if (nextWay->nodes[i].id==iter->GetTargetNodeId()) {
            nextNode=i;
            break;
          }
        }

        if (prevWay.Valid()) {
          double deltaDistance=GetEllipsoidalDistance(prevWay->nodes[prevNode].lon,
                                                      prevWay->nodes[prevNode].lat,
                                                      nextWay->nodes[nextNode].lon,
                                                      nextWay->nodes[nextNode].lat);
          double deltaTime=profile.GetTime(nextWay,
                                           deltaDistance);

          distance+=deltaDistance;
          time+=deltaTime;
        }
      }

      iter->SetDistance(distance);
      iter->SetTime(time);

      prevWay=nextWay;
      prevNode=nextNode;
    }

    return true;
  }

  bool RoutePostprocessor::WayNamePostprocessor::Process(const RoutingProfile& profile,
                                                         RouteDescription& description,
                                                         Database& database)
  {
    RouteDescription::NameDescriptionRef lastDescription;
    WayRef                               lastWay;
    WayRef                               nextWay;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      // The last node does not have a pathWayId set, since we are not going anywhere from the target node!
      if (node->GetPathWayId()==0) {
        break;
      }

      RouteDescription::NameDescriptionRef nextDescription;

      // Only load the next way, if it is different from the old one
      if (lastWay.Invalid() || lastWay->GetId()!=node->GetPathWayId()) {
        if (!database.GetWay(node->GetPathWayId(),nextWay)) {
          std::cerr << "Cannot retrieve way " << node->GetPathWayId() << std::endl;
          return false;
        }

        nextDescription=new RouteDescription::NameDescription(nextWay->GetName(),
                                                              nextWay->GetRefName());
        lastDescription=nextDescription;
      }
      else {
        nextWay=lastWay;
        nextDescription=lastDescription;
      }

      node->AddDescription(RouteDescription::WAY_NAME_DESC,
                           nextDescription);
    }

    return true;
  }

  bool RoutePostprocessor::WayNameChangedPostprocessor::Process(const RoutingProfile& profile,
                                                                RouteDescription& description,
                                                                Database& database)
  {
    WayRef lastInterestingWay;
    WayRef lastWay;
    WayRef nextWay;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      // The last node does not have a pathWayId set, since we are not going anywhere from the target node!
      if (node->GetPathWayId()==0) {
        break;
      }

      RouteDescription::NameDescriptionRef originDescription;
      RouteDescription::NameDescriptionRef targetDescription;

      if (lastWay.Invalid() || lastWay->GetId()!=node->GetPathWayId()) {
        if (!database.GetWay(node->GetPathWayId(),nextWay)) {
          std::cerr << "Cannot retrieve way " << node->GetPathWayId() << std::endl;
          return false;
        }
      }
      else {
        nextWay=lastWay;
      }

      if (lastInterestingWay.Valid()) {
        // We didn't change street name and ref, so we do not create a new entry...
        if (lastInterestingWay->GetName()==nextWay->GetName() &&
            lastInterestingWay->GetRefName()==nextWay->GetRefName()) {
          continue;
        }

        // We skip steps where street does not have any names and silently
        // assume they still have the old name (currently this happens for
        // motorway links that no have a name.
        if (nextWay->GetName().empty() &&
            nextWay->GetRefName().empty()) {
          continue;
        }

        // If the ref name is still the same but the way name changes and the new way is a bridge
        // we assume that the name changed just because of the bridge and do not see this as
        // relevant name change.
        // TODO: Check if this is because of some import error
        if (lastInterestingWay->GetName().empty() &&
            !nextWay->GetName().empty() &&
            lastInterestingWay->GetRefName()==nextWay->GetRefName() &&
            nextWay->IsBridge()) {
          continue;
        }

        originDescription=new RouteDescription::NameDescription(lastInterestingWay->GetName(),
                                                                lastInterestingWay->GetRefName());
      }

      targetDescription=new RouteDescription::NameDescription(nextWay->GetName(),
                                                              nextWay->GetRefName());


      node->AddDescription(RouteDescription::WAY_NAME_CHANGED_DESC,
                           new RouteDescription::NameChangedDescription(originDescription,
                                                                        targetDescription));
      lastInterestingWay=nextWay;
    }

    return true;
  }

  bool RoutePostprocessor::CrossingWaysPostprocessor::Process(const RoutingProfile& profile,
                                                              RouteDescription& description,
                                                              Database& database)
  {
    std::set<Id>                                wayIds;
    std::vector<WayRef>                         ways;
    std::map<Id,WayRef>                         wayMap;
    std::list<RouteDescription::Node>::iterator lastNode;

    lastNode=description.Nodes().end();
    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      // The origin way
      if (lastNode!=description.Nodes().end()) {
        wayIds.insert(lastNode->GetPathWayId());
      }

      // The target way
      if (node->GetPathWayId()!=0) {
        wayIds.insert(node->GetPathWayId());
      }

      for (std::vector<Id>::const_iterator id=node->GetWays().begin();
          id!=node->GetWays().end();
          ++id) {
        wayIds.insert(*id);
      }

      lastNode=node;
    }

    if (!database.GetWays(wayIds,ways)) {
      std::cerr << "Cannot retrieve crossing ways" << std::endl;
      return false;
    }

    for (std::vector<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        ++w) {
      WayRef way=*w;

      wayMap[way->GetId()]=way;
    }

    lastNode=description.Nodes().end();
    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      if (!node->GetWays().empty()) {
        WayRef                            originWay;
        WayRef                            targetWay;
        RouteDescription::NameDescription *originDescription=NULL;
        RouteDescription::NameDescription *targetDescription=NULL;

        if (lastNode!=description.Nodes().end()) {
          std::map<Id,WayRef>::const_iterator way=wayMap.find(lastNode->GetPathWayId());

          if (way!=wayMap.end()) {
            originWay=way->second;
            originDescription=new RouteDescription::NameDescription(way->second->GetName(),
                                                                    way->second->GetRefName());
          }
        }

        if (node->GetPathWayId()!=0) {
          std::map<Id,WayRef>::const_iterator way=wayMap.find(node->GetPathWayId());

          if (way!=wayMap.end()) {
            targetWay=way->second;
            targetDescription=new RouteDescription::NameDescription(way->second->GetName(),
                                                                    way->second->GetRefName());
          }
        }

        RouteDescription::CrossingWaysDescriptionRef crossingDescription=new RouteDescription::CrossingWaysDescription(originDescription,
                                                                                                                       targetDescription);

        for (std::vector<Id>::const_iterator id=node->GetWays().begin();
            id!=node->GetWays().end();
            ++id) {
          std::map<Id,WayRef>::const_iterator way=wayMap.find(*id);

          if (way!=wayMap.end()) {
            // Way is origin way and starts or end here so it is not an additional crossing way
            if (originWay.Valid() &&
                way->second->GetId()==originWay->GetId() &&
                (way->second->nodes.front().GetId()==node->GetCurrentNodeId() ||
                 way->second->nodes.back().GetId()==node->GetCurrentNodeId())) {
              continue;
            }

            // Way is target way and starts or end here so it is not an additional crossing way
            if (targetWay.Valid() &&
                way->second->GetId()==targetWay->GetId() &&
                (way->second->nodes.front().GetId()==node->GetCurrentNodeId() ||
                 way->second->nodes.back().GetId()==node->GetCurrentNodeId())) {
              continue;
            }

            // ways is origin way and way is target way so it is not an additional crossing way
            if (originWay.Valid() &&
                targetWay.Valid() &&
                way->second->GetId()==originWay->GetId() &&
                way->second->GetId()==targetWay->GetId()) {
              continue;
            }

            crossingDescription->AddDescription(new RouteDescription::NameDescription(way->second->GetName(),
                                                                                      way->second->GetRefName()));
          }
        }

        node->AddDescription(RouteDescription::CROSSING_WAYS_DESC,
                             crossingDescription);
      }

      lastNode=node;
    }

    return true;
  }

  bool RoutePostprocessor::PostprocessRouteDescription(RouteDescription& description,
                                                       const RoutingProfile& profile,
                                                       Database& database,
                                                       std::list<PostprocessorRef> processors)
  {
    size_t pos=1;
    for (std::list<PostprocessorRef>::iterator p=processors.begin();
        p!=processors.end();
        ++p) {
      PostprocessorRef processor=*p;

      if (!processor->Process(profile,
                              description,
                              database)) {
        std::cerr << "Error during execution of postprocessor " << pos << std::endl;
        return false;
      }

      pos++;
    }

    return true;
  }

  RouteData::RouteEntry::RouteEntry(Id currentNodeId,
                                    Id pathWayId,
                                    Id targetNodeId)
   : currentNodeId(currentNodeId),
     pathWayId(pathWayId),
     targetNodeId(targetNodeId)
  {
    // no code
  }

  RouteData::RouteEntry::RouteEntry(Id currentNodeId,
                                    const std::vector<Id>& ways,
                                    Id pathWayId,
                                    Id targetNodeId)
   : currentNodeId(currentNodeId),
     ways(ways),
     pathWayId(pathWayId),
     targetNodeId(targetNodeId)
  {
    // no code
  }

  RouteData::RouteData()
  {
    // no code
  }

  void RouteData::Clear()
  {
    entries.clear();
  }

  void RouteData::AddEntry(Id currentNodeId,
                           Id pathWayId,
                           Id targetNodeId)
  {
    entries.push_back(RouteEntry(currentNodeId,
                                 pathWayId,
                                 targetNodeId));
  }

  void RouteData::AddEntry(Id currentNodeId,
                           const std::vector<Id>& ways,
                           Id pathWayId,
                           Id targetNodeId)
  {
    entries.push_back(RouteEntry(currentNodeId,
                                 ways,
                                 pathWayId,
                                 targetNodeId));
  }

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

  void Router::GetClosestRouteNode(const WayRef& way, Id nodeId, RouteNodeRef& routeNode, size_t& pos)
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
        pos=i;
        return;
      }
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

  bool Router::ResolveRNodesToList(const RNode& end,
                                   const std::map<Id,RNode>& closeMap,
                                   std::list<RNode>& nodes)
  {
    std::map<Id,RNode>::const_iterator current=closeMap.find(end.nodeId);
    RouteNodeRef                       routeNode;

    while (current->second.prev!=0) {
      std::map<Id,RNode>::const_iterator prev=closeMap.find(current->second.prev);

      nodes.push_back(current->second);

      current=prev;
    }

    nodes.push_back(current->second);

    std::reverse(nodes.begin(),nodes.end());

    return true;
  }

  bool Router::ResolveRNodesToRouteData(const std::list<RNode>& nodes,
                                        RouteData& route)
  {
    for (std::list<RNode>::const_iterator n=nodes.begin();
        n!=nodes.end();
        n++) {
      std::list<RNode>::const_iterator nn=n;

      nn++;

      // We do not have any follower node, push the final entry (leading nowhere)
      // to the route
      if (nn==nodes.end()) {
        route.AddEntry(n->nodeId,
                       0,
                       0);
        continue;
      }

      RouteNodeRef node;
      RouteNodeRef nextNode;
      WayRef       way;
      size_t       pathIndex=0;

      // TODO: Optimize node=nextNode of the last step!
      if (!routeNodeDataFile.Get(n->nodeId,node)) {
        return false;
      }

      if (!routeNodeDataFile.Get(nn->nodeId,nextNode)) {
        return false;
      }

      // Find the path with need to go to reach the next route node
      for (size_t i=0; i<node->paths.size(); i++) {
        if (node->ways[node->paths[i].wayIndex]==n->wayId) {
          pathIndex=i;
          break;
        }
      }

      // Load the way to get all nodes between current route node and the next route node
      if (!wayDataFile.Get(node->ways[node->paths[pathIndex].wayIndex],way)) {
        std::cerr << "Cannot load way " << node->ways[node->paths[pathIndex].wayIndex] << std::endl;
        return false;
      }

      size_t start=0;
      while (start<way->nodes.size() &&
          way->nodes[start].GetId()!=node->id) {
        start++;
      }

      size_t end=0;
      while (end<way->nodes.size() &&
          way->nodes[end].GetId()!=nextNode->id) {
        end++;
      }

      if (start>=way->nodes.size() ||
          end>=way->nodes.size()) {
        continue;
      }

      if (std::max(start,end)-std::min(start,end)==1) {
        route.AddEntry(way->nodes[start].GetId(),
                       node->ways,
                       way->GetId(),
                       nextNode->id);
      }
      else if (start<end) {
        route.AddEntry(way->nodes[start].GetId(),
                       node->ways,
                       way->GetId(),
                       way->nodes[start+1].GetId());

        for (size_t i=start+1; i<end-1; i++) {
          route.AddEntry(way->nodes[i].GetId(),
                         way->GetId(),
                         way->nodes[i+1].GetId());
        }

        route.AddEntry(way->nodes[end-1].GetId(),
                       way->GetId(),
                       nextNode->id);
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

        route.AddEntry(node->id,
                       node->ways,
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
                       nextNode->id);
      }
      else {
        route.AddEntry(way->nodes[start].GetId(),
                       node->ways,
                       way->GetId(),
                       way->nodes[start-1].GetId());

        for (int i=start-1; i>(int)end+1; i--) {
          route.AddEntry(way->nodes[i].GetId(),
                         way->GetId(),
                         way->nodes[i-1].GetId());
        }

        route.AddEntry(way->nodes[end+1].GetId(),
                       way->GetId(),
                       nextNode->id);
      }
    }

    return true;
  }

  bool Router::CalculateRoute(const RoutingProfile& profile,
                              Id startWayId, Id startNodeId,
                              Id targetWayId, Id targetNodeId,
                              RouteData& route)
  {
    WayRef                startWay;
    double                startLon=0.0L,startLat=0.0L;
    RouteNodeRef          startRouteNode;
    size_t                startNodePos;

    WayRef                targetWay;
    double                targetLon=0.0L,targetLat=0.0L;
    RouteNodeRef          targetRouteNode;
    size_t                targetNodePos;

    WayRef                currentWay;

    // Sorted list (smallest cost first) of ways to check (we are using a std::set)
    OpenList              openList;
    // Map routing nodes by id
    std::map<Id,RNodeRef> openMap;
    std::map<Id,RNode>    closeMap;

    size_t                nodesLoadedCount=0;
    size_t                nodesIgnoredCount=0;

    std::cout << startWayId << "[" << startNodeId << "] => " << targetWayId << "[" << targetNodeId << "]" << std::endl;

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

    GetClosestRouteNode(startWay,startNodeId,startRouteNode,startNodePos);

    if (!startRouteNode.Valid()) {
      std::cerr << "No route node found for start way" << std::endl;
      return false;
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

    GetClosestRouteNode(targetWay,targetNodeId,targetRouteNode,targetNodePos);

    if (!targetRouteNode.Valid()) {
      std::cerr << "No route node found for target way" << std::endl;
      return false;
    }

    // Start node, we do not have any cost up to now
    // The estimated costs for the rest of the way are cost of the the spherical distance (shortest
    // way) using the fasted way type available.
    RNode node=RNode(startRouteNode->id,
                     startWay->GetId());

    node.currentCost=profile.GetCosts(startWay,
                                      GetSphericalDistance(startLon,
                                                           startLat,
                                                           startWay->nodes[startNodePos].GetLon(),
                                                           startWay->nodes[startNodePos].GetLat()));
    node.estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                            startLat,
                                                            targetLon,
                                                            targetLat));

    node.overallCost=node.currentCost+node.estimateCost;

    openList.insert(node);
    openMap[node.nodeId]=openList.begin();

    currentWay=NULL;

    RNode        current;
    RouteNodeRef currentRouteNode;

    do {
      //
      // Take entry from open list with lowest cost
      //

      current=*openList.begin();

      if (!routeNodeDataFile.Get(current.nodeId,currentRouteNode) ||
          !currentRouteNode.Valid()) {
        std::cerr << "Cannot load route node with id " << current.nodeId << std::endl;
        nodesIgnoredCount++;
        return false;
      }

      nodesLoadedCount++;

      openList.erase(openList.begin());
      openMap.erase(current.nodeId);

      // Get potential follower in the current way

#if defined(DEBUG_ROUTING)
      std::cout << "Analysing follower of node " << currentRouteNode->GetId() << " " << current.currentCost << " " << current.estimateCost << " " << current.overallCost << std::endl;
#endif
      for (size_t i=0; i<currentRouteNode->paths.size(); i++) {
        if (!profile.CanUse(*currentRouteNode,i)) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route from " << currentRouteNode->id << " to " << currentRouteNode->paths[i].id << " (wrong type " << typeConfig->GetTypeInfo(currentRouteNode->paths[i].type).GetName()  << ")" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!current.access && currentRouteNode->paths[i].HasAccess()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route from " << currentRouteNode->id << " to " << currentRouteNode->paths[i].id << " (moving from non-accessible way back to accessible way)" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!currentRouteNode->excludes.empty()) {
          bool canTurnedInto=true;
          for (size_t e=0; e<currentRouteNode->excludes.size(); e++) {
            if (currentRouteNode->excludes[e].sourceWay==current.wayId &&
                currentRouteNode->excludes[e].targetPath==i) {
#if defined(DEBUG_ROUTING)
              WayRef sourceWay;
              WayRef targetWay;

              wayDataFile.Get(current.wayId,sourceWay);
              wayDataFile.Get(currentRouteNode->ways[currentRouteNode->paths[i].wayIndex],targetWay);

              std::cout << "  Node " <<  currentRouteNode->id << ": ";
              std::cout << "Cannot turn from " << current.wayId << " " << sourceWay->GetName() << " (" << sourceWay->GetRefName()  << ")";
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

        std::map<Id,RNode>::iterator closeEntry=closeMap.find(currentRouteNode->paths[i].id);

        if (closeEntry!=closeMap.end()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route node " << currentRouteNode->paths[i].id << " (closed)" << std::endl;
#endif
          continue;
        }

        double currentCost=current.currentCost+
                           profile.GetCosts(*currentRouteNode,i);

        // TODO: Turn costs

        std::map<Id,RNodeRef>::iterator openEntry=openMap.find(currentRouteNode->paths[i].id);

        // Check, if we already have a cheaper path to the new node. If yes, do not put the new path
        // into the open list
        if (openEntry!=openMap.end() &&
            openEntry->second->currentCost<=currentCost) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route node " << currentRouteNode->paths[i].id << " (cheaper route exists " << currentCost << "<=>" << openEntry->second->currentCost << ")" << std::endl;
#endif
          continue;
        }

        // Estimate costs for the rest of the distance to the target
        double estimateCost=profile.GetCosts(GetSphericalDistance(currentRouteNode->paths[i].lon,
                                                                  currentRouteNode->paths[i].lat,
                                                                  targetLon,
                                                                  targetLat));
        double overallCost=currentCost+estimateCost;

        RNode node(currentRouteNode->paths[i].id,
                   currentRouteNode->ways[currentRouteNode->paths[i].wayIndex],
                   currentRouteNode->id);

        node.currentCost=currentCost;
        node.estimateCost=estimateCost;
        node.overallCost=overallCost;
        node.access=currentRouteNode->paths[i].HasAccess();

        // If we already have the node in the open list, but the new path is cheaper,
        // update the existing entry
        if (openEntry!=openMap.end()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Updating route " << node.nodeId << " via way " << node.wayId << " " << currentCost << " " << estimateCost << " " << overallCost << std::endl;
#endif

          openList.erase(openEntry->second);

          std::pair<RNodeRef,bool> result=openList.insert(node);

          openEntry->second=result.first;
        }
        else {
#if defined(DEBUG_ROUTING)
          std::cout << "  Inserting route " << node.nodeId <<  " via way " << node.wayId  << " " << currentCost << " " << estimateCost << " " << overallCost << std::endl;
#endif

          std::pair<RNodeRef,bool> result=openList.insert(node);
          openMap[node.nodeId]=result.first;
        }
      }

      //
      // Added current node to close map
      //

      closeMap[current.nodeId]=current;
    } while (!openList.empty() && current.nodeId!=targetRouteNode->id);

    clock.Stop();

    std::cout << "Time:                " << clock << std::endl;
#if defined(DEBUG_ROUTING)
    std::cout << "Cost:                " << current.currentCost << " " << current.estimateCost << " " << current.overallCost << std::endl;
#endif
    std::cout << "Route nodes loaded:  " << nodesLoadedCount << std::endl;
    std::cout << "Route nodes ignored: " << nodesIgnoredCount << std::endl;

    if (current.nodeId!=targetRouteNode->id) {
      std::cout << "No route found!" << std::endl;
      return false;
    }

    std::list<RNode> nodes;

    if (!ResolveRNodesToList(current,
                             closeMap,
                             nodes)) {
      std::cerr << "Cannot resolve route nodes from routed path" << std::endl;
      return false;
    }

    if (!ResolveRNodesToRouteData(nodes,
                                  route)) {
      std::cerr << "Cannot convert routing result to route data" << std::endl;
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

        for (size_t i=0; i<w->nodes.size(); i++) {
          if (w->nodes[i].id==iter->GetTargetNodeId()) {
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
    points.clear();

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

        for (size_t i=0; i<w->nodes.size(); i++) {
          if (w->nodes[i].id==iter->GetTargetNodeId()) {
            Point point;

            point.SetId(w->nodes[i].GetId());
            point.SetCoordinates(w->nodes[i].GetLat(),
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
    TypeId routeType;
    Way    tmp;

    routeType=typeConfig->GetWayTypeId("_route");

    assert(routeType!=typeIgnore);

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      description.AddNode(iter->GetCurrentNodeId(),
                          iter->GetWays(),
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

