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

namespace osmscout {

  struct NodeUseCacheValueSizer : public Router::NodeUseCache::ValueSizer
  {
    unsigned long GetSize(const std::vector<Router::NodeUse>& value) const
    {
      unsigned long memory=0;

      for (size_t i=0; i<value.size(); i++) {
        memory+=sizeof(Router::NodeUse);
      }

      return memory;
    }
  };

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
     nodeUseCache(10),
     wayDataFile("ways.dat",
                 "way.idx",
                  parameter.GetWayCacheSize(),
                  parameter.GetWayIndexCacheSize()),
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

    std::cout << "Loading node use index..." << std::endl;
    if (!nodeUseIndex.LoadNodeUseIndex(path)) {
      std::cerr << "Cannot load node use index!" << std::endl;
      return false;
    }
    std::cout << "Loading node use index done." << std::endl;

    std::cout << "Opening 'ways.dat'..." << std::endl;
    if (!wayDataFile.Open(path,true,true)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }
    std::cout << "Opening 'ways.dat' done." << std::endl;

    isOpen=true;

    return true;
  }

  bool Router::IsOpen() const
  {
    return isOpen;
  }


  void Router::Close()
  {
    wayDataFile.Close();

    isOpen=false;
  }

  void Router::FlushCache()
  {
    wayDataFile.FlushCache();
  }

  bool Router::GetWay(const Id& id, WayRef& way) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::vector<Id>     ids;
    std::vector<WayRef> ways;

    ids.push_back(id);

    if (wayDataFile.Get(ids,ways)) {
      if (!ways.empty()) {
        way=*ways.begin();
        return true;
      }
    }

    return false;
  }

  bool Router::GetJoints(const std::set<Id>& ids,
                         std::set<Id>& wayIds) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::list<NodeUseIndexEntry> indexEntries;
    std::string                  file=path+"/"+"nodeuse.idx";

    wayIds.clear();

    nodeUseIndex.GetNodeIndexEntries(ids,indexEntries);

    if (indexEntries.empty()) {
      std::cout << "GetJoints(): Ids not found in index" << std::endl;
      return false;
    }

    if (!nodeUseScanner.IsOpen()) {
      if (!nodeUseScanner.Open(file)) {
        std::cerr << "Cannot open nodeuse.idx file!" << std::endl;
        return false;
      }
    }

    NodeUseCache::CacheRef cacheRef;

    for (std::list<NodeUseIndexEntry>::const_iterator indexEntry=indexEntries.begin();
         indexEntry!=indexEntries.end();
         ++indexEntry) {
      if (!nodeUseCache.GetEntry(indexEntry->interval,cacheRef)) {
        if (!nodeUseScanner.SetPos(indexEntry->offset)) {
          std::cerr << "Cannot read nodeuse.idx page from file!" << std::endl;
          nodeUseScanner.Close();
          return false;
        }

        NodeUseCache::CacheEntry cacheEntry(indexEntry->interval);
        cacheRef=nodeUseCache.SetEntry(cacheEntry);

        cacheRef->value.resize(indexEntry->count);

        for (size_t i=0; i<indexEntry->count; i++) {
          uint32_t count;

          nodeUseScanner.Read(cacheRef->value[i].id);
          nodeUseScanner.ReadNumber(count);

          if (nodeUseScanner.HasError()) {
            std::cerr << "Error while reading from nodeuse.idx file!" << std::endl;
            nodeUseScanner.Close();
            return false;
          }

          cacheRef->value[i].references.resize(count);

          for (size_t j=0; j<count; j++) {
            nodeUseScanner.Read(cacheRef->value[i].references[j]);
          }
        }
      }

      for (std::vector<NodeUse>::const_iterator w=cacheRef->value.begin();
           w!=cacheRef->value.end();
           ++w) {
        if (ids.find(w->id)!=ids.end()) {
          for (size_t i=0; i<w->references.size(); i++) {
            wayIds.insert(w->references[i]);
          }
        }
      }
    }

    return true;
  }

  bool Router::GetJoints(Id id,
                         std::set<Id>& wayIds) const
  {
    if (!IsOpen()) {
      return false;
    }

    std::set<Id> ids;

    ids.insert(id);

    return GetJoints(ids,wayIds);
  }

  bool Router::GetWays(std::map<Id,Way>& cache,
                       const std::set<Id>& ids,
                       std::vector<WayPtr>& refs)
  {
    bool result=true;

    refs.clear();
    refs.reserve(ids.size());

    std::vector<Id> remaining;

    for (std::set<Id>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {
      std::map<Id,Way>::const_iterator ref=cache.find(*id);

      if (ref!=cache.end()) {
        refs.push_back(&ref->second);
      }
      else {
        remaining.push_back(*id);
      }
    }

    if (!remaining.empty()) {
      std::vector<FileOffset> offsets;
      static FileScanner      wayScanner;
      std::string             file=path+"/"+"ways.dat";

      if (!wayScanner.IsOpen()) {
        if (!wayScanner.Open(file)){
          std::cerr << "Error while opening ways.dat for routing file!" << std::endl;

          return false;
        }
      }

      if (!wayDataFile.GetOffsets(remaining,offsets)) {
        return false;
      }

      for (std::vector<FileOffset>::const_iterator entry=offsets.begin();
           entry!=offsets.end();
           ++entry) {
        wayScanner.SetPos(*entry);

        Way way;

        way.Read(wayScanner);

        std::pair<std::map<Id,Way>::iterator,bool> result=cache.insert(std::pair<Id,Way>(way.GetId(),way));

        refs.push_back(&result.first->second);
      }

      result=!wayScanner.HasError()/* && wayScanner.Close()*/;
    }

    assert(ids.size()==refs.size());

    return result;
  }

  bool Router::GetWay(std::map<Id,Way>& cache,
                      Id id,
                      WayPtr& ref)
  {
    std::set<Id>        ids;
    std::vector<WayPtr> refs;

    ids.insert(id);

    if (!GetWays(cache,ids,refs)) {
      return false;
    }

    ref=refs[0];

    return true;
  }


  /**
   * A node in the routing graph, normally a node as part of a way
   */
  struct RNode
  {
    Id        id;
    double    lon;
    double    lat;
    double    currentCost;
    double    estimateCost;
    double    overallCost;
    ObjectRef ref;
    Id        prev;

    RNode()
     : id(0)
    {
    }

    RNode(Id id,
          double lon, double lat,
          const ObjectRef& reference,
          Id prev)
     : id(id),
       lon(lon),
       lat(lat),
       currentCost(0),
       estimateCost(0),
       overallCost(0),
       ref(reference),
       prev(prev)
    {
      // no code
    }

    bool operator==(const RNode& node)
    {
      return id==node.id;
    }

    bool operator<(const RNode& node) const
    {
      return id<node.id;
    }
  };

  struct RNodeCostCompare
  {
    bool operator()(const RNode& a, const RNode& b) const
    {
      return a.overallCost<b.overallCost;
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

  struct Follower
  {
    std::set<Id>  ways;
  };

  typedef std::set<RNode,RNodeCostCompare> OpenList;
  typedef OpenList::iterator               RNodeRef;

  struct RouteStep
  {
    Id wayId;
    Id nodeId;
  };

  bool Router::CalculateRoute(Id startWayId, Id startNodeId,
                              Id targetWayId, Id targetNodeId,
                              RouteData& route)
  {
    TypeId                type;
    std::map<Id,Way>      waysCache;
    std::map<Id,Follower> candidatesCache;
    std::vector<WayPtr>   followWays;
    WayPtr                startWay;
    WayPtr                currentWay;
    double                startLon=0.0L,startLat=0.0L;
    WayPtr                targetWay;
    double                targetLon=0.0L,targetLat=0.0L;
    // Sorted list (smallest cost first) of ways to check (we are using a std::set)
    OpenList              openList;
    // Map routing nodes by id
    std::map<Id,RNodeRef> openMap;
    std::map<Id,RNode>    closeMap;
    std::set<Id>          loaded;
    std::vector<size_t>   costs;
    RoutingProfile        profile;

    size_t                nodesVisitedCount=0;
    size_t                waysVisitedCount=0;
    size_t                waysDroppedCount=0;
    size_t                turnsUsedCount=0;
    size_t                turnsDroppedCount=0;

    std::cout << startWayId << "[" << startNodeId << "] => " << targetWayId << "[" << targetNodeId << "]" << std::endl;

    std::cout << "=========== Routing start =============" << std::endl;

    StopClock clock;

    route.Clear();

    profile.SetTurnCostFactor(1/60/2); // 30 seconds

    type=typeConfig->GetWayTypeId("highway_motorway");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/110.0);

    type=typeConfig->GetWayTypeId("highway_motorway_link");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/60.0);

    type=typeConfig->GetWayTypeId("highway_trunk");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/70.0);

    type=typeConfig->GetWayTypeId("highway_trunk_link");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/70.0);

    type=typeConfig->GetWayTypeId("highway_primary");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/70.0);

    type=typeConfig->GetWayTypeId("highway_primary_link");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/60.0);

    type=typeConfig->GetWayTypeId("highway_secondary");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/60.0);

    type=typeConfig->GetWayTypeId("highway_secondary_link");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/50.0);

    type=typeConfig->GetWayTypeId("highway_tertiary");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/55.0);

    type=typeConfig->GetWayTypeId("highway_unclassified");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/50.0);

    type=typeConfig->GetWayTypeId("highway_road");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/50.0);

    type=typeConfig->GetWayTypeId("highway_residential");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/40.0);

    type=typeConfig->GetWayTypeId("highway_living_street");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/10.0);

    type=typeConfig->GetWayTypeId("highway_service");
    assert(type!=typeIgnore);
    profile.SetTypeCostFactor(type,1/30.0);

    if (!GetWay(waysCache,
                startWayId,
                startWay)) {
      std::cerr << "Cannot get start way!" << std::endl;
      return false;
    }

    if (!GetWay(waysCache,
                targetWayId,
                targetWay)) {
      std::cerr << "Cannot get end way!" << std::endl;
      return false;
    }

    std::cout << "Searching for node " << startNodeId << " in way " << startWayId << "..." << std::endl;

    size_t index=0;
    while (index<startWay->nodes.size()) {
      if (startWay->nodes[index].id==startNodeId) {
        startLon=startWay->nodes[index].lon;
        startLat=startWay->nodes[index].lat;
        break;
      }

      index++;
    }

    assert(index<startWay->nodes.size());

    std::cout << "OK" << std::endl;

    std::cout << "Searching for node " << targetNodeId << " in way " << targetWayId << "..." << std::endl;

    index=0;
    while (index<targetWay->nodes.size()) {
      if (targetWay->nodes[index].id==targetNodeId) {
        targetLon=targetWay->nodes[index].lon;
        targetLat=targetWay->nodes[index].lat;
        break;
      }

      index++;
    }

    assert(index<targetWay->nodes.size());

    std::cout << "OK" << std::endl;

    // Start way is a way we cannot use for routing
    if (!profile.CanUse(startWay->GetType())) {
      std::cout << "Start way of type " << typeConfig->GetTypeInfo(startWay->GetType()).GetName() <<  " is not in the routing profile and thus cannot be routing start!" << std::endl;
      return false;
    }

    // Target way is a way we cannot use for routing
    if (!profile.CanUse(targetWay->GetType())) {
      std::cout << "Start way of type " << typeConfig->GetTypeInfo(targetWay->GetType()).GetName() <<  " is not in the routing profile and thus cannot be routing start!" << std::endl;
      return false;
    }

    // Start node, we do not have any cost up to now
    // The estimated costs for the rest of the way are cost of the the spherical distance (shortest
    // way) using the fasted way type available.
    RNode node=RNode(startNodeId,
                     startLon,
                     startLat,
                     ObjectRef(startWayId,refWay),
                     0);

    node.currentCost=0.0;
    node.estimateCost=profile.GetMinCostFactor()*
                      GetSphericalDistance(startLon,
                                           startLat,
                                           targetLon,
                                           targetLat);
    node.overallCost=node.currentCost+node.estimateCost;

    openList.insert(node);
    openMap[openList.begin()->id]=openList.begin();

    // array of follower nodes in the routing graph
    std::vector<RNode> follower;

    follower.reserve(1000);

    // As long as the way does not change we can cache the list of followers for the current way
    bool cachedFollower=false;

    currentWay=NULL;

    do {
      //
      // Take entry from open list with lowest cost
      //

      RNode current=*openList.begin();

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
      openMap.erase(current.id);

      //
      // Load current way, if not already loaded/cached
      //

      follower.clear();

      // Get joint nodes in same way/area, if the current way changes
      if (currentWay==NULL || currentWay->GetId()!=current.ref.id) {
        if (!GetWay(waysCache,
                    current.ref.id,
                    currentWay)) {
          return false;
        }

        waysVisitedCount++;
        cachedFollower=false;
      }

      //
      // Calculate follower of current node
      //

      // Get potential follower in the current way

      // If the current way is an area, then all nodes in the area are potential followers if not already
      // visited
      if (currentWay->IsArea()) {
        for (size_t i=0; i<currentWay->nodes.size(); ++i) {
          if (currentWay->nodes[i].id!=current.id) {

            std::map<Id,RNode>::iterator closeEntry=closeMap.find(currentWay->nodes[i].id);

            if (closeEntry!=closeMap.end()) {
              continue;
            }

            follower.push_back(RNode(currentWay->nodes[i].id,
                                     currentWay->nodes[i].lon,
                                     currentWay->nodes[i].lat,
                                     ObjectRef(currentWay->GetId(),refWay),
                                     current.id));
          }
        }
      }
      // For ways the node before and after the current node aree potential followers, if not already
      // visited. For Oneways only the next node in the way is candidate.
      else {
        for (size_t i=0; i<currentWay->nodes.size(); ++i) {
          if (currentWay->nodes[i].id==current.id) {
            if (i>0 && !currentWay->IsOneway()) {
              std::map<Id,RNode>::iterator closeEntry=closeMap.find(currentWay->nodes[i-1].id);

              if (closeEntry==closeMap.end()) {
                follower.push_back(RNode(currentWay->nodes[i-1].id,
                                         currentWay->nodes[i-1].lon,
                                         currentWay->nodes[i-1].lat,
                                         ObjectRef(currentWay->GetId(),refWay),
                                         current.id));
              }
            }

            if (i<currentWay->nodes.size()-1) {
              std::map<Id,RNode>::iterator closeEntry=closeMap.find(currentWay->nodes[i+1].id);

              if (closeEntry==closeMap.end()) {
                follower.push_back(RNode(currentWay->nodes[i+1].id,
                                         currentWay->nodes[i+1].lon,
                                         currentWay->nodes[i+1].lat,
                                         ObjectRef(currentWay->GetId(),refWay),
                                         current.id));
              }
            }

            break;
          }
        }
      }

      // Get joint ways and areas

      if (!cachedFollower) {
        std::map<Id,Follower>::const_iterator cacheEntry=candidatesCache.find(current.ref.id);

        if (cacheEntry==candidatesCache.end()) {
          std::pair<std::map<Id,Follower >::iterator,bool> result;

          result=candidatesCache.insert(std::pair<Id,Follower>(current.ref.id,Follower()));

          if (!GetJoints(current.ref.id,
                         result.first->second.ways)) {
            return false;
          }

          cacheEntry=result.first;
        }

        if (!GetWays(waysCache,
                     cacheEntry->second.ways,
                     followWays)) {
          return false;
        }

        cachedFollower=true;
      }

      // Get joint nodes in joint way/area, for ways we need to be able to turn into the new way

      for (std::vector<WayPtr>::const_iterator iter=followWays.begin();
           iter!=followWays.end();
           ++iter) {
        const Way* way=*iter;

        // joint way/area is of non-routable type
        if (!profile.CanUse(way->GetType())) {
          //std::cout << typeConfig->GetTypeInfo(way->GetType()).GetName() << std::endl;
          waysDroppedCount++;
          continue;
        }

        if (way->IsArea()) {
          for (size_t i=0; i<way->nodes.size(); i++) {
            if (way->nodes[i].id!=current.id) {
              std::map<Id,RNode>::iterator closeEntry=closeMap.find(way->nodes[i].id);

              if (closeEntry!=closeMap.end()) {
                continue;
              }

              follower.push_back(RNode(way->nodes[i].id,
                                       way->nodes[i].lon,
                                       way->nodes[i].lat,
                                       ObjectRef(way->GetId(),refWay),
                                       current.id));
            }
          }
        }
        else {
          for (size_t i=0; i<way->nodes.size(); ++i) {
            if (way->nodes[i].id==current.id) {
              if (CanBeTurnedInto(*currentWay,way->nodes[i].id,way->GetId())) {
                turnsUsedCount++;

                if (i>0 && !way->IsOneway()) {
                  std::map<Id,RNode>::iterator closeEntry=closeMap.find(way->nodes[i-1].id);

                  if (closeEntry==closeMap.end()) {
                    follower.push_back(RNode(way->nodes[i-1].id,
                                             way->nodes[i-1].lon,
                                             way->nodes[i-1].lat,
                                             ObjectRef(way->GetId(),refWay),
                                             current.id));
                  }
                }

                if (i<way->nodes.size()-1) {
                  std::map<Id,RNode>::iterator closeEntry=closeMap.find(way->nodes[i+1].id);

                  if (closeEntry==closeMap.end()) {
                    follower.push_back(RNode(way->nodes[i+1].id,
                                             way->nodes[i+1].lon,
                                             way->nodes[i+1].lat,
                                             ObjectRef(way->GetId(),refWay),
                                             current.id));
                  }
                }
              }
              else {
                turnsDroppedCount++;
              }

              break;
            }
          }
        }
      }

      //
      // Calculate costs of follower and put them into the open list
      //
      for (std::vector<RNode>::iterator iter=follower.begin();
           iter!=follower.end();
           ++iter) {
        // Calculate cost of moving from the current node to the new node using the current way type
        double currentCost=current.currentCost+
                           profile.GetCostFactor(currentWay->GetType())*
                           GetSphericalDistance(current.lon,
                                                current.lat,
                                                iter->lon,
                                                iter->lat);

        if (currentWay->GetId()!=iter->id) {
          currentCost+=profile.GetTurnCostFactor();
        }

        std::map<Id,RNodeRef>::iterator openEntry=openMap.find(iter->id);

        // Check, if we already have a cheaper path to the new node.If yes, do not put the new path
        // into the open list
        if (openEntry!=openMap.end() &&
            openEntry->second->currentCost<=currentCost) {
          continue;
        }

        // Estimate costs for the rest of the distance to the target
        double estimateCost=profile.GetMinCostFactor()*
                            GetSphericalDistance(iter->lon,iter->lat,targetLon,targetLat);
        double overallCost=currentCost+estimateCost;

        // If we already have the node in the open list, put the new path is cheaper,
        // update the existing entry
        if (openEntry!=openMap.end()) {
          iter->prev=current.id;
          iter->currentCost=currentCost;
          iter->estimateCost=estimateCost;
          iter->overallCost=overallCost;

          openList.erase(openEntry->second);
          std::pair<RNodeRef,bool> result=openList.insert(*iter);

          openEntry->second=result.first;
        }
        else {
          iter->currentCost=currentCost;
          iter->estimateCost=estimateCost;
          iter->overallCost=overallCost;

          std::pair<RNodeRef,bool> result=openList.insert(*iter);
          openMap[iter->id]=result.first;
        }
      }

      //
      // Added current node to close map
      //

      closeMap[current.id]=current;

      //
      // Check if finished
      //

      if (current.id==targetNodeId) {
        std::list<RouteStep> steps;

        while (current.prev!=0) {
          RouteStep step;

          step.wayId=current.ref.id;
          step.nodeId=current.id;

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
        std::cout << "Ways visited:        " << waysVisitedCount << std::endl;
        std::cout << "Ways dropped:        " << waysDroppedCount << std::endl;
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

    if (!GetWay(iter->GetWayId(),way)) {
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
        if (!GetWay(iter->GetWayId(),newWay)) {
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

      if (!GetWay(iter->GetWayId(),w)) {
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

