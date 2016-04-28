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

#include <osmscout/RoutingService.h>

#include <algorithm>

#include <osmscout/RoutingProfile.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING

namespace osmscout {

  RouterParameter::RouterParameter()
  : debugPerformance(false)
  {
    // no code
  }

  void RouterParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  bool RouterParameter::IsDebugPerformance() const
  {
    return debugPerformance;
  }

  const char* const RoutingService::FILENAME_INTERSECTIONS_DAT   = "intersections.dat";
  const char* const RoutingService::FILENAME_INTERSECTIONS_IDX   = "intersections.idx";

  const char* const RoutingService::DEFAULT_FILENAME_BASE        = "router";

  /**
   * Create a new instance of the routing service.
   *
   * @param database
   *    A valid reference to a database instance
   * @param parameter
   *    An instance to the parameter object holding further paramterization
   * @param vehicle
   *    The vehicle to route for (this results in loading a
   *    routing network for the given vehicle).
   */
  RoutingService::RoutingService(const DatabaseRef& database,
                                 const RouterParameter& parameter,
                                 const std::string& filenamebase)
   : database(database),
     filenamebase(filenamebase),
     accessReader(*database->GetTypeConfig()),
     isOpen(false),
     debugPerformance(parameter.IsDebugPerformance()),
     routeNodeDataFile(GetDataFilename(filenamebase),
                       GetIndexFilename(filenamebase),
                       12000),
     junctionDataFile(RoutingService::FILENAME_INTERSECTIONS_DAT,
                      RoutingService::FILENAME_INTERSECTIONS_IDX,
                      6000)
  {
    assert(database);
  }

  RoutingService::~RoutingService()
  {
    if (isOpen) {
      Close();
    }
  }

  std::string RoutingService::GetDataFilename(const std::string& filenamebase) const
  {
    return filenamebase+".dat";
  }

  std::string RoutingService::GetData2Filename(const std::string& filenamebase) const
  {
    return filenamebase+"2.dat";
  }

  std::string RoutingService::GetIndexFilename(const std::string& filenamebase) const
  {
    return filenamebase+".idx";
  }

  bool RoutingService::HasNodeWithId(const std::vector<Point>& nodes) const
  {
    for (const auto node : nodes) {
      if (node.IsRelevant()) {
        return true;
      }
    }

    return false;
  }

