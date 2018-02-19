/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings
  Copyright (C) 2017  Lukas Karas

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

#include <algorithm>

#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/AbstractRoutingService.h>
#include <osmscout/Pixel.h>
#include <osmscout/ObjectRef.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <iomanip>
#include <iostream>

//#define DEBUG_ROUTING

namespace osmscout {

  template <class RoutingState>
  AbstractRoutingService<RoutingState>::AbstractRoutingService(const RouterParameter& parameter):
    debugPerformance(parameter.IsDebugPerformance())
  {
  }

  template <class RoutingState>
  AbstractRoutingService<RoutingState>::~AbstractRoutingService()
  {
  }

  template <class RoutingState>
  void AbstractRoutingService<RoutingState>::ResolveRNodeChainToList(DBFileOffset finalRouteNode,
                                                                     const ClosedSet& closedSet,
                                                                     const ClosedSet& closedRestrictedSet,
                                                                     std::list<VNode>& nodes)
  {
    bool restricted=false;
    auto current=closedSet.find(VNode(finalRouteNode));

    if (current==closedSet.end()){
      current=closedRestrictedSet.find(VNode(finalRouteNode));
      assert(current!=closedSet.end());
      restricted=true;
    }

    while (current->previousNode.IsValid()) {
#if defined(DEBUG_ROUTING)
      std::cout << "Chain item " << current->currentNode << " -> " << current->previousNode << std::endl;
#endif
      ClosedSet::const_iterator prev;
      if (!restricted){
        prev=closedSet.find(VNode(current->previousNode));
        if (prev==closedSet.end()){
          prev=closedRestrictedSet.find(VNode(current->previousNode));
          assert(prev!=closedRestrictedSet.end());
          restricted=true;
        }
      }else{
        prev=closedRestrictedSet.find(VNode(current->previousNode));
        if (prev==closedRestrictedSet.end()){
          prev=closedSet.find(VNode(current->previousNode));
          assert(prev!=closedSet.end());
          restricted=false;
        }
      }


      nodes.push_back(*current);

      current=prev;
    }

    nodes.push_back(*current);

    std::reverse(nodes.begin(),nodes.end());
  }

  template <class RoutingState>
  void AbstractRoutingService<RoutingState>::GetStartForwardRouteNode(const RoutingState& state,
                                                                      const DatabaseId& database,
                                                                      const WayRef& way,
                                                                      size_t nodeIndex,
                                                                      RouteNodeRef& routeNode,
                                                                      size_t& routeNodeIndex)
  {
    routeNode=nullptr;

    if (!CanUseForward(state,database,way)) {
      return;
    }

    // TODO: What if the way is a roundabout?

    for (size_t i=nodeIndex; i<way->nodes.size(); i++) {
      GetRouteNode(database,
                   way->GetId(i),
                   routeNode);

      if (routeNode) {
        routeNodeIndex=i;
        return;
      }
    }
  }

