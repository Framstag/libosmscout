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
#include <iomanip>
#include <iostream>

#include <osmscout/RoutingProfile.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING

namespace osmscout {

  RoutePosition::RoutePosition()
  : nodeIndex(0)
  {
    // no code
  }

  RoutePosition::RoutePosition(const ObjectFileRef& object,
                               size_t nodeIndex)
  : object(object),
    nodeIndex(nodeIndex)
  {
    // no code
  }

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

  RoutingProgress::~RoutingProgress()
  {
    // no code
  }

  void RoutingParameter::SetBreaker(const BreakerRef& breaker)
  {
    this->breaker=breaker;
  }

  void RoutingParameter::SetProgress(const RoutingProgressRef& progress)
  {
    this->progress=progress;
  }

  RoutingResult::RoutingResult()
  : currentMaxDistance(0.0),
    overallDistance(0.0)
  {
    // no code
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
                      10000)
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

    if (!objectVariantDataFile.Load(*database->GetTypeConfig(),
                                    AppendFileToDir(path,
                                                    GetData2Filename(filenamebase)))) {
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

    for (long i=(long)nodeIndex-1; i>=0; i--) {
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

    for (long i=(long)nodeIndex-1; i>=0; i--) {
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

  void RoutingService::ResolveRNodeChainToList(FileOffset finalRouteNode,
                                               const ClosedSet& closedSet,
                                               std::list<VNode>& nodes)
  {
    ClosedSet::const_iterator current=closedSet.find(VNode(finalRouteNode));

    assert(current!=closedSet.end());

    while (current->previousNode!=0) {
      ClosedSet::const_iterator prev=closedSet.find(VNode(current->previousNode));

      assert(prev!=closedSet.end());

      nodes.push_back(*current);

      current=prev;
    }

    nodes.push_back(*current);

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

      for (long i=(long)startNodeIndex-1; i>(long)targetNodeIndex+1; i--) {
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
                                                const std::list<VNode>& nodes,
                                                const RoutePosition& start,
                                                const RoutePosition& target,
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
      routeNodeOffsets.insert(node.currentNode);

      if (node.object.Valid()) {
        switch (node.object.GetType()) {
        case refArea:
          areaOffsets.insert(node.object.GetFileOffset());
          break;
        case refWay:
          wayOffsets.insert(node.object.GetFileOffset());
          break;
        default:
          assert(false);
          break;
        }
      }
    }

    // Collect area/way file offset for the
    // target object (not traversed by above loop)
    switch (target.GetObjectFileRef().GetType()) {
    case refArea:
      areaOffsets.insert(target.GetObjectFileRef().GetFileOffset());
      break;
    case refWay:
      wayOffsets.insert(target.GetObjectFileRef().GetFileOffset());
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

    assert(start.GetObjectFileRef().GetType()==refArea ||
           start.GetObjectFileRef().GetType()==refWay);

    if (start.GetObjectFileRef().GetType()==refArea) {
      std::unordered_map<FileOffset,AreaRef>::const_iterator entry=areaMap.find(start.GetObjectFileRef().GetFileOffset());

      assert(entry!=areaMap.end());

      ids=&entry->second->rings.front().nodes;
      oneway=false;
    }
    else if (start.GetObjectFileRef().GetType()==refWay) {
      std::unordered_map<FileOffset,WayRef>::const_iterator entry=wayMap.find(start.GetObjectFileRef().GetFileOffset());

      assert(entry!=wayMap.end());

      ids=&entry->second->nodes;
      oneway=!profile.CanUseBackward(*entry->second);
    }

    if (nodes.empty()) {
      // We assume that startNode and targetNode are on the same area/way (with no routing node in between)
      assert(start.GetObjectFileRef()==target.GetObjectFileRef());

      AddNodes(route,
               (*ids)[start.GetNodeIndex()].GetId(),
               start.GetNodeIndex(),
               start.GetObjectFileRef(),
               ids->size(),
               oneway,
               target.GetNodeIndex());

      route.AddEntry(0,
                     target.GetNodeIndex(),
                     ObjectFileRef(),
                     0);
      return true;
    }

    RouteNodeRef initialNode=routeNodeMap.find(nodes.front().currentNode)->second;

    //
    // Add The path from the start node to the first routing node
    //
    if ((*ids)[start.GetNodeIndex()].GetId()!=initialNode->GetId()) {
      size_t routeNodeIndex=0;

      while ((*ids)[routeNodeIndex].GetId()!=initialNode->GetId() &&
          routeNodeIndex<ids->size()) {
        routeNodeIndex++;
      }
      assert(routeNodeIndex<ids->size());

      // Start node to initial route node
      AddNodes(route,
               (*ids)[start.GetNodeIndex()].GetId(),
               start.GetNodeIndex(),
               start.GetObjectFileRef(),
               ids->size(),
               oneway,
               routeNodeIndex);
    }

    //
    // Walk the routing path from route node to the next route node
    // and build entries.
    //
    for (std::list<VNode>::const_iterator n=nodes.begin();
        n!=nodes.end();
        n++) {
      std::list<VNode>::const_iterator nn=n;

      nn++;

      RouteNodeRef node=routeNodeMap.find(n->currentNode)->second;

      //
      // The path from the last routing node to the target node and the
      // target node itself
      //
      if (nn==nodes.end()) {
        assert(target.GetObjectFileRef().GetType()==refArea ||
               target.GetObjectFileRef().GetType()==refWay);

        if (target.GetObjectFileRef().GetType()==refArea) {
          std::unordered_map<FileOffset,AreaRef>::const_iterator entry=areaMap.find(target.GetObjectFileRef().GetFileOffset());

          assert(entry!=areaMap.end());

          ids=&entry->second->rings.front().nodes;
          oneway=false;
        }
        else if (target.GetObjectFileRef().GetType()==refWay) {
          std::unordered_map<FileOffset,WayRef>::const_iterator entry=wayMap.find(target.GetObjectFileRef().GetFileOffset());

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

        if (currentNodeIndex!=target.GetNodeIndex()) {
          AddNodes(route,
                   (*ids)[currentNodeIndex].GetId(),
                   currentNodeIndex,
                   target.GetObjectFileRef(),
                   ids->size(),
                   oneway,
                   target.GetNodeIndex());
        }

        route.AddEntry(0,
                       target.GetNodeIndex(),
                       ObjectFileRef(),
                       0);

        break;
      }

      RouteNodeRef nextNode=routeNodeMap.find(nn->currentNode)->second;

      assert(nn->object.GetType()==refArea ||
             nn->object.GetType()==refWay);

      if (nn->object.GetType()==refArea) {
        std::unordered_map<FileOffset,AreaRef>::const_iterator entry=areaMap.find(nn->object.GetFileOffset());

        assert(entry!=areaMap.end());

        ids=&entry->second->rings.front().nodes;
        oneway=false;
      }
      else if (nn->object.GetType()==refWay) {
        std::unordered_map<FileOffset,WayRef>::const_iterator entry=wayMap.find(nn->object.GetFileOffset());

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
               nn->object,
               ids->size(),
               oneway,
               nextNodeIndex);
    }

    return true;
  }

  bool RoutingService::ResolveRouteDataJunctions(RouteData& route)
  {
    std::set<Id> nodeIds;

    for (const auto& routeEntry : route.Entries()) {
      if (routeEntry.GetCurrentNodeId()!=0) {
        nodeIds.insert(routeEntry.GetCurrentNodeId());
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

    for (const auto& junction : junctions) {
      junctionMap.insert(std::make_pair(junction->GetId(),junction));
    }

    junctions.clear();

    for (auto& routeEntry : route.Entries()) {
      if (routeEntry.GetCurrentNodeId()!=0) {
        auto junctionEntry=junctionMap.find(routeEntry.GetCurrentNodeId());
        if (junctionEntry!=junctionMap.end()) {
          routeEntry.SetObjects(junctionEntry->second->GetObjects());
        }
      }
    }

    return junctionDataFile.Close();
  }

  bool RoutingService::GetRNode(const RoutingProfile& profile,
                                const RoutePosition& position,
                                const WayRef& way,
                                size_t routeNodeIndex,
                                const RouteNodeRef& routeNode,
                                const GeoCoord& startCoord,
                                const GeoCoord& targetCoord,
                                RNodeRef& node)
  {
    FileOffset offset;

    node=NULL;

    if (!routeNodeDataFile.GetOffset(routeNode->GetId(),
                                     offset)) {
      log.Error() << "Cannot get offset of startForwardRouteNode";

      return false;
    }

    node=std::make_shared<RNode>(offset,
                                 routeNode,
                                 position.GetObjectFileRef());

    node->currentCost=profile.GetCosts(*way,
                                       GetSphericalDistance(startCoord,
                                                            way->nodes[routeNodeIndex].GetCoord()));
    node->estimateCost=profile.GetCosts(GetSphericalDistance(startCoord,
                                                             targetCoord));

    node->overallCost=node->currentCost+node->estimateCost;

    return true;
  }

  /**
   * The start position is at the given way and the index of the node within
   * the object. Return the closest route node and routing node either in the
   * forward or backward direction or both.
   *
   * @param profile
   *    The routing profile
   * @param position
   *    The start position
   * @param startCoord
   *    The coordinate of the start position
   * @param targetCoord
   *    The coordinate of the target position
   * @param forwardRouteNode
   *    Optional route node in the forward direction
   * @param backwardRouteNode
   *    Optional route node in the backward direction
   * @param forwardRNode
   *    Optional prefilled routing node for the forward direction to be used as part of the routing process
   * @param backwardRNode
   *    Optional prefilled routing node for the backward direction to be used as part of the routing process
   * @return
   *    True, if at least one route node was found. If not or in case of technical errors false is returned.
   */
  bool RoutingService::GetWayStartNodes(const RoutingProfile& profile,
                                        const RoutePosition& position,
                                        GeoCoord& startCoord,
                                        const GeoCoord& targetCoord,
                                        RouteNodeRef& forwardRouteNode,
                                        RouteNodeRef& backwardRouteNode,
                                        RNodeRef& forwardRNode,
                                        RNodeRef& backwardRNode)
  {
    WayDataFileRef  wayDataFile(database->GetWayDataFile());

    if (!wayDataFile) {
      return false;
    }

    assert(position.GetObjectFileRef().GetType()==refWay);

    WayRef     way;
    size_t     forwardNodePos;
    size_t     backwardNodePos;

    if (!wayDataFile->GetByOffset(position.GetObjectFileRef().GetFileOffset(),
                                  way)) {
      log.Error() << "Cannot get start way!";
      return false;
    }

    if (position.GetNodeIndex()>=way->nodes.size()) {
      log.Error() << "Given start node index " << position.GetNodeIndex() << " is not within valid range [0," << way->nodes.size()-1;
      return false;
    }

    startCoord=way->nodes[position.GetNodeIndex()].GetCoord();

    // Check, if the current node is already the route node
    routeNodeDataFile.Get(way->GetId(position.GetNodeIndex()),
                          forwardRouteNode);

    if (forwardRouteNode) {
      forwardNodePos=position.GetNodeIndex();
    }
    else {
      GetStartForwardRouteNode(profile,
                               way,
                               position.GetNodeIndex(),
                               forwardRouteNode,
                               forwardNodePos);

      GetStartBackwardRouteNode(profile,
                                way,
                                position.GetNodeIndex(),
                                backwardRouteNode,
                                backwardNodePos);
    }

    if (!forwardRouteNode &&
        !backwardRouteNode) {
      log.Error() << "No route node found for start way";
      return false;
    }

    if (forwardRouteNode &&
        !GetRNode(profile,
                  position,
                  way,
                  forwardNodePos,
                  forwardRouteNode,
                  startCoord,
                  targetCoord,
                  forwardRNode)) {
      return false;
    }

    if (backwardRouteNode &&
        !GetRNode(profile,
                  position,
                  way,
                  backwardNodePos,
                  backwardRouteNode,
                  startCoord,
                  targetCoord,
                  backwardRNode)) {
      return false;
    }

    return true;
  }

  /**
   * The start position is at the given position defined by an object and the index of the node within
   * the object. Return the closest route node and routing node either in the
   * forward or backward direction or both.
   *
   * @param profile
   *    The routing profile
   * @param position
   *    The start position
   * @param startCoord
   *    The coordinate of the start position
   * @param targetCoord
   *    The coordinate of the target position
   * @param forwardRouteNode
   *    Optional route node in the forward direction
   * @param backwardRouteNode
   *    Optional route node in the backward direction
   * @param forwardRNode
   *    Optional prefilled routing node for the forward direction to be used as part of the routing process
   * @param backwardRNode
   *    Optional prefilled routing node for the backward direction to be used as part of the routing process
   * @return
   *    True, if at least one route node was found. If not or in case of technical errors false is returned.
   */
  bool RoutingService::GetStartNodes(const RoutingProfile& profile,
                                     const RoutePosition& position,
                                     GeoCoord& startCoord,
                                     const GeoCoord& targetCoord,
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

    if (position.GetObjectFileRef().GetType()==refWay) {
      return GetWayStartNodes(profile,
                             position,
                             startCoord,
                             targetCoord,
                             forwardRouteNode,
                             backwardRouteNode,
                             forwardRNode,
                             backwardRNode);
    }
    else {
      log.Error() << "Unsupported object type '" << position.GetObjectFileRef().GetTypeName() << "' for source!";
      return false;
    }
  }

  /**
   * The target position is at the given position defined by an object and the index of the node within
   * the object. Return the closest route node and routing node either in the
   * forward or backward direction or both.
   *
   * @param profile
   *    The routing profile
   * @param position
   *    The start position
   * @param targetCoord
   *    The coordinate of the target position
   * @param forwardRouteNode
   *    Optional route node in the forward direction
   * @param backwardRouteNode
   *    Optional route node in the backward direction
   * @return
   *    True, if at least one route node was found. If not or in case of technical errors false is returned.
   */
  bool RoutingService::GetWayTargetNodes(const RoutingProfile& profile,
                                         const RoutePosition& position,
                                         GeoCoord& targetCoord,
                                         RouteNodeRef& forwardNode,
                                         RouteNodeRef& backwardNode)
  {
    WayDataFileRef wayDataFile(database->GetWayDataFile());

    if (!wayDataFile) {
      return false;
    }

    assert(position.GetObjectFileRef().GetType()==refWay);

    WayRef way;

    if (!wayDataFile->GetByOffset(position.GetObjectFileRef().GetFileOffset(),
                                  way)) {
      log.Error() << "Cannot get end way!";
      return false;
    }

    if (position.GetNodeIndex()>=way->nodes.size()) {
      log.Error() << "Given target node index " << position.GetNodeIndex() << " is not within valid range [0," << way->nodes.size()-1;
      return false;
    }

    targetCoord=way->nodes[position.GetNodeIndex()].GetCoord();

    // Check, if the current node is already the route node
    routeNodeDataFile.Get(way->GetId(position.GetNodeIndex()),
                          forwardNode);

    if (!forwardNode) {
      GetTargetForwardRouteNode(profile,
                                way,
                                position.GetNodeIndex(),
                                forwardNode);
      GetTargetBackwardRouteNode(profile,
                                 way,
                                 position.GetNodeIndex(),
                                 backwardNode);
    }

    if (!forwardNode &&
        !backwardNode) {
      log.Error() << "No route node found for target way";
      return false;
    }

    return true;
  }

  /**
   * The target position is at the given position defined by an object and the index of the node within
   * the object. Return the closest route node and routing node either in the
   * forward or backward direction or both.
   *
   * @param profile
   *    The routing profile
   * @param position
   *    The start position
   * @param targetCoord
   *    The coordinate of the target position
   * @param forwardRouteNode
   *    Optional route node in the forward direction
   * @param backwardRouteNode
   *    Optional route node in the backward direction
   * @return
   *    True, if at least one route node was found. If not or in case of technical errors false is returned.
   */
  bool RoutingService::GetTargetNodes(const RoutingProfile& profile,
                                      const RoutePosition& position,
                                      GeoCoord& targetCoord,
                                      RouteNodeRef& forwardNode,
                                      RouteNodeRef& backwardNode)
  {
    if (position.GetObjectFileRef().GetType()==refWay) {
      return GetWayTargetNodes(profile,
                              position,
                              targetCoord,
                              forwardNode,
                              backwardNode);
    }
    else {
      log.Error() << "Unsupported object type '" << position.GetObjectFileRef().GetTypeName() << "' for target!";
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

  RoutingResult RoutingService::CalculateRoute(const RoutingProfile& profile,
                                               std::vector<osmscout::GeoCoord> via,
                                               double radius,
                                               const RoutingParameter& parameter)
  {
    RoutingResult                        result;
    std::vector<size_t>                  nodeIndexes;
    std::vector<osmscout::ObjectFileRef> objects;

    for (const auto& etap : via) {
      RoutePosition target=GetClosestRoutableNode(etap,
                                                  profile,
                                                  radius);

      if (!target.IsValid()) {
        return result;
      }

      nodeIndexes.push_back(target.GetNodeIndex());
      objects.push_back(target.GetObjectFileRef());
    }

    for (int index=0; index<(int)nodeIndexes.size()-1; index++) {
      size_t                     fromNodeIndex=nodeIndexes.at(index);
      osmscout::ObjectFileRef    fromObject=objects.at(index);
      size_t                     toNodeIndex=nodeIndexes.at(index+1);
      osmscout::ObjectFileRef    toObject=objects.at(index+1);
      RoutingResult              partialResult;

      partialResult=CalculateRoute(profile,
                                   RoutePosition(fromObject,fromNodeIndex),
                                   RoutePosition(toObject,toNodeIndex),
                                   parameter);
      if (!partialResult.Success()) {
        result.GetRoute().Clear();

        return result;
      }

      /* In intermediary via points the end of the previous part is the start of the */
      /* next part, we need to remove the duplicate point in the calculated route */
      if (index<(int)nodeIndexes.size()-2) {
        result.GetRoute().PopEntry();
      }

      result.GetRoute().Append(partialResult.GetRoute());
    }

    return result;
  }

  /**
   * Calculate a route
   *
   * @param profile
   *    Profile to use
   * @param start
   *    Start of the route
   * @param target
   *    Target of teh route
   * @param progress
   *    Optional callback for handling routing progress
   * @param route
   *    The route object holding the resulting route on success
   * @return
   *    True, if the engine was able to find a route, else false
   */
  RoutingResult RoutingService::CalculateRoute(const RoutingProfile& profile,
                                               const RoutePosition& start,
                                               const RoutePosition& target,
                                               const RoutingParameter& parameter)
  {
    RoutingResult            result;
    Vehicle                  vehicle=profile.GetVehicle();
    RouteNodeRef             startForwardRouteNode;
    RouteNodeRef             startBackwardRouteNode;
    RNodeRef                 startForwardNode;
    RNodeRef                 startBackwardNode;

    GeoCoord                 startCoord;
    GeoCoord                 targetCoord;

    RouteNodeRef             targetForwardRouteNode;
    RouteNodeRef             targetBackwardRouteNode;

    // Sorted list (smallest cost first) of ways to check (we are using a std::set)
    OpenList                 openList;
    // Map routing nodes by id
    OpenMap                  openMap;
    ClosedSet                closedSet;

    size_t                   nodesLoadedCount=0;
    size_t                   nodesIgnoredCount=0;
    size_t                   maxOpenList=0;
    size_t                   maxClosedSet=0;

    openMap.reserve(10000);
    closedSet.reserve(300000);

    if (!GetTargetNodes(profile,
                        target,
                        targetCoord,
                        targetForwardRouteNode,
                        targetBackwardRouteNode)) {
      return result;
    }

    if (!GetStartNodes(profile,
                       start,
                       startCoord,
                       targetCoord,
                       startForwardRouteNode,
                       startBackwardRouteNode,
                       startForwardNode,
                       startBackwardNode)) {
      return result;
    }

    if (parameter.GetBreaker() &&
        parameter.GetBreaker()->IsAborted()) {
      return result;
    }

    if (startForwardNode) {
      std::pair<OpenListRef,bool> result=openList.insert(startForwardNode);

      openMap[startForwardNode->nodeOffset]=result.first;
    }

    if (startBackwardNode) {
      std::pair<OpenListRef,bool> result=openList.insert(startBackwardNode);

      openMap[startBackwardNode->nodeOffset]=result.first;
    }


    double currentMaxDistance=0.0;
    double overallDistance=GetSphericalDistance(startCoord,
                                                targetCoord);
    double overallCost=profile.GetCosts(overallDistance);
    double costLimit=profile.GetCosts(profile.GetCostLimitDistance())+overallCost*profile.GetCostLimitFactor();

    result.SetOverallDistance(overallDistance);
    result.SetCurrentMaxDistance(currentMaxDistance);

    StopClock    clock;
    RNodeRef     current;
    RouteNodeRef currentRouteNode;

    do {
      //
      // Take entry from open list with lowest cost
      //

      if (parameter.GetBreaker() &&
          parameter.GetBreaker()->IsAborted()) {
        return result;
      }

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

        if (!profile.CanUse(*currentRouteNode,objectVariantDataFile.GetData(),i)) {
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

        if (closedSet.find(VNode(path.offset))!=closedSet.end()) {
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
                currentRouteNode->objects[exclude.targetIndex].object==currentRouteNode->objects[path.objectIndex].object) {
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
                           profile.GetCosts(*currentRouteNode,objectVariantDataFile.GetData(),i);

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
        else if (!routeNodeDataFile.GetByOffset(path.offset,
                                                nextNode)) {
          log.Error() << "Cannot load route node with id " << path.offset;
          return result;
        }

        double distanceToTarget=GetSphericalDistance(nextNode->GetCoord(),
                                                     targetCoord);

        currentMaxDistance=std::max(currentMaxDistance,overallDistance-distanceToTarget);
        result.SetCurrentMaxDistance(currentMaxDistance);

        // Estimate costs for the rest of the distance to the target
        double estimateCost=profile.GetCosts(distanceToTarget);
        double overallCost=currentCost+estimateCost;

        if (overallCost>costLimit) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
              std::cout << " to " << path.offset;
              std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetTypeName() << " " << currentRouteNode->objects[path.objectIndex].object.GetFileOffset() << ")";
              std::cout << " => cost limit reached (" << overallCost << ">" << costLimit << ")" << std::endl;
#endif

          nodesIgnoredCount++;
          i++;

          continue;
        }

        if (parameter.GetProgress()) {
          parameter.GetProgress()->Progress(currentMaxDistance,overallDistance);
        }

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
          std::cout << "  Updating route " << current->nodeOffset << " via " << node->object.GetTypeName() << " " << node->object.GetFileOffset() << " " << currentCost << " " << estimateCost << " " << overallCost << " " << currentRouteNode->GetId() << std::endl;
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
          std::cout << " " << currentCost << " " << estimateCost << " " << overallCost << " " << currentRouteNode->GetId() << std::endl;
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
        closedSet.insert(VNode(current->nodeOffset,
                                    current->object,
                                    current->prev));
      }

      current->node=NULL;

      maxOpenList=std::max(maxOpenList,openMap.size());
      maxClosedSet=std::max(maxClosedSet,closedSet.size());

#if defined(DEBUG_ROUTING)
      if (openList.empty()) {
        std::cout << "No more alternatives, stopping" << std::endl;
      }

      if ((targetForwardRouteNode && current->nodeOffset==targetForwardRouteNode->GetFileOffset())) {
        std::cout << "Reached target: " << current->nodeOffset << " == " << targetForwardRouteNode->GetFileOffset() << " (forward)" << std::endl;
      }

      if (targetBackwardRouteNode && current->nodeOffset==targetBackwardRouteNode->GetFileOffset()) {
        std::cout << "Reached target: " << current->nodeOffset << " == " << targetBackwardRouteNode->GetFileOffset() << " (backward)" << std::endl;
      }
#endif
    } while (!openList.empty() &&
             (!targetForwardRouteNode || current->nodeOffset!=targetForwardRouteNode->GetFileOffset()) &&
             (!targetBackwardRouteNode || current->nodeOffset!=targetBackwardRouteNode->GetFileOffset()));

    // If we have keep the last node open because of access violations, add it
    // afte rrouting is done
    if (closedSet.find(VNode(current->nodeOffset))==closedSet.end()) {
      closedSet.insert(VNode(current->nodeOffset,
                             current->object,
                             current->prev));
    }

    clock.Stop();

    if (debugPerformance) {
      std::cout << "From:                ";
      if (startBackwardRouteNode) {
        std::cout << startBackwardRouteNode->GetCoord().GetDisplayText();
      }
      else {
        std::cout << startForwardRouteNode->GetCoord().GetDisplayText();
      }
      std::cout << " ";
      std::cout << start.GetObjectFileRef().GetTypeName() << " " << start.GetObjectFileRef().GetFileOffset();
      std::cout << "[";
      if (startBackwardRouteNode) {
        std::cout << startBackwardRouteNode->GetId() << " >* ";
      }
      std::cout << start.GetNodeIndex();
      if (startForwardRouteNode) {
        std::cout << " *> " << startForwardRouteNode->GetId();
      }
      std::cout << "]" << std::endl;

      std::cout << "To:                  ";
      if (targetBackwardRouteNode) {
        std::cout << targetBackwardRouteNode->GetCoord().GetDisplayText();
      }
      else {
        std::cout << targetForwardRouteNode->GetCoord().GetDisplayText();
      }
      std::cout << " ";
      std::cout << target.GetObjectFileRef().GetTypeName() <<  " " << target.GetObjectFileRef().GetFileOffset();
      std::cout << "[";
      if (targetForwardRouteNode) {
        std::cout << targetForwardRouteNode->GetId() << " >* ";
      }
      std::cout << target.GetNodeIndex();
      if (targetBackwardRouteNode) {
        std::cout << " *> " << targetBackwardRouteNode->GetId();
      }
      std::cout << "]" << std::endl;

      std::cout << "Time:                " << clock << std::endl;

      std::cout << "Air-line distance:   " << std::fixed << std::setprecision(1) << overallDistance << "km" << std::endl;
      std::cout << "Minimum cost:        " << overallCost << std::endl;
      std::cout << "Actual cost:         " << current->currentCost << std::endl;
      std::cout << "Cost limit:          " << costLimit << std::endl;
      std::cout << "Route nodes loaded:  " << nodesLoadedCount << std::endl;
      std::cout << "Route nodes ignored: " << nodesIgnoredCount << std::endl;
      std::cout << "Max. OpenList size:  " << maxOpenList << std::endl;
      std::cout << "Max. ClosedSet size: " << maxClosedSet << std::endl;
    }

    if (!((targetForwardRouteNode && currentRouteNode->GetId()==targetForwardRouteNode->GetId()) ||
          (targetBackwardRouteNode && currentRouteNode->GetId()==targetBackwardRouteNode->GetId()))) {
      std::cout << "No route found!" << std::endl;

      return result;
    }

    std::list<VNode> nodes;

    if (parameter.GetBreaker() &&
      parameter.GetBreaker()->IsAborted()) {
      return result;
    }

    ResolveRNodeChainToList(current->nodeOffset,
                            closedSet,
                            nodes);

    if (parameter.GetBreaker() &&
      parameter.GetBreaker()->IsAborted()) {
      return result;
    }

    if (!ResolveRNodesToRouteData(profile,
                                  nodes,
                                  start,
                                  target,
                                  result.GetRoute())) {
      //std::cerr << "Cannot convert routing result to route data" << std::endl;
      return result;
    }

    ResolveRouteDataJunctions(result.GetRoute());

    return result;
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

            points.push_back(w->GetPoint(index));
          }

          // target node of current path
          size_t index=iter->GetTargetNodeIndex();

          points.push_back(w->GetPoint(index));
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
   * @param coord
   *    coordinate of the search center
   * @param profile
   *    Routing profile to use
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
  RoutePosition RoutingService::GetClosestRoutableNode(const GeoCoord& coord,
                                                       const RoutingProfile& profile,
                                                       double radius) const
  {
    TypeConfigRef    typeConfig=database->GetTypeConfig();
    AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();
    AreaWayIndexRef  areaWayIndex=database->GetAreaWayIndex();
    AreaDataFileRef  areaDataFile=database->GetAreaDataFile();
    WayDataFileRef   wayDataFile=database->GetWayDataFile();
    RoutePosition    position;

    if (!typeConfig ||
        !areaAreaIndex ||
        !areaWayIndex ||
        !areaDataFile ||
        !wayDataFile) {
      log.Error() << "At least one index file is invalid!";
      return position;
    }

    GeoBox      boundingBox=GeoBox::BoxByCenterAndRadius(coord,radius);
    TypeInfoSet wayRoutableTypes;
    TypeInfoSet areaRoutableTypes;
    TypeInfoSet wayLoadedTypes;
    TypeInfoSet areaLoadedTypes;

    for (const auto& type : database->GetTypeConfig()->GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanRoute(profile.GetVehicle())) {
        if (type->CanBeWay()) {
          wayRoutableTypes.Set(type);
        }

        if (type->CanBeArea()) {
          // TODO: Currently disabled, since router cannot handle areas as start or target node
          //areaRoutableTypes.Set(type);
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
      return position;
    }

    if (!areaDataFile->GetByBlockSpans(wayAreaSpans,
                                       areas)) {
      log.Error() << "Error reading areas in area!";
      return position;
    }

    double minDistance=std::numeric_limits<double>::max();

    for (const auto& area : areas) {
      if (!profile.CanUse(*area)) {
        continue;
      }

      if (!HasNodeWithId(area->rings[0].nodes)) {
        continue;
      }

      for (size_t i=0; i<area->rings[0].nodes.size(); i++) {
        double distance=sqrt((area->rings[0].nodes[i].GetLat()-coord.GetLat())*(area->rings[0].nodes[i].GetLat()-coord.GetLat())+
                             (area->rings[0].nodes[i].GetLon()-coord.GetLon())*(area->rings[0].nodes[i].GetLon()-coord.GetLon()));

        if (distance<minDistance) {
          minDistance=distance;

          position=RoutePosition(area->GetObjectFileRef(),i);
        }
      }
    }

    for (const auto& way : ways) {
      if (!profile.CanUse(*way)) {
        continue;
      }

      if (!HasNodeWithId(way->nodes)) {
        continue;
      }

      for (size_t i=0;  i<way->nodes.size(); i++) {
        double distance=sqrt((way->nodes[i].GetLat()-coord.GetLat())*(way->nodes[i].GetLat()-coord.GetLat())+
                             (way->nodes[i].GetLon()-coord.GetLon())*(way->nodes[i].GetLon()-coord.GetLon()));
        if (distance<minDistance) {
          minDistance=distance;

          position=RoutePosition(way->GetObjectFileRef(),i);
        }
      }
    }

    return position;
  }
}