  bool RoutingService::LoadObjectVariantData(const std::string& filename,
                                             std::vector<ObjectVariantData>& objectVariantData) const
  {
    FileScanner scanner;

    try {
      scanner.Open(filename,
                   FileScanner::FastRandom,true);

      uint32_t objectVariantDataCount;

      scanner.Read(objectVariantDataCount);

      objectVariantData.resize(objectVariantDataCount);

      for (size_t i=0; i<objectVariantDataCount; i++) {
        objectVariantData[i].Read(*database->GetTypeConfig(),
                                  scanner);
      }

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }

  /**
   * Opens the routing service. This loads the routing graph for the given vehicle
   *
   * @return
   *    false on error, else true
   */
  bool RoutingService::Open()
  {
    path=database->GetPath();

    assert(!path.empty());

    StopClock timer;

    if (!routeNodeDataFile.Open(database->GetTypeConfig(),
                                path,
                                true,
                                true)) {
      log.Error() << "Cannot open '" <<  path << "'!";
      return false;
    }

    timer.Stop();

    log.Debug() << "Opening RouteNodeData: " << timer.ResultString();

    if (!LoadObjectVariantData(AppendFileToDir(path,
                                               GetData2Filename(filenamebase)),
                               objectVariantData)) {
      return false;
    }

    isOpen=true;

    return true;
  }

  /**
   * Returns true, if the routing service has been successfully opened, else false.
   *
   * @return
   *    True, if the routing service has been successfully opened
   */
  bool RoutingService::IsOpen() const
  {
    return isOpen;
  }

  /**
   * Close the routing service
   */
  void RoutingService::Close()
  {
    routeNodeDataFile.Close();

    isOpen=false;
  }

  /**
   * Returns the type configuration of the underlying database instance
   *
   * @return
   *    A valid type configuration or null if the database is not valid
   */
  TypeConfigRef RoutingService::GetTypeConfig() const
  {
    return database->GetTypeConfig();
  }

  void RoutingService::GetStartForwardRouteNode(const RoutingProfile& profile,
                                                const WayRef& way,
                                                size_t nodeIndex,
                                                RouteNodeRef& routeNode,
                                                size_t& routeNodeIndex)
  {
    routeNode=NULL;

    if (!profile.CanUseForward(*way)) {
      return;
    }

    // TODO: What if the way is a roundabout?

    for (size_t i=nodeIndex; i<way->nodes.size(); i++) {
      routeNodeDataFile.Get(way->GetId(i),
                            routeNode);

      if (routeNode) {
        routeNodeIndex=i;
        return;
      }
    }
  }

  void RoutingService::GetStartBackwardRouteNode(const RoutingProfile& profile,
                                                 const WayRef& way,
                                                 size_t nodeIndex,
                                                 RouteNodeRef& routeNode,
                                                 size_t& routeNodeIndex)
  {
    routeNode=NULL;

    if (nodeIndex>=way->nodes.size()) {
      return;
    }

    if (!profile.CanUseBackward(*way)) {
      return;
    }

    for (long i=nodeIndex-1; i>=0; i--) {
      routeNodeDataFile.Get(way->GetId(i),
                            routeNode);

      if (routeNode) {
        routeNodeIndex=(size_t)i;
        return;
      }
    }
  }

  /**
   * Return the route node that allows navigating to the given node
   * in forward direction. In result the returned routing node
   * will have a smaller index then the given node index.
   */
  void RoutingService::GetTargetForwardRouteNode(const RoutingProfile& profile,
                                                 const WayRef& way,
                                                 size_t nodeIndex,
                                                 RouteNodeRef& routeNode)
  {
    routeNode=NULL;

    if (nodeIndex>=way->nodes.size()) {
      return;
    }

    if (!profile.CanUseForward(*way)) {
      return;
    }

    for (long i=nodeIndex-1; i>=0; i--) {
      routeNodeDataFile.Get(way->GetId(i),
                            routeNode);

      if (routeNode) {
        return;
      }
    }
  }

  /**
   * Return the route node that allows navigating to the given node
   * in backward direction. In result the returned routing node
   * will have a bigger or equal index then the given node index.
   */
  void RoutingService::GetTargetBackwardRouteNode(const RoutingProfile& profile,
                                                  const WayRef& way,
                                                  size_t nodeIndex,
                                                  RouteNodeRef& routeNode)
  {
    routeNode=NULL;

    if (!profile.CanUseBackward(*way)) {
      return;
    }

    // TODO: What if the way is a roundabout?

    for (size_t i=nodeIndex; i<way->nodes.size(); i++) {
      routeNodeDataFile.Get(way->GetId(i),
                            routeNode);

      if (routeNode) {
        return;
      }
    }
  }

  void RoutingService::ResolveRNodeChainToList(const RNodeRef& end,
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
  }

  void RoutingService::AddNodes(RouteData& route,
                                Id startNodeId,
                                size_t startNodeIndex,
                                const ObjectFileRef& object,
                                size_t idCount,
                                bool oneway,
                                size_t targetNodeIndex)
  {
    assert(startNodeIndex<idCount);
    assert(targetNodeIndex<idCount);

    if (std::max(startNodeIndex,targetNodeIndex)-std::min(startNodeIndex,targetNodeIndex)==1) {
      // From one node to the neighbour node (+1 or -1)
      route.AddEntry(startNodeId,
                     startNodeIndex,
                     object,
                     targetNodeIndex);
    }
    else if (startNodeIndex<targetNodeIndex) {
      // Following the way
      route.AddEntry(startNodeId,
                     startNodeIndex,
                     object,
                     startNodeIndex+1);

      for (size_t i=startNodeIndex+1; i<targetNodeIndex-1; i++) {
        route.AddEntry(0,
                       i,
                       object,
                       i+1);
      }

      route.AddEntry(0,
                     targetNodeIndex-1,
                     object,
                     targetNodeIndex);
    }
    else if (oneway) {
      // startNodeIndex>tragetNodeIndex, but this is a oneway we assume
      // that the way is either an area or is a roundabound
      // TODO: proof this using the right assertions or checks
      size_t pos=startNodeIndex+1;
      size_t next;

      if (pos>=idCount) {
        pos=0;
      }

      next=pos+1;
      if (next>=idCount) {
        next=0;
      }

      route.AddEntry(startNodeId,
                     startNodeIndex,
                     object,
                     pos);

      while (next!=targetNodeIndex) {
        route.AddEntry(0,
                       pos,
                       object,
                       next);

        pos++;
        if (pos>=idCount) {
          pos=0;
        }

        next=pos+1;
        if (next>=idCount) {
          next=0;
        }
      }

      route.AddEntry(0,
                     pos,
                     object,
                     targetNodeIndex);
    }
    else {
      // We follow the way in the opposite direction
      route.AddEntry(startNodeId,
                     startNodeIndex,
                     object,
                     startNodeIndex-1);

      for (long i=startNodeIndex-1; i>(long)targetNodeIndex+1; i--) {
        route.AddEntry(0,
                       i,
                       object,
                       (size_t)i-1);
      }

      route.AddEntry(0,
                     targetNodeIndex+1,
                     object,
                     targetNodeIndex);
    }
  }

  bool RoutingService::ResolveRNodesToRouteData(const RoutingProfile& profile,
                                                const std::list<RNodeRef>& nodes,
                                                const ObjectFileRef& startObject,
                                                size_t startNodeIndex,
                                                const ObjectFileRef& targetObject,
                                                size_t targetNodeIndex,
                                                RouteData& route)
  {
    AreaDataFileRef                             areaDataFile(database->GetAreaDataFile());
    WayDataFileRef                              wayDataFile(database->GetWayDataFile());

    std::set<FileOffset>                        routeNodeOffsets;
    std::set<FileOffset>                        wayOffsets;
    std::set<FileOffset>                        areaOffsets;

    std::unordered_map<FileOffset,RouteNodeRef> routeNodeMap;
    std::unordered_map<FileOffset,AreaRef>      areaMap;
    std::unordered_map<FileOffset,WayRef>       wayMap;

    std::vector<Point>                          *ids=NULL;
    bool                                        oneway=false;

    if (!areaDataFile ||
        !wayDataFile) {
      return false;
    }

    // Collect all route node file offsets on the path and also
    // all area/way file offsets on the path
    for (const auto& node : nodes) {
      routeNodeOffsets.insert(node->nodeOffset);

      if (node->object.Valid()) {
        switch (node->object.GetType()) {
        case refArea:
          areaOffsets.insert(node->object.GetFileOffset());
          break;
        case refWay:
          wayOffsets.insert(node->object.GetFileOffset());
          break;
        default:
          assert(false);
          break;
        }
      }
    }

    // Collect area/way file offset for the
    // target object (not traversed by above loop)
    switch (targetObject.GetType()) {
    case refArea:
      areaOffsets.insert(targetObject.GetFileOffset());
      break;
    case refWay:
      wayOffsets.insert(targetObject.GetFileOffset());
      break;
    default:
      assert(false);
      break;
    }

    //
    // Load data
    //

    if (!routeNodeDataFile.GetByOffset(routeNodeOffsets,
                                       routeNodeMap)) {
      log.Error() << "Cannot load route nodes";
      return false;
    }

    if (!areaDataFile->GetByOffset(areaOffsets,
                                   areaMap)) {
      log.Error() << "Cannot load areas";
      return false;
    }

    if (!wayDataFile->GetByOffset(wayOffsets,
                                  wayMap)) {
      log.Error() << "Cannot load ways";
      return false;
    }

    assert(startObject.GetType()==refArea ||
           startObject.GetType()==refWay);

    if (startObject.GetType()==refArea) {
      std::unordered_map<FileOffset,AreaRef>::const_iterator entry=areaMap.find(startObject.GetFileOffset());

      assert(entry!=areaMap.end());

      ids=&entry->second->rings.front().nodes;
      oneway=false;
    }
    else if (startObject.GetType()==refWay) {
      std::unordered_map<FileOffset,WayRef>::const_iterator entry=wayMap.find(startObject.GetFileOffset());

      assert(entry!=wayMap.end());

      ids=&entry->second->nodes;
      oneway=!profile.CanUseBackward(*entry->second);
    }

    if (nodes.empty()) {
      // We assume that startNode and targetNode are on the same area/way (with no routing node in between)
      assert(startObject==targetObject);

      AddNodes(route,
               (*ids)[startNodeIndex].GetId(),
               startNodeIndex,
               startObject,
               ids->size(),
               oneway,
               targetNodeIndex);

      route.AddEntry(0,
                     targetNodeIndex,
                     ObjectFileRef(),
                     0);
      return true;
    }

    RouteNodeRef initialNode=routeNodeMap.find(nodes.front()->nodeOffset)->second;

    //
    // Add The path from the start node to the first routing node
    //
    if ((*ids)[startNodeIndex].GetId()!=initialNode->GetId()) {
      size_t routeNodeIndex=0;

      while ((*ids)[routeNodeIndex].GetId()!=initialNode->GetId() &&
          routeNodeIndex<ids->size()) {
        routeNodeIndex++;
      }
      assert(routeNodeIndex<ids->size());

      // Start node to initial route node
      AddNodes(route,
               (*ids)[startNodeIndex].GetId(),
               startNodeIndex,
               startObject,
               ids->size(),
               oneway,
               routeNodeIndex);
    }

    //
    // Walk the routing path from route node to the next route node
    // and build entries.
    //
    for (std::list<RNodeRef>::const_iterator n=nodes.begin();
        n!=nodes.end();
        n++) {
      std::list<RNodeRef>::const_iterator nn=n;

      nn++;

      RouteNodeRef node=routeNodeMap.find((*n)->nodeOffset)->second;

      //
      // The path from the last routing node to the target node and the
      // target node itself
      //
      if (nn==nodes.end()) {
        assert(targetObject.GetType()==refArea ||
               targetObject.GetType()==refWay);

        if (targetObject.GetType()==refArea) {
          std::unordered_map<FileOffset,AreaRef>::const_iterator entry=areaMap.find(targetObject.GetFileOffset());

          assert(entry!=areaMap.end());

          ids=&entry->second->rings.front().nodes;
          oneway=false;
        }
        else if (targetObject.GetType()==refWay) {
          std::unordered_map<FileOffset,WayRef>::const_iterator entry=wayMap.find(targetObject.GetFileOffset());

          assert(entry!=wayMap.end());

          ids=&entry->second->nodes;
          oneway=!profile.CanUseBackward(*entry->second);
        }

        size_t currentNodeIndex=0;

        while ((*ids)[currentNodeIndex].GetId()!=node->GetId() &&
            currentNodeIndex<ids->size()) {
          currentNodeIndex++;
        }
        assert(currentNodeIndex<ids->size());

        if (currentNodeIndex!=targetNodeIndex) {
          AddNodes(route,
                   (*ids)[currentNodeIndex].GetId(),
                   currentNodeIndex,
                   targetObject,
                   ids->size(),
                   oneway,
                   targetNodeIndex);
        }

        route.AddEntry(0,
                       targetNodeIndex,
                       ObjectFileRef(),
                       0);

        break;
      }

      RouteNodeRef nextNode=routeNodeMap.find((*nn)->nodeOffset)->second;

      assert((*nn)->object.GetType()==refArea ||
             (*nn)->object.GetType()==refWay);

      if ((*nn)->object.GetType()==refArea) {
        std::unordered_map<FileOffset,AreaRef>::const_iterator entry=areaMap.find((*nn)->object.GetFileOffset());

        assert(entry!=areaMap.end());

        ids=&entry->second->rings.front().nodes;
        oneway=false;
      }
      else if ((*nn)->object.GetType()==refWay) {
        std::unordered_map<FileOffset,WayRef>::const_iterator entry=wayMap.find((*nn)->object.GetFileOffset());

        assert(entry!=wayMap.end());

        ids=&entry->second->nodes;
        oneway=!profile.CanUseBackward(*entry->second);
      }

      size_t currentNodeIndex=0;

      while ((*ids)[currentNodeIndex].GetId()!=node->GetId() &&
          currentNodeIndex<ids->size()) {
        currentNodeIndex++;
      }
      assert(currentNodeIndex<ids->size());

      size_t nextNodeIndex=0;

      while ((*ids)[nextNodeIndex].GetId()!=nextNode->GetId() &&
          nextNodeIndex<ids->size()) {
        nextNodeIndex++;
      }
      assert(nextNodeIndex<ids->size());

      AddNodes(route,
               (*ids)[currentNodeIndex].GetId(),
               currentNodeIndex,
               (*nn)->object,
               ids->size(),
               oneway,
               nextNodeIndex);
    }

    return true;
  }

  bool RoutingService::ResolveRouteDataJunctions(RouteData& route)
  {
    std::set<Id> nodeIds;

    for (std::list<RouteData::RouteEntry>::const_iterator routeEntry=route.Entries().begin();
         routeEntry!=route.Entries().end();
         ++routeEntry) {
      if (routeEntry->GetCurrentNodeId()!=0) {
        nodeIds.insert(routeEntry->GetCurrentNodeId());
      }
    }

    if (!junctionDataFile.IsOpen()) {
      StopClock timer;

      if (!junctionDataFile.Open(database->GetTypeConfig(),
                                 path,
                                 false,
                                 false)) {
        return false;
      }

      timer.Stop();

      log.Debug() << "Opening JunctionDataFile: " << timer.ResultString();
    }

    std::vector<JunctionRef> junctions;

    if (!junctionDataFile.Get(nodeIds,
                              junctions)) {
      log.Error() << "Error while resolving junction ids to junctions";
    }

    nodeIds.clear();

    std::unordered_map<Id,JunctionRef> junctionMap;

    for (std::vector<JunctionRef>::const_iterator j=junctions.begin();
        j!=junctions.end();
        ++j) {
      JunctionRef junction(*j);

      junctionMap.insert(std::make_pair(junction->GetId(),junction));
    }

    junctions.clear();

    for (std::list<RouteData::RouteEntry>::iterator routeEntry=route.Entries().begin();
         routeEntry!=route.Entries().end();
         ++routeEntry) {
      if (routeEntry->GetCurrentNodeId()!=0) {
        std::unordered_map<Id,JunctionRef>::const_iterator junction=junctionMap.find(routeEntry->GetCurrentNodeId());
        if (junction!=junctionMap.end()) {
          routeEntry->SetObjects(junction->second->GetObjects());
        }
      }
    }

    return junctionDataFile.Close();
  }

  bool RoutingService::GetStartNodes(const RoutingProfile& profile,
                                     const ObjectFileRef& object,
                                     size_t nodeIndex,
                                     double& targetLon,
                                     double& targetLat,
                                     RouteNodeRef& forwardRouteNode,
                                     RouteNodeRef& backwardRouteNode,
                                     RNodeRef& forwardRNode,
                                     RNodeRef& backwardRNode)
  {
    AreaDataFileRef areaDataFile(database->GetAreaDataFile());
    WayDataFileRef  wayDataFile(database->GetWayDataFile());

    if (!areaDataFile ||
        !wayDataFile) {
      return false;
    }

    if (object.GetType()==refArea) {
      // TODO:
      return false;
    }
    else if (object.GetType()==refWay) {
      WayRef        way;
      double        startLon=0.0L;
      double        startLat=0.0L;
      size_t        forwardNodePos;
      FileOffset    forwardOffset;
      size_t        backwardNodePos;
      FileOffset    backwardOffset;

      if (!wayDataFile->GetByOffset(object.GetFileOffset(),
                                    way)) {
        log.Error() << "Cannot get start way!";
        return false;
      }

      if (nodeIndex>=way->nodes.size()) {
        log.Error() << "Given start node index " << nodeIndex << " is not within valid range [0," << way->nodes.size()-1;
        return false;
      }

      startLon=way->nodes[nodeIndex].GetLon();
      startLat=way->nodes[nodeIndex].GetLat();

      // Check, if the current node is already the route node
      routeNodeDataFile.Get(way->GetId(nodeIndex),
                            forwardRouteNode);

      if (forwardRouteNode) {
        forwardNodePos=nodeIndex;
      }
      else {
        GetStartForwardRouteNode(profile,
                                 way,
                                 nodeIndex,
                                 forwardRouteNode,
                                 forwardNodePos);

        GetStartBackwardRouteNode(profile,
                                  way,
                                  nodeIndex,
                                  backwardRouteNode,
                                  backwardNodePos);
      }

      if (!forwardRouteNode &&
          !backwardRouteNode) {
        log.Error() << "No route node found for start way";
        return false;
      }

      if (forwardRouteNode) {
        if (!routeNodeDataFile.GetOffset(forwardRouteNode->GetId(),
                                         forwardOffset)) {
          log.Error() << "Cannot get offset of startForwardRouteNode";

          return false;
        }

        RNodeRef node=std::make_shared<RNode>(forwardOffset,
                                              forwardRouteNode,
                                              object);

        node->currentCost=profile.GetCosts(*way,
                                           GetSphericalDistance(startLon,
                                                                startLat,
                                                                way->nodes[forwardNodePos].GetLon(),
                                                                way->nodes[forwardNodePos].GetLat()));
        node->estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                                 startLat,
                                                                 targetLon,
                                                                 targetLat));

        node->overallCost=node->currentCost+node->estimateCost;

        forwardRNode=node;
      }

      if (backwardRouteNode) {
        if (!routeNodeDataFile.GetOffset(backwardRouteNode->GetId(),
                                         backwardOffset)) {
          log.Error() << "Cannot get offset of startBackwardRouteNode";

          return false;
        }

        RNodeRef node=std::make_shared<RNode>(backwardOffset,
                                              backwardRouteNode,
                                              object);

        node->currentCost=profile.GetCosts(*way,
                                           GetSphericalDistance(startLon,
                                                                startLat,
                                                                way->nodes[backwardNodePos].GetLon(),
                                                                way->nodes[backwardNodePos].GetLat()));
        node->estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                                 startLat,
                                                                 targetLon,
                                                                 targetLat));