  template <class RoutingState>
  void AbstractRoutingService<RoutingState>::GetStartBackwardRouteNode(const RoutingState& state,
                                                                       const DatabaseId& database,
                                                                       const WayRef& way,
                                                                       size_t nodeIndex,
                                                                       RouteNodeRef& routeNode,
                                                                       size_t& routeNodeIndex)
  {
    routeNode=nullptr;

    if (nodeIndex>=way->nodes.size()) {
      return;
    }

    if (!CanUseBackward(state,database,way)) {
      return;
    }

    for (long i=(long)nodeIndex-1; i>=0; i--) {
      GetRouteNode(database,
                   way->GetId((size_t)i),
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
  template <class RoutingState>
  void AbstractRoutingService<RoutingState>::GetTargetForwardRouteNode(const RoutingState& state,
                                                                       const DatabaseId& database,
                                                                       const WayRef& way,
                                                                       size_t nodeIndex,
                                                                       RouteNodeRef& routeNode)
  {
    routeNode=nullptr;

    if (nodeIndex>=way->nodes.size()) {
      return;
    }

    if (!CanUseForward(state,database,way)) {
      return;
    }

    for (long i=(long)nodeIndex-1; i>=0; i--) {
      GetRouteNode(database,
                   way->GetId((size_t)i),
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
  template <class RoutingState>
  void AbstractRoutingService<RoutingState>::GetTargetBackwardRouteNode(const RoutingState& state,
                                                                        const DatabaseId& database,
                                                                        const WayRef& way,
                                                                        size_t nodeIndex,
                                                                        RouteNodeRef& routeNode)
  {
    routeNode=nullptr;

    if (!CanUseBackward(state,database,way)) {
      return;
    }

    // TODO: What if the way is a roundabout?

    for (size_t i=nodeIndex; i<way->nodes.size(); i++) {
      GetRouteNode(database,
                   way->GetId(i),
                   routeNode);


      if (routeNode) {
        return;
      }
    }
  }

  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::GetRNode(const RoutingState& state,
                                                      const RoutePosition& position,
                                                      const WayRef& way,
                                                      size_t routeNodeIndex,
                                                      const RouteNodeRef& routeNode,
                                                      const GeoCoord& startCoord,
                                                      const GeoCoord& targetCoord,
                                                      RNodeRef& node)
  {
    node=std::make_shared<RNode>(DBFileOffset(position.GetDatabaseId(),routeNode->GetFileOffset()),
                                 routeNode,
                                 position.GetObjectFileRef());

    node->currentCost=GetCosts(state,
                               position.GetDatabaseId(),
                               way,
                               GetSphericalDistance(startCoord,
                                                    way->nodes[routeNodeIndex].GetCoord()));
    node->estimateCost=GetEstimateCosts(state,
                                        position.GetDatabaseId(),
                                        GetSphericalDistance(way->nodes[routeNodeIndex].GetCoord(),
                                                             targetCoord));

    node->overallCost=node->currentCost+node->estimateCost;

    return true;
  }

  /**
   * The start position is at the given way and the index of the node within
   * the object. Return the closest route node and routing node either in the
   * forward or backward direction or both.
   *
   * @param state
   *    The routing state
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
  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::GetWayStartNodes(const RoutingState& state,
                                                              const RoutePosition& position,
                                                              GeoCoord& startCoord,
                                                              const GeoCoord& targetCoord,
                                                              RouteNodeRef& forwardRouteNode,
                                                              RouteNodeRef& backwardRouteNode,
                                                              RNodeRef& forwardRNode,
                                                              RNodeRef& backwardRNode)
  {
    assert(position.GetObjectFileRef().GetType()==refWay);

    WayRef     way;
    size_t     forwardNodePos;
    size_t     backwardNodePos;

    if (!GetWayByOffset(DBFileOffset(position.GetDatabaseId(),
                                     position.GetObjectFileRef().GetFileOffset()),
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
    GetRouteNode(position.GetDatabaseId(),
                 way->GetId(position.GetNodeIndex()),
                 forwardRouteNode);

    if (forwardRouteNode) {
      forwardNodePos=position.GetNodeIndex();
    }
    else {
      GetStartForwardRouteNode(state,
                               position.GetDatabaseId(),
                               way,
                               position.GetNodeIndex(),
                               forwardRouteNode,
                               forwardNodePos);

      GetStartBackwardRouteNode(state,
                                position.GetDatabaseId(),
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
        !GetRNode(state,
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
        !GetRNode(state,
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
   * @param state
   *    The routing state
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
  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::GetStartNodes(const RoutingState& state,
                                                           const RoutePosition& position,
                                                           GeoCoord& startCoord,
                                                           const GeoCoord& targetCoord,
                                                           RouteNodeRef& forwardRouteNode,
                                                           RouteNodeRef& backwardRouteNode,
                                                           RNodeRef& forwardRNode,
                                                           RNodeRef& backwardRNode)
  {
    if (position.GetObjectFileRef().GetType()==refWay) {
      return GetWayStartNodes(state,
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
   * @param state
   *    The routing state
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
  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::GetWayTargetNodes(const RoutingState& state,
                                                               const RoutePosition& position,
                                                               GeoCoord& targetCoord,
                                                               RouteNodeRef& forwardNode,
                                                               RouteNodeRef& backwardNode)
  {

    assert(position.GetObjectFileRef().GetType()==refWay);

    WayRef way;

    if (!GetWayByOffset(DBFileOffset(position.GetDatabaseId(),
                                     position.GetObjectFileRef().GetFileOffset()),
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
    GetRouteNode(position.GetDatabaseId(),
                 way->GetId(position.GetNodeIndex()),
                 forwardNode);

    if (!forwardNode) {
      GetTargetForwardRouteNode(state,
                                position.GetDatabaseId(),
                                way,
                                position.GetNodeIndex(),
                                forwardNode);
      GetTargetBackwardRouteNode(state,
                                 position.GetDatabaseId(),
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
   * @param state
   *    The routing state
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
  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::GetTargetNodes(const RoutingState& state,
                                                            const RoutePosition& position,
                                                            GeoCoord& targetCoord,
                                                            RouteNodeRef& forwardNode,
                                                            RouteNodeRef& backwardNode)
  {
    if (position.GetObjectFileRef().GetType()==refWay) {
      return GetWayTargetNodes(state,
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

  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::WalkToOtherDatabases(const RoutingState& state,
                                                                  RNodeRef &current,
                                                                  RouteNodeRef &currentRouteNode,
                                                                  OpenList &openList,
                                                                  OpenMap &openMap,
                                                                  const ClosedSet &closedSet,
                                                                  const ClosedSet &closedRestrictedSet)
  {
    // add twin nodes to nextNode from other databases to open list
    std::vector<DBFileOffset> twins=GetNodeTwins(state,
                                                 current->nodeOffset.database,
                                                 currentRouteNode->GetId());
    for (const auto& twin : twins) {
      if ((current->access &&
           closedSet.find(VNode(twin))!=closedSet.end()) ||
          (!current->access &&
            closedRestrictedSet.find(VNode(twin))!=closedRestrictedSet.end())){
#if defined(DEBUG_ROUTING)
        std::cout << "Twin node " << twin << " is closed already, ignore it" << std::endl;
#endif
        continue;
      }

      auto twinIt=openMap.find(twin);

      if (twinIt!=openMap.end()){
        RNodeRef rn=(*twinIt->second);
        if (rn->currentCost > current->currentCost) {
          // this is cheaper path to twin

          rn->prev=current->nodeOffset;
          //rn->object=node->objects.begin()->object, /*TODO: how to find correct way from other DB?*/

          rn->currentCost=current->currentCost;
          rn->estimateCost=current->estimateCost;
          rn->overallCost=current->overallCost;
          rn->access=current->access;

          openList.erase(twinIt->second);

          std::pair<OpenListRef,bool> insertResult=openList.insert(rn);
          twinIt->second=insertResult.first;

#if defined(DEBUG_ROUTING)
          std::cout << "Better transition from " << rn->prev << " to " << rn->nodeOffset << std::endl;
#endif
        }
      }
      else {
        RouteNodeRef node;
        if (!GetRouteNodeByOffset(twin,node)){
          return false;
        }
        RNodeRef rn=std::make_shared<RNode>(twin,
                                            node,
                                            //node->objects.begin()->object, /*TODO: how to find correct way from other DB?*/
                                            ObjectFileRef(), // TODO: have to be valid Object here?
                                            /*prev*/current->nodeOffset);

        rn->currentCost=current->currentCost;
        rn->estimateCost=current->estimateCost;
        rn->overallCost=current->overallCost;
        rn->access=current->access;

        std::pair<OpenListRef,bool> insertResult=openList.insert(rn);
        openMap[rn->nodeOffset]=insertResult.first;

#if defined(DEBUG_ROUTING)
        std::cout << "Transition from " << rn->prev << " to " << rn->nodeOffset << std::endl;
#endif
      }
    }
    return true;
  }

  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::WalkPaths(const RoutingState &state,
                                                       RNodeRef &current,
                                                       RouteNodeRef &currentRouteNode,
                                                       OpenList &openList,
                                                       OpenMap &openMap,
                                                       ClosedSet &closedSet,
                                                       ClosedSet &closedRestrictedSet,
                                                       RoutingResult &result,
                                                       const RoutingParameter& parameter,
                                                       const GeoCoord &targetCoord,
                                                       const Vehicle &vehicle,
                                                       size_t &nodesIgnoredCount,
                                                       double &currentMaxDistance,
                                                       const double &overallDistance,
                                                       const double &costLimit)
  {
    DatabaseId dbId=current->nodeOffset.database;
    size_t i=0;
    for (const auto& path : currentRouteNode->paths) {
      if (path.offset==current->prev.offset) {
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
        continue;
      }

      if (!CanUse(state,
                  dbId,
                  *currentRouteNode,
                  /*pathIndex*/ i)) {
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

      if ((current->access &&
           closedSet.find(VNode(DBFileOffset(dbId,path.offset)))!=closedSet.end()) ||
          (!current->access &&
           closedRestrictedSet.find(VNode(DBFileOffset(dbId,path.offset)))!=closedRestrictedSet.end())) {
#if defined(DEBUG_ROUTING)
        std::cout << "  Skipping route";
        std::cout << " to " << dbId << " / " << path.offset;
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
            std::cout << " to " << dbId << " / " << path.offset;
            std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetName() << ")";
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

      double currentCost=current->currentCost+GetCosts(state,dbId,*currentRouteNode,i);

      auto openEntry=openMap.find(DBFileOffset(current->nodeOffset.database,
                                               path.offset));

      // Check, if we already have a cheaper path to the new node. If yes, do not put the new path
      // into the open list
      if (openEntry!=openMap.end() &&
          (*openEntry->second)->currentCost<=currentCost) {
#if defined(DEBUG_ROUTING)
        std::cout << "  Skipping route";
        std::cout << " to " << dbId << " / " << path.offset;
        std::cout << " (" << currentRouteNode->objects[path.objectIndex].object.GetName() << ")";
        std::cout << " => cheaper route exists " << currentCost << "<=>" << (*openEntry->second)->object.GetName() << " " << (*openEntry->second)->node->GetId() << " " << (*openEntry->second)->currentCost << std::endl;
#endif
        i++;

        continue;
      }

      RouteNodeRef nextNode;

      if (openEntry!=openMap.end()) {
        nextNode=(*openEntry->second)->node;
      }
      else if (!GetRouteNodeByOffset(DBFileOffset(current->nodeOffset.database,
                                                  path.offset),
                                     nextNode)) {
        log.Error() << "Cannot load route node with id " << path.offset;
        return false;
      }

      double distanceToTarget=GetSphericalDistance(nextNode->GetCoord(),
                                                   targetCoord);

      currentMaxDistance=std::max(currentMaxDistance,overallDistance-distanceToTarget);
      result.SetCurrentMaxDistance(currentMaxDistance);

      // Estimate costs for the rest of the distance to the target
      double estimateCost=GetEstimateCosts(state,dbId,distanceToTarget);
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

      // If we already have the node in the open list, but the new path is cheaper (as tested above),
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

        std::pair<OpenListRef,bool> insertResult=openList.insert(node);
        openEntry->second=insertResult.first;
      }
      else {
        RNodeRef node=std::make_shared<RNode>(DBFileOffset(dbId,path.offset),
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

        std::pair<OpenListRef,bool> insertResult=openList.insert(node);
        openMap[node->nodeOffset]=insertResult.first;
      }

      i++;
    }

    return true;
  }

  /**
   * Calculate a route
   *
   * @param state
   *    State to use
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
  template <class RoutingState>
  RoutingResult AbstractRoutingService<RoutingState>::CalculateRoute(RoutingState& state,
                                                                     const RoutePosition& start,
                                                                     const RoutePosition& target,
                                                                     const RoutingParameter& parameter)
  {
    RoutingResult            result;
    Vehicle                  vehicle=GetVehicle(state);
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

    // Restricted way (access=destination) is a way that may be used just
    // in case when target is on this way. Some routing nodes may be accessed
    // from two different ways - one without any access restriction (closedSet)
    // and second with restriction (closedRestrictedSet)
    ClosedSet                closedSet;
    ClosedSet                closedRestrictedSet;

    size_t                   nodesLoadedCount=0;
    size_t                   nodesIgnoredCount=0;
    size_t                   maxOpenList=0;
    size_t                   maxClosedSet=0;

    openMap.reserve(10000);
    closedSet.reserve(300000);
    closedRestrictedSet.reserve(10000);

    if (!GetTargetNodes(state,
                        target,
                        targetCoord,
                        targetForwardRouteNode,
                        targetBackwardRouteNode)) {
      return result;
    }

    if (!GetStartNodes(state,
                       start,
                       startCoord,
                       targetCoord,
                       startForwardRouteNode,
                       startBackwardRouteNode,
                       startForwardNode,
                       startBackwardNode)) {
      return result;
    }

#if defined(DEBUG_ROUTING)
    if (startForwardNode) {
      std::cout << "StartForwardNode:   " << start.GetObjectFileRef().GetName() << " " << startForwardRouteNode->GetId() << " "
                << startForwardNode->currentCost << " " << startForwardNode->estimateCost << " " << startForwardNode->overallCost
                << std::endl;
    }
    if (startBackwardNode) {
      std::cout << "StartBackwardNode:  " << start.GetObjectFileRef().GetName() << " " << startBackwardRouteNode->GetId() << " "
                << startBackwardNode->currentCost << " " << startBackwardNode->estimateCost << " " << startBackwardNode->overallCost
                << std::endl;
    }
    if (targetForwardRouteNode) {
      std::cout << "TargetForwardNode:  " << target.GetObjectFileRef().GetName() << " " << targetForwardRouteNode->GetId() << std::endl;
    }
    if (startBackwardNode) {
      std::cout << "TargetBackwardNode: " << target.GetObjectFileRef().GetName() << " " << targetBackwardRouteNode->GetId() << std::endl;
    }
#endif

    if (parameter.GetBreaker() &&
        parameter.GetBreaker()->IsAborted()) {
      return result;
    }

    if (startForwardNode) {
      std::pair<OpenListRef,bool> insertResult=openList.insert(startForwardNode);

      openMap[startForwardNode->nodeOffset]=insertResult.first;
    }

    if (startBackwardNode) {
      std::pair<OpenListRef,bool> insertResult=openList.insert(startBackwardNode);

      openMap[startBackwardNode->nodeOffset]=insertResult.first;
    }


    double currentMaxDistance=0.0;
    double overallDistance=GetSphericalDistance(startCoord,
                                                targetCoord);
    double overallCost=GetEstimateCosts(state,start.GetDatabaseId(),overallDistance);
    double costLimit=GetCostLimit(state,start.GetDatabaseId(),overallDistance);

    result.SetOverallDistance(overallDistance);
    result.SetCurrentMaxDistance(currentMaxDistance);

    StopClock    clock;
    RNodeRef     current;
    RouteNodeRef currentRouteNode;
    DatabaseId   dbId;
    bool         targetForwardFound=targetForwardRouteNode ? false : true;
    bool         targetBackwardFound=targetBackwardRouteNode ? false : true;
    RNodeRef     targetForwardFinalNode;
    RNodeRef     targetBackwardFinalNode;

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
      dbId=current->nodeOffset.database;

      nodesLoadedCount++;

      // Get potential follower in the current way

#if defined(DEBUG_ROUTING)
      std::cout << "Analysing follower of node " << dbId << " / " << currentRouteNode->GetFileOffset();
      std::cout << " (" << current->object.GetName() << "["  << currentRouteNode->GetId() << "]" << ")";
      std::cout << " " << current->currentCost << " " << current->estimateCost << " " << current->overallCost << std::endl;
#endif

      if (!WalkPaths(state,
                     current,
                     currentRouteNode,
                     openList,
                     openMap,
                     closedSet,
                     closedRestrictedSet,
                     result,
                     parameter,
                     targetCoord,
                     vehicle,
                     nodesIgnoredCount,
                     currentMaxDistance,
                     overallDistance,
                     costLimit)){

        log.Error() << "Failed to walk paths from " << dbId << " / " << currentRouteNode->GetFileOffset();
        return result;
      }

      //
      // Add current node twins (nodes from another databases with same Id)
      // to openList/openMap or update it
      //

      if (!WalkToOtherDatabases(state,
                                current,
                                currentRouteNode,
                                openList,
                                openMap,
                                closedSet,
                                closedRestrictedSet)) {
        log.Error() << "Failed to walk to other databases from " << dbId << " / " << currentRouteNode->GetFileOffset();
        return result;
      }

      //
      // Add current node to close map
      //

#if defined(DEBUG_ROUTING)
        std::cout << "Closing " << current->nodeOffset << " (previous " << current->prev << ")" << std::endl;
#endif
      if (current->access) {
        closedSet.insert(VNode(current->nodeOffset,
                               current->object,
                               current->prev));
      }
      else {
        closedRestrictedSet.insert(VNode(current->nodeOffset,
                                         current->object,
                                         current->prev));
      }

      current->node=nullptr;

      maxOpenList=std::max(maxOpenList,openMap.size());
      maxClosedSet=std::max(maxClosedSet,closedSet.size()+closedRestrictedSet.size());

#if defined(DEBUG_ROUTING)
      if (openList.empty()) {
        std::cout << "No more alternatives, stopping" << std::endl;
      }

      if (targetForwardRouteNode &&
          current->nodeOffset.offset==targetForwardRouteNode->GetFileOffset() &&
          current->nodeOffset.database==target.GetDatabaseId()) {
        std::cout << "Reached target: " << current->nodeOffset << " == " << targetForwardRouteNode->GetFileOffset() << " (forward)" << std::endl;
      }

      if (targetBackwardRouteNode &&
          current->nodeOffset.offset==targetBackwardRouteNode->GetFileOffset() &&
          current->nodeOffset.database==target.GetDatabaseId()) {
        std::cout << "Reached target: " << current->nodeOffset << " == " << targetBackwardRouteNode->GetFileOffset() << " (backward)" << std::endl;
      }
#endif

      if (!targetForwardFound) {
        targetForwardFound=current->nodeOffset.offset==targetForwardRouteNode->GetFileOffset() &&
                           current->nodeOffset.database==target.GetDatabaseId();
        if (targetForwardFound) {
          targetForwardFinalNode=current;
        }
      }

      if (!targetBackwardFound) {
        targetBackwardFound=current->nodeOffset.offset==targetBackwardRouteNode->GetFileOffset() &&
                            current->nodeOffset.database==target.GetDatabaseId();
        if (targetBackwardFound) {
          targetBackwardFinalNode=current;
        }
      }

    } while (!openList.empty() && !(targetForwardFound && targetBackwardFound));

    // If we have keep the last node open because of access violations, add it
    // after routing is done
    if (closedSet.find(VNode(current->nodeOffset))==closedSet.end()) {
      closedSet.insert(VNode(current->nodeOffset,
                             current->object,
                             current->prev));
    }
    RNodeRef  targetFinalNode;

    if (targetBackwardFinalNode && targetForwardFinalNode) {
      if (targetForwardFinalNode->currentCost<=targetBackwardFinalNode->currentCost) {
        targetFinalNode=targetForwardFinalNode;
      }
      else {
        targetFinalNode=targetBackwardFinalNode;
      }
    }
    else if (targetBackwardFinalNode) {
      targetFinalNode=targetBackwardFinalNode;
    }
    else if (targetForwardFinalNode) {
      targetFinalNode=targetForwardFinalNode;
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
      std::cout << start.GetObjectFileRef().GetName();
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
      std::cout << target.GetObjectFileRef().GetName();
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
      if (targetFinalNode) {
        std::cout << "Actual cost:         " << targetFinalNode->currentCost << std::endl;
      }
      std::cout << "Cost limit:          " << costLimit << std::endl;
      std::cout << "Route nodes loaded:  " << nodesLoadedCount << std::endl;
      std::cout << "Route nodes ignored: " << nodesIgnoredCount << std::endl;
      std::cout << "Max. OpenList size:  " << maxOpenList << std::endl;
      std::cout << "Max. ClosedSet size: " << maxClosedSet << std::endl;
    }

    if (!targetFinalNode) {
      log.Warn() << "No route found!";

      return result;
    }

    std::list<VNode> nodes;

    if (parameter.GetBreaker() &&
      parameter.GetBreaker()->IsAborted()) {
      return result;
    }

    ResolveRNodeChainToList(targetFinalNode->nodeOffset,
                            closedSet,
                            closedRestrictedSet,
                            nodes);

#if defined(DEBUG_ROUTING)
    std::cout << "VNode List:" << std::endl;
    for (const auto& node : nodes) {
      std::cout << node.object.GetName() << " " << node.currentNode.database << "/" << node.currentNode.offset << std::endl;
    }
#endif

    if (parameter.GetBreaker() &&
      parameter.GetBreaker()->IsAborted()) {
      return result;
    }

    if (!ResolveRNodesToRouteData(state,
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

  template <class RoutingState>
  void AbstractRoutingService<RoutingState>::AddNodes(RouteData& route,
                                                      DatabaseId database,
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
      route.AddEntry(database,
                     startNodeId,
                     startNodeIndex,
                     object,
                     targetNodeIndex);
    }
    else if (startNodeIndex<targetNodeIndex) {
      // Following the way
      route.AddEntry(database,
                     startNodeId,
                     startNodeIndex,
                     object,
                     startNodeIndex+1);

      for (size_t i=startNodeIndex+1; i<targetNodeIndex-1; i++) {
        route.AddEntry(database,
                       0,
                       i,
                       object,
                       i+1);
      }

      route.AddEntry(database,
                     0,
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

      route.AddEntry(database,
                     startNodeId,
                     startNodeIndex,
                     object,
                     pos);

      while (next!=targetNodeIndex) {
        route.AddEntry(database,
                       0,
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

      route.AddEntry(database,
                     0,
                     pos,
                     object,
                     targetNodeIndex);
    }
    else {
      // We follow the way in the opposite direction
      route.AddEntry(database,
                     startNodeId,
                     startNodeIndex,
                     object,
                     startNodeIndex-1);

      for (long i=(long)startNodeIndex-1; i>(long)targetNodeIndex+1; i--) {
        route.AddEntry(database,
                       0,
                       (size_t)i,
                       object,
                       (size_t)i-1);
      }

      route.AddEntry(database,
                     0,
                     targetNodeIndex+1,
                     object,
                     targetNodeIndex);
    }
  }

  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::ResolveRNodesToRouteData(const RoutingState& state,
                                                                      const std::list<VNode>& nodes,
                                                                      const RoutePosition& start,
                                                                      const RoutePosition& target,
                                                                      RouteData& route)
  {
    std::set<DBFileOffset>                        routeNodeOffsets;
    std::set<DBFileOffset>                        wayOffsets;
    std::set<DBFileOffset>                        areaOffsets;

    std::unordered_map<DBFileOffset,RouteNodeRef> routeNodeMap;
    std::unordered_map<DBFileOffset,AreaRef>      areaMap;
    std::unordered_map<DBFileOffset,WayRef>       wayMap;

    std::vector<Point>                            *ids=nullptr;
    bool                                          oneway=false;

    // Collect all route node file offsets on the path and also
    // all area/way file offsets on the path
    for (const auto& node : nodes) {
      routeNodeOffsets.insert(node.currentNode);
      DatabaseId dbId=node.currentNode.database;

      if (node.object.Valid()) {
        switch (node.object.GetType()) {
        case refArea:
          areaOffsets.insert(DBFileOffset(dbId,node.object.GetFileOffset()));
          break;
        case refWay:
          wayOffsets.insert(DBFileOffset(dbId,node.object.GetFileOffset()));
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
      areaOffsets.insert(DBFileOffset(target.GetDatabaseId(),
                                      target.GetObjectFileRef().GetFileOffset()));
      break;
    case refWay:
      wayOffsets.insert(DBFileOffset(target.GetDatabaseId(),
                                     target.GetObjectFileRef().GetFileOffset()));
      break;
    default:
      assert(false);
      break;
    }

    //
    // Load data
    //

    if (!GetRouteNodesByOffset(routeNodeOffsets,
                               routeNodeMap)) {
      log.Error() << "Cannot load route nodes";
      return false;
    }

    if (!GetAreasByOffset(areaOffsets,
                          areaMap)) {
      log.Error() << "Cannot load areas";
      return false;
    }

    if (!GetWaysByOffset(wayOffsets,
                         wayMap)) {
      log.Error() << "Cannot load ways";
      return false;
    }

    assert(start.GetObjectFileRef().GetType()==refArea ||
           start.GetObjectFileRef().GetType()==refWay);

    if (start.GetObjectFileRef().GetType()==refArea) {
      std::unordered_map<DBFileOffset,AreaRef>::const_iterator entry=areaMap
        .find(DBFileOffset(start.GetDatabaseId(),start.GetObjectFileRef().GetFileOffset()));

      assert(entry!=areaMap.end());

      ids=&entry->second->rings.front().nodes;
      oneway=false;
    }
    else if (start.GetObjectFileRef().GetType()==refWay) {
      std::unordered_map<DBFileOffset,WayRef>::const_iterator entry=wayMap
        .find(DBFileOffset(start.GetDatabaseId(),start.GetObjectFileRef().GetFileOffset()));

      assert(entry!=wayMap.end());

      ids=&entry->second->nodes;
      oneway=!CanUseBackward(state,entry->first.database,entry->second);
    }

    if (nodes.empty()) {
      // We assume that startNode and targetNode are on the same area/way (with no routing node in between)
      assert(start.GetObjectFileRef()==target.GetObjectFileRef());

      AddNodes(route,
               start.GetDatabaseId(),
               (*ids)[start.GetNodeIndex()].GetId(),
               start.GetNodeIndex(),
               start.GetObjectFileRef(),
               ids->size(),
               oneway,
               target.GetNodeIndex());

      route.AddEntry(target.GetDatabaseId(),
                     0,
                     target.GetNodeIndex(),
                     ObjectFileRef(),
                     0);
      return true;
    }

    auto entry=routeNodeMap.find(nodes.front().currentNode);
    if (entry==routeNodeMap.end()){
      log.Error() << "Can't found route node " << nodes.front().currentNode.database <<
        ", " << nodes.front().currentNode.offset;
      return false;
    }
    RouteNodeRef initialNode=entry->second;

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
               start.GetDatabaseId(),
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
    for (auto n=nodes.begin();
        n!=nodes.end();
        n++) {
      auto nn=n;

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
          std::unordered_map<DBFileOffset,AreaRef>::const_iterator areaEntry=areaMap
            .find(DBFileOffset(target.GetDatabaseId(),target.GetObjectFileRef().GetFileOffset()));

          assert(areaEntry!=areaMap.end());

          ids=&areaEntry->second->rings.front().nodes;
          oneway=false;
        }
        else if (target.GetObjectFileRef().GetType()==refWay) {
          std::unordered_map<DBFileOffset,WayRef>::const_iterator wayEntry=wayMap
            .find(DBFileOffset(target.GetDatabaseId(),target.GetObjectFileRef().GetFileOffset()));

          assert(wayEntry!=wayMap.end());

          ids=&wayEntry->second->nodes;
          oneway=!CanUseBackward(state,wayEntry->first.database,wayEntry->second);
        }

        size_t currentNodeIndex=0;

        while ((*ids)[currentNodeIndex].GetId()!=node->GetId() &&
            currentNodeIndex<ids->size()) {
          currentNodeIndex++;
        }
        assert(currentNodeIndex<ids->size());

        if (currentNodeIndex!=target.GetNodeIndex()) {
          AddNodes(route,
                   target.GetDatabaseId(),
                   (*ids)[currentNodeIndex].GetId(),
                   currentNodeIndex,
                   target.GetObjectFileRef(),
                   ids->size(),
                   oneway,
                   target.GetNodeIndex());
        }

        route.AddEntry(target.GetDatabaseId(),
                       0,
                       target.GetNodeIndex(),
                       ObjectFileRef(),
                       0);

        break;
      }

      RouteNodeRef nextNode=routeNodeMap.find(nn->currentNode)->second;

      if (n->currentNode.database!=nn->currentNode.database &&
          node->GetId()==nextNode->GetId()){
        // there is no way between database transition nodes
        continue;
      }

      assert(nn->object.GetType()==refArea ||
             nn->object.GetType()==refWay);

      if (nn->object.GetType()==refArea) {
        std::unordered_map<DBFileOffset,AreaRef>::const_iterator areaEntry=areaMap
          .find(DBFileOffset(nn->currentNode.database,nn->object.GetFileOffset()));

        assert(areaEntry!=areaMap.end());

        ids=&areaEntry->second->rings.front().nodes;
        oneway=false;
      }
      else if (nn->object.GetType()==refWay) {
        std::unordered_map<DBFileOffset,WayRef>::const_iterator wayEntry=wayMap
          .find(DBFileOffset(nn->currentNode.database,nn->object.GetFileOffset()));

        assert(wayEntry!=wayMap.end());

        ids=&wayEntry->second->nodes;
        oneway=!CanUseBackward(state,wayEntry->first.database,wayEntry->second);
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
               nn->currentNode.database,
               (*ids)[currentNodeIndex].GetId(),
               currentNodeIndex,
               nn->object,
               ids->size(),
               oneway,
               nextNodeIndex);
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
  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::TransformRouteDataToRouteDescription(const RouteData& data,
                                                                                  RouteDescription& description)
  {
    description.Clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (const auto& entry : data.Entries()) {
      description.AddNode(entry.GetDatabaseId(),
                          entry.GetCurrentNodeIndex(),
                          entry.GetObjects(),
                          entry.GetPathObject(),
                          entry.GetTargetNodeIndex());
    }

    return true;
  }

  /**
   * Transforms the route into a Way (with empty type)
   * @param data
   *    Route data
   * @param way
   *    Way to get initialized to the route on success
   * @return
   *    True, if the way could be build, else false
   */
  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::TransformRouteDataToWay(const RouteData& data,
                                                                     Way& way)
  {
    std::list<Point> points;
    if (!TransformRouteDataToPoints(data,points)){
      return false;
    }
    way.nodes.clear();
    way.SetLayerToMax();
    way.nodes.reserve(data.Entries().size());
    for (const auto &p:points){
      way.nodes.push_back(p);
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
  template <class RoutingState>
  bool AbstractRoutingService<RoutingState>::TransformRouteDataToPoints(const RouteData& data,
                                                                        std::list<Point>& points)

  {
    AreaRef       a;
    DBFileOffset  aId;

    WayRef        w;
    DBFileOffset  wId;

    points.clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (auto iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      if (iter->GetPathObject().Valid()) {
        if (iter->GetPathObject().GetType()==refArea) {
          if (!a ||
              aId!=iter->GetDBFileOffset()) {
            if (!GetAreaByOffset(iter->GetDBFileOffset(),a)) {
              log.Error() << "Cannot load area with id " << iter->GetPathObject().GetFileOffset();
              return false;
            }
            aId=iter->GetDBFileOffset();
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
              wId!=iter->GetDBFileOffset()) {
            if (!GetWayByOffset(iter->GetDBFileOffset(),w)) {
              log.Error() << "Cannot load way with id " << iter->GetPathObject().GetFileOffset();
              return false;
            }
            wId=iter->GetDBFileOffset();
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

  template class AbstractRoutingService<RoutingProfile>;
  template class AbstractRoutingService<MultiDBRoutingState>;
}