        node->overallCost=node->currentCost+node->estimateCost;

        backwardRNode=node;
      }

      return true;
    }
    else {
      return false;
    }
  }

  bool RoutingService::GetTargetNodes(const RoutingProfile& profile,
                                      const ObjectFileRef& object,
                                      size_t nodeIndex,
                                      double& targetLon,
                                      double& targetLat,
                                      RouteNodeRef& forwardNode,
                                      RouteNodeRef& backwardNode)
  {
    AreaDataFileRef areaDataFile(database->GetAreaDataFile());
    WayDataFileRef  wayDataFile(database->GetWayDataFile());

    if (!areaDataFile ||
        !wayDataFile) {
      return false;
    }

    if (object.GetType()==refArea) {
      // TODO:
      return false;
    }
    else if (object.GetType()==refWay) {
      WayRef way;

      if (!wayDataFile->GetByOffset(object.GetFileOffset(),
                                    way)) {
        log.Error() << "Cannot get end way!";
        return false;
      }

      if (nodeIndex>=way->nodes.size()) {
        log.Error() << "Given target node index " << nodeIndex << " is not within valid range [0," << way->nodes.size()-1;
        return false;
      }

      targetLon=way->nodes[nodeIndex].GetLon();
      targetLat=way->nodes[nodeIndex].GetLat();

      // Check, if the current node is already the route node
      routeNodeDataFile.Get(way->GetId(nodeIndex),
                            forwardNode);

      if (!forwardNode) {
        GetTargetForwardRouteNode(profile,
                                  way,
                                  nodeIndex,
                                  forwardNode);
        GetTargetBackwardRouteNode(profile,
                                   way,
                                   nodeIndex,
                                   backwardNode);
      }

      if (!forwardNode &&
          !backwardNode) {
        log.Error() << "No route node found for target way";
        return false;
      }

      if (forwardNode) {
        FileOffset forwardRouteNodeOffset;

        if (!routeNodeDataFile.GetOffset(forwardNode->GetId(),
                                         forwardRouteNodeOffset)) {
          log.Error() << "Cannot get offset of targetForwardRouteNode";
        }
      }

      if (backwardNode) {
        FileOffset backwardRouteNodeOffset;

        if (!routeNodeDataFile.GetOffset(backwardNode->GetId(),
                                         backwardRouteNodeOffset)) {
          log.Error() << "Cannot get offset of targetBackwardRouteNode";
        }
      }

      return true;
    }
    else {
      return false;
    }
  }

  /**
   * Calculate a route
   *
   * @param profile
   *    Profile to use
   * @param route
   *    The route object holding the resulting route on success
   * @return
   *    True, if the engine was able to find a route, else false
   */

  bool RoutingService::CalculateRoute(const RoutingProfile& profile,
                                      Vehicle vehicle,
                                      double radius,
                                      std::vector<osmscout::GeoCoord> via,
                                      RouteData& route)
  {
      std::vector<size_t>                  nodeIndexes;
      std::vector<osmscout::ObjectFileRef> objects;

      for (const auto& etap : via) {
        size_t                  targetNodeIndex;
        osmscout::ObjectFileRef targetObject;

        if (!GetClosestRoutableNode(etap.GetLat(),
                                    etap.GetLon(),
                                    vehicle,
                                    radius,
                                    targetObject,
                                    targetNodeIndex)) {
          return false;
        }

        nodeIndexes.push_back(targetNodeIndex);
        objects.push_back(targetObject);
      }

      for (int index=0; index<(int)nodeIndexes.size()-1; index++) {
        size_t                  fromNodeIndex=nodeIndexes.at(index);
        osmscout::ObjectFileRef fromObject=objects.at(index);
        size_t                  toNodeIndex=nodeIndexes.at(index+1);
        osmscout::ObjectFileRef toObject=objects.at(index+1);
        RouteData               *routePart=new RouteData;

        if (!CalculateRoute(profile,
                            fromObject,
                            fromNodeIndex,
                            toObject,
                            toNodeIndex,
                            *routePart)) {
            return false;
        }

        if (routePart->IsEmpty()) {
          return false;
        }

        /* In intermediary via points the end of the previous part is the start of the */
        /* next part, we need to remove the duplicate point in the calculated route */
        if (index<(int)nodeIndexes.size()-2) {
          routePart->PopEntry();
        }

        route.Append(*routePart);

        delete routePart;
      }

      return true;
  }

  /**
   * Calculate a route
   *
   * @param profile
   *    Profile to use
   * @param startObject
   *    Start object
   * @param startNodeIndex
   *    Index of the node within the start object used as starting point
   * @param targetObject
   *    Target object
   * @param targetNodeIndex
   *    Index of the node within the target object used as target point
   * @param route
   *    The route object holding the resulting route on success
   * @return
   *    True, if the engine was able to find a route, else false
   */
  bool RoutingService::CalculateRoute(const RoutingProfile& profile,
                                      const ObjectFileRef& startObject,
                                      size_t startNodeIndex,
                                      const ObjectFileRef& targetObject,
                                      size_t targetNodeIndex,
                                      RouteData& route)
  {
    Vehicle                  vehicle=profile.GetVehicle();
    RouteNodeRef             startForwardRouteNode;
    RouteNodeRef             startBackwardRouteNode;
    RNodeRef                 startForwardNode;
    RNodeRef                 startBackwardNode;

    double                   targetLon=0.0L;
    double                   targetLat=0.0L;

    RouteNodeRef             targetForwardRouteNode;
    RouteNodeRef             targetBackwardRouteNode;

    // Sorted list (smallest cost first) of ways to check (we are using a std::set)
    OpenList                 openList;
    // Map routing nodes by id
    OpenMap                  openMap;
    CloseMap                 closeMap;

    size_t                   nodesLoadedCount=0;
    size_t                   nodesIgnoredCount=0;
    size_t                   maxOpenList=0;
    size_t                   maxCloseMap=0;

    route.Clear();

    openMap.reserve(10000);
    closeMap.reserve(300000);

    if (!GetTargetNodes(profile,
                        targetObject,
                        targetNodeIndex,
                        targetLon,
                        targetLat,
                        targetForwardRouteNode,
                        targetBackwardRouteNode)) {
      return false;
    }

    if (!GetStartNodes(profile,
                       startObject,
                       startNodeIndex,
                       targetLon,
                       targetLat,
                       startForwardRouteNode,
                       startBackwardRouteNode,
                       startForwardNode,
                       startBackwardNode)) {
      return false;
    }

    if (startForwardNode) {
      std::pair<OpenListRef,bool> result=openList.insert(startForwardNode);

      openMap[startForwardNode->nodeOffset]=result.first;
    }

    if (startBackwardNode) {
      std::pair<OpenListRef,bool> result=openList.insert(startBackwardNode);

      openMap[startBackwardNode->nodeOffset]=result.first;
    }

    StopClock    clock;
    RNodeRef     current;
    RouteNodeRef currentRouteNode;

    do {
      //
      // Take entry from open list with lowest cost
      //

      current=*openList.begin();

      openMap.erase(current->nodeOffset);
      openList.erase(openList.begin());

      currentRouteNode=current->node;

      nodesLoadedCount++;

      bool accessViolation=false;

      // Get potential follower in the current way

#if defined(DEBUG_ROUTING)
      std::cout << "Analysing follower of node " << currentRouteNode->GetFileOffset();
      std::cout << " (" << current->object.GetTypeName() << " " << current->object.GetFileOffset() << "["  << currentRouteNode->GetId() << "]" << ")";
      std::cout << " " << current->currentCost << " " << current->estimateCost << " " << current->overallCost << std::endl;
#endif
      size_t i=0;
      for (const auto& path : currentRouteNode->paths) {
        if (path.offset==current->prev) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path.offset;
          std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetTypeName() << " " << currentRouteNode->objects[path.objectIndex].object.GetFileOffset() << ")";
          std::cout << " => back to the last node visited" << std::endl;
#endif
          nodesIgnoredCount++;
          i++;
          continue;
        }

        if (!current->access &&
            !path.IsRestricted(vehicle)) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path.offset;
          std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetTypeName() << " " << currentRouteNode->objects[path.objectIndex].object.GetFileOffset() << ")";
          std::cout << " => moving from non-accessible way back to accessible way" << std::endl;
#endif
          nodesIgnoredCount++;
          i++;

          accessViolation=true;

          continue;
        }

        if (!profile.CanUse(*currentRouteNode,objectVariantData,i)) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path.offset;
          std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetTypeName() << " " << currentRouteNode->objects[path.objectIndex].object.GetFileOffset() << ")";
          std::cout << " => Cannot be used"<< std::endl;
#endif
          nodesIgnoredCount++;
          i++;
          continue;
        }

        if (closeMap.find(path.offset)!=closeMap.end()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path.offset;
          std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetTypeName() << " " << currentRouteNode->objects[path.objectIndex].object.GetFileOffset() << ")";
          std::cout << " => already calculated" << std::endl;
#endif
          i++;
          continue;
        }

        if (!currentRouteNode->excludes.empty()) {
          bool canTurnedInto=true;

          for (const auto& exclude : currentRouteNode->excludes) {
            if (exclude.source==current->object &&
                exclude.targetIndex==i) {
#if defined(DEBUG_ROUTING)
              std::cout << "  Skipping route";
              std::cout << " to " << path.offset;
              std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetTypeName() << " " << currentRouteNode->objects[path.objectIndex].object.GetFileOffset() << ")";
              std::cout << " => turn not allowed" << std::endl;
#endif
              canTurnedInto=false;
              break;
            }
          }

          if (!canTurnedInto) {
            nodesIgnoredCount++;
            i++;
            continue;
          }
        }

        double currentCost=current->currentCost+
                           profile.GetCosts(*currentRouteNode,objectVariantData,i);

        OpenMap::iterator openEntry=openMap.find(path.offset);

        // Check, if we already have a cheaper path to the new node. If yes, do not put the new path
        // into the open list
        if (openEntry!=openMap.end() &&
            (*openEntry->second)->currentCost<=currentCost) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path.offset;
          std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetTypeName() << " " << currentRouteNode->objects[path.objectIndex].object.GetFileOffset() << ")";
          std::cout << "  => cheaper route exists " << currentCost << "<=>" << (*openEntry->second)->currentCost << std::endl;
#endif
          i++;
          continue;
        }

        RouteNodeRef nextNode;

        if (openEntry!=openMap.end()) {
          nextNode=(*openEntry->second)->node;
        }
        else {
          if (!routeNodeDataFile.GetByOffset(path.offset,
                                             nextNode)) {
            log.Error() << "Cannot load route node with id " << path.offset;
            return false;
          }
        }

        double distanceToTarget=GetSphericalDistance(nextNode->GetCoord().GetLon(),
                                                     nextNode->GetCoord().GetLat(),
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
          node->object=currentRouteNode->objects[path.objectIndex].object;

          node->currentCost=currentCost;
          node->estimateCost=estimateCost;
          node->overallCost=overallCost;
          node->access=!currentRouteNode->paths[i].IsRestricted(vehicle);

#if defined(DEBUG_ROUTING)
          std::cout << "  Updating route " << current->nodeOffset << " via " << node->object.GetTypeName() << " " << node->object.GetFileOffset() << " " << currentCost << " " << estimateCost << " " << overallCost << " " << currentRouteNode->id << std::endl;
#endif

          openList.erase(openEntry->second);

          std::pair<OpenListRef,bool> result=openList.insert(node);
          openEntry->second=result.first;
        }
        else {
          RNodeRef node=std::make_shared<RNode>(path.offset,
                                                nextNode,
                                                currentRouteNode->objects[path.objectIndex].object,
                                                current->nodeOffset);

          node->currentCost=currentCost;
          node->estimateCost=estimateCost;
          node->overallCost=overallCost;
          node->access=!path.IsRestricted(vehicle);

#if defined(DEBUG_ROUTING)
          std::cout << "  Inserting route to " << path.offset;
          std::cout <<  " (" << node->object.GetTypeName() << " " << node->object.GetFileOffset() << ")";
          std::cout << " " << currentCost << " " << estimateCost << " " << overallCost << " " << currentRouteNode->id << std::endl;
#endif

          std::pair<OpenListRef,bool> result=openList.insert(node);
          openMap[node->nodeOffset]=result.first;
        }

        i++;
      }

      //
      // Added current node to close map
      //

      if (!accessViolation) {
        closeMap[current->nodeOffset]=current;
      }
      current->node=NULL;

      maxOpenList=std::max(maxOpenList,openMap.size());
      maxCloseMap=std::max(maxCloseMap,closeMap.size());

#if defined(DEBUG_ROUTING)
      if (openList.empty()) {
        std::cout << "No more alternatives, stopping" << std::endl;
      }

      if ((targetForwardRouteNode && current->nodeOffset==targetForwardRouteNode->fileOffset)) {
        std::cout << "Reached target: " << current->nodeOffset << " == " << targetForwardRouteNode->fileOffset << " (forward)" << std::endl;
      }

      if (targetBackwardRouteNode && current->nodeOffset==targetBackwardRouteNode->fileOffset) {
        std::cout << "Reached target: " << current->nodeOffset << " == " << targetBackwardRouteNode->fileOffset << " (backward)" << std::endl;
      }
#endif
    } while (!openList.empty() &&
             (!targetForwardRouteNode || current->nodeOffset!=targetForwardRouteNode->GetFileOffset()) &&
             (!targetBackwardRouteNode || current->nodeOffset!=targetBackwardRouteNode->GetFileOffset()));

    clock.Stop();

    if (debugPerformance) {
      std::cout << "From:                " << startObject.GetTypeName() << " " << startObject.GetFileOffset();
      std::cout << "[";
      if (startBackwardRouteNode) {
        std::cout << startBackwardRouteNode->GetId() << " >* ";
      }
      std::cout << startNodeIndex;
      if (startForwardRouteNode) {
        std::cout << " *> " << startForwardRouteNode->GetId();
      }
      std::cout << "]" << std::endl;

      std::cout << "To:                  " << targetObject.GetTypeName() <<  " " << targetObject.GetFileOffset();
      std::cout << "[";
      if (targetForwardRouteNode) {
        std::cout << targetForwardRouteNode->GetId() << " >* ";
      }
      std::cout << targetNodeIndex;
      if (targetBackwardRouteNode) {
        std::cout << " *> " << targetBackwardRouteNode->GetId();
      }
      std::cout << "]" << std::endl;

      std::cout << "Time:                " << clock << std::endl;

      std::cout << "Route nodes loaded:  " << nodesLoadedCount << std::endl;
      std::cout << "Route nodes ignored: " << nodesIgnoredCount << std::endl;
      std::cout << "Max. OpenList size:  " << maxOpenList << std::endl;
      std::cout << "Max. CloseMap size:  " << maxCloseMap << std::endl;
    }

    if (!((targetForwardRouteNode && currentRouteNode->GetId()==targetForwardRouteNode->GetId()) ||
          (targetBackwardRouteNode && currentRouteNode->GetId()==targetBackwardRouteNode->GetId()))) {
      std::cout << "No route found!" << std::endl;
      route.Clear();

      return true;
    }

    std::list<RNodeRef> nodes;

    ResolveRNodeChainToList(current,
                            closeMap,
                            nodes);

    if (!ResolveRNodesToRouteData(profile,
                                  nodes,
                                  startObject,
                                  startNodeIndex,
                                  targetObject,
                                  targetNodeIndex,
                                  route)) {
      //std::cerr << "Cannot convert routing result to route data" << std::endl;
      return false;
    }

    ResolveRouteDataJunctions(route);

    return true;
  }

  /**
   * Transforms the route into a Way
   * @param data
   *    Route data
   * @param way
   *    Way to get initialized to the route on success
   * @return
   *    True, if the way could be build, else false
   */
  bool RoutingService::TransformRouteDataToWay(const RouteData& data,
                                               Way& way)
  {
    TypeConfigRef   typeConfig=database->GetTypeConfig();
    AreaDataFileRef areaDataFile(database->GetAreaDataFile());
    WayDataFileRef  wayDataFile(database->GetWayDataFile());

    Way             tmp;

    if (!typeConfig ||
        !areaDataFile ||
        !wayDataFile) {
      return false;
    }

    TypeInfoRef routeType=typeConfig->GetTypeInfo("_route");

    assert(routeType!=typeConfig->typeInfoIgnore);

    way=tmp;

    way.SetType(routeType);
    way.SetLayerToMax();
    way.nodes.reserve(data.Entries().size());

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      if (iter->GetPathObject().Valid()) {
        if (iter->GetPathObject().GetType()==refArea) {
          AreaRef a;

          if (!areaDataFile->GetByOffset(iter->GetPathObject().GetFileOffset(),a)) {
            return false;
          }

          // Initial starting point
          if (iter==data.Entries().begin()) {
            size_t index=iter->GetCurrentNodeIndex();

            way.nodes.push_back(a->rings.front().nodes[index]);
          }

          size_t index=iter->GetTargetNodeIndex();

          way.nodes.push_back(a->rings.front().nodes[index]);
        }
        else if (iter->GetPathObject().GetType()==refWay) {
          WayRef w;

          if (!wayDataFile->GetByOffset(iter->GetPathObject().GetFileOffset(),w)) {
            return false;
          }

          // Initial starting point
          if (iter==data.Entries().begin()) {
            size_t index=iter->GetCurrentNodeIndex();

            way.nodes.push_back(w->nodes[index]);
          }

          size_t index=iter->GetTargetNodeIndex();

          way.nodes.push_back(w->nodes[index]);
        }
      }
    }

    return true;
  }

  /**
   * Transforms the route into a list of points.
   * @param data
   *    Route data
   * @param points
   *    A list of the points holding route nodes
   * @return
   *    True, if the way could be build, else false
   */
  bool RoutingService::TransformRouteDataToPoints(const RouteData& data,
                                                  std::list<Point>& points)
  {
    AreaDataFileRef areaDataFile(database->GetAreaDataFile());
    WayDataFileRef  wayDataFile(database->GetWayDataFile());

    if (!areaDataFile ||
        !wayDataFile) {
      return false;
    }

    AreaRef a;
    WayRef  w;

    points.clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      if (iter->GetPathObject().Valid()) {
        if (iter->GetPathObject().GetType()==refArea) {
          if (!a ||
              a->GetFileOffset()!=iter->GetPathObject().GetFileOffset()) {
            if (!areaDataFile->GetByOffset(iter->GetPathObject().GetFileOffset(),a)) {
              log.Error() << "Cannot load area with id " << iter->GetPathObject().GetFileOffset();
              return false;
            }
          }

          // Initial starting point
          if (iter==data.Entries().begin()) {
            size_t index=iter->GetCurrentNodeIndex();

            points.push_back(a->rings.front().nodes[index]);
          }

          // target node of current path
          size_t index=iter->GetTargetNodeIndex();

          points.push_back(a->rings.front().nodes[index]);
        }
        else if (iter->GetPathObject().GetType()==refWay) {
          if (!w ||
              w->GetFileOffset()!=iter->GetPathObject().GetFileOffset()) {
            if (!wayDataFile->GetByOffset(iter->GetPathObject().GetFileOffset(),w)) {
              log.Error() << "Cannot load way with id " << iter->GetPathObject().GetFileOffset();
              return false;
            }
          }

          // Initial starting point
          if (iter==data.Entries().begin()) {
            size_t index=iter->GetCurrentNodeIndex();

            points.push_back(Point(w->GetId(index),
                                   w->GetCoord(index)));
          }

          // target node of current path
          size_t index=iter->GetTargetNodeIndex();

          points.push_back(Point(w->GetId(index),
                                 w->GetCoord(index)));
        }
      }
    }

    return true;
  }

  /**
   * Transform the route into a RouteDescription. The RouteDescription can be further transformed
   * to enhanced textual and/or visual description of the route containing additional information.
   * @param data
   *    Route data
   * @param description
   *    An initialized description on success
   * @return
   *    True on success, else false
   */
  bool RoutingService::TransformRouteDataToRouteDescription(const RouteData& data,
                                                            RouteDescription& description)
  {
    description.Clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      description.AddNode(iter->GetCurrentNodeIndex(),
                          iter->GetObjects(),
                          iter->GetPathObject(),
                          iter->GetTargetNodeIndex());
    }

    return true;
  }

  void RoutingService::DumpStatistics()
  {
    if (database) {
      database->DumpStatistics();
    }
  }

  /**
   * Returns the closed routeable object (area or way) relative
   * to the given coordinate.
   *
   * @note The returned node may in fact not be routable, it is just
   * the closest node to the given position on a routable way or area.
   *
   * @note The actual object may not be within the given radius
   * due to internal search index resolution.
   *
   * @param lat
   *    Latitude value of the search center
   * @param lon
   *    Longitude value of the search center
   * @param vehicle
   *    Vehicle to use (may differ from the vehicle the router was initialized
   *    but tin this case the route will not return a valid route based on this
   *    object).
   * @param radius
   *    The maximum radius to search in from the search center in meter
   * @param object
   *    The resulting object if one was found
   * @param nodeIndex
   *    The index of the closed node to the search center.
   * @return
   */
  bool RoutingService::GetClosestRoutableNode(double lat,
                                              double lon,
                                              const osmscout::Vehicle& vehicle,
                                              double radius,
                                              osmscout::ObjectFileRef& object,
                                              size_t& nodeIndex) const
  {
    object.Invalidate();
    nodeIndex=std::numeric_limits<size_t>::max();

    TypeConfigRef    typeConfig=database->GetTypeConfig();
    AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();
    AreaWayIndexRef  areaWayIndex=database->GetAreaWayIndex();
    AreaDataFileRef  areaDataFile=database->GetAreaDataFile();
    WayDataFileRef   wayDataFile=database->GetWayDataFile();

    if (!typeConfig ||
        !areaAreaIndex ||
        !areaWayIndex ||
        !areaDataFile ||
        !wayDataFile) {
      log.Error() << "At least one index file is invalid!";
      return false;
    }

    GeoBox      boundingBox=GeoBox::BoxByCenterAndRadius(GeoCoord(lat,lon),radius);
    TypeInfoSet wayRoutableTypes;
    TypeInfoSet areaRoutableTypes;
    TypeInfoSet wayLoadedTypes;
    TypeInfoSet areaLoadedTypes;

    for (const auto& type : database->GetTypeConfig()->GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanRoute(vehicle)) {
        if (type->CanBeWay()) {
          wayRoutableTypes.Set(type);
        }

        if (type->CanBeArea()) {
          areaRoutableTypes.Set(type);
        }
      }
    }

    std::vector<FileOffset>        wayWayOffsets;
    std::vector<DataBlockSpan>     wayAreaSpans;
    std::vector<osmscout::AreaRef> areas;
    std::vector<osmscout::WayRef>  ways;

    if (!areaWayIndex->GetOffsets(boundingBox,
                                  wayRoutableTypes,
                                  wayWayOffsets,
                                  wayLoadedTypes)) {
      log.Error() << "Error getting ways from area way index!";
    }

    if (!areaAreaIndex->GetAreasInArea(*database->GetTypeConfig(),
                                       boundingBox,
                                       std::numeric_limits<size_t>::max(),
                                       areaRoutableTypes,
                                       wayAreaSpans,
                                       areaLoadedTypes)) {
      log.Error() << "Error getting areas from area area index!";
    }

    std::sort(wayWayOffsets.begin(),
              wayWayOffsets.end());

    if (!wayDataFile->GetByOffset(wayWayOffsets,
                                  ways)) {
      log.Error() << "Error reading ways in area!";
      return false;
    }

    if (!areaDataFile->GetByBlockSpans(wayAreaSpans,
                                       areas)) {
      log.Error() << "Error reading areas in area!";
      return false;
    }

    double minDistance=std::numeric_limits<double>::max();

    for (const auto& area : areas) {
      if (!HasNodeWithId(area->rings[0].nodes)) {
        continue;
      }

      for (size_t i=0; i<area->rings[0].nodes.size(); i++) {
        double distance=sqrt((area->rings[0].nodes[i].GetLat()-lat)*(area->rings[0].nodes[i].GetLat()-lat)+
                             (area->rings[0].nodes[i].GetLon()-lon)*(area->rings[0].nodes[i].GetLon()-lon));

        if (distance<minDistance) {
          minDistance=distance;

          object.Set(area->GetFileOffset(),osmscout::refArea);
          nodeIndex=i;
        }
      }
    }

    for (const auto& way : ways) {
      if (!HasNodeWithId(way->nodes)) {
        continue;
      }

      for (size_t i=0;  i<way->nodes.size(); i++) {
        double distance=sqrt((way->nodes[i].GetLat()-lat)*(way->nodes[i].GetLat()-lat)+
                             (way->nodes[i].GetLon()-lon)*(way->nodes[i].GetLon()-lon));
        if (distance<minDistance) {
          minDistance=distance;

          object.Set(way->GetFileOffset(),osmscout::refWay);
          nodeIndex=i;
        }
      }
    }

    return true;
  }
}
