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
#include <iomanip>
#include <iostream>
#include <iterator>

#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/MultiDBRoutingService.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/log/Logger.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING

namespace osmscout {

  MultiDBRoutingService::MultiDBRoutingService(const RouterParameter& parameter,
                                               const std::vector<DatabaseRef> &databases):
    AbstractRoutingService<MultiDBRoutingState>(parameter)
  {
    this->handles.resize(databases.size());

    DatabaseId id=0;
    for (const auto& db : databases) {
      handles[id].dbId=id;
      handles[id].database=db;

      id++;
    }
  }

  MultiDBRoutingService::~MultiDBRoutingService()
  {
    Close();
  }

  bool MultiDBRoutingService::Open(RoutingProfileBuilder profileBuilder)
  {
    if (handles.empty()) {
      return false;
    }

    RouterParameter routerParameter;
    routerParameter.SetDebugPerformance(debugPerformance);

    isOpen=true;
    for (auto& handle : handles) {
      SimpleRoutingServiceRef router=std::make_shared<osmscout::SimpleRoutingService>(
                                              handle.database,
                                              routerParameter,
                                              osmscout::RoutingService::DEFAULT_FILENAME_BASE);
      if (!router->Open()) {
        Close();
        return false;
      }

      handle.router=std::move(router);
      handle.profile=profileBuilder(handle.database);

      RoutingDatabaseRef routingDatabase=std::make_shared<RoutingDatabase>();
      if (!routingDatabase->Open(handle.database)) {
        Close();
        return false;
      }

      handle.routingDatabase=std::move(routingDatabase);

    }

    return true;
  }

  void MultiDBRoutingService::Close()
  {
    if (!isOpen) {
      return;
    }

    for (auto& handle : handles) {
      if (handle.routingDatabase) {
        handle.routingDatabase->Close();
        handle.routingDatabase.reset();
      }
      if (handle.router) {
        handle.router->Close();
        handle.router.reset();
      }
      handle.profile.reset();
    }

    isOpen=false;
  }

  RoutePositionResult MultiDBRoutingService::GetRoutableNode(const DatabaseId &dbId, const std::vector<ObjectFileRef> &refs)
  {
    auto handleIt = std::find_if(handles.begin(), handles.end(),
                                 [dbId](const DatabaseHandle& handle) -> bool { return handle.dbId == dbId; });
    if (handleIt == handles.end()) {
      return RoutePositionResult();
    }
    for (auto const &ref: refs) {
      RoutePositionResult res = handleIt->router->GetRoutableNode(ref, *handleIt->profile);
      if (res.IsValid()) {
        return RoutePositionResult(RoutePosition(res.GetRoutePosition().GetObjectFileRef(),
                                                 res.GetRoutePosition().GetNodeIndex(),
                                                 /*db*/ dbId),
                                   res.GetDistance());
      }
    }
    return RoutePositionResult();
  }

  RoutePositionResult MultiDBRoutingService::GetClosestRoutableNode(const GeoCoord& coord,
                                                                    const Distance &radius) const
  {
    RoutePositionResult position, closestPosition;

    for (const auto& handle : handles) {
      position=handle.router->GetClosestRoutableNode(coord,
                                                     *handle.profile,
                                                     radius);
      if (position.IsValid() && position.GetDistance() < closestPosition.GetDistance()) {
        closestPosition=RoutePositionResult(RoutePosition(position.GetRoutePosition().GetObjectFileRef(),
                                                          position.GetRoutePosition().GetNodeIndex(),
                                                          /*db*/ handle.dbId),
                                            position.GetDistance());
      }
    }

    return closestPosition;
  }

  Vehicle MultiDBRoutingService::GetVehicle(const MultiDBRoutingState& /*state*/)
  {
    assert(!handles.empty());
    return handles.begin()->profile->GetVehicle();
  }

  bool MultiDBRoutingService::CanUseForward(const MultiDBRoutingState& /*state*/,
                                            const DatabaseId& database,
                                            const WayRef& way)
  {
    assert(handles.size()>database);
    return handles[database].profile->CanUseForward(*way);
  }

  bool MultiDBRoutingService::CanUseBackward(const MultiDBRoutingState& /*state*/,
                                             const DatabaseId& database,
                                             const WayRef& way)
  {
    assert(handles.size()>database);
    return handles[database].profile->CanUseBackward(*way);
  }

  double MultiDBRoutingService::GetCosts(const MultiDBRoutingState& /*state*/,
                                         const DatabaseId databaseId,
                                         const RouteNode& routeNode,
                                         size_t inPathIndex,
                                         size_t outPathIndex)
  {
    assert(handles.size()>databaseId);
    return handles[databaseId].profile->GetCosts(routeNode,
                                                 handles[databaseId].routingDatabase->GetObjectVariantData(),
                                                 inPathIndex,
                                                 outPathIndex);
  }

  double MultiDBRoutingService::GetCosts(const MultiDBRoutingState& /*state*/,
                                         const DatabaseId database,
                                         const WayRef &way,
                                         const Distance &wayLength)
  {
    assert(handles.size()>database);
    return handles[database].profile->GetCosts(*way,wayLength);
  }

  double MultiDBRoutingService::GetEstimateCosts(const MultiDBRoutingState& /*state*/,
                                                 const DatabaseId database,
                                                 const Distance &targetDistance)
  {
    assert(handles.size()>database);
    return handles[database].profile->GetCosts(targetDistance);
  }

  double MultiDBRoutingService::GetCostLimit(const MultiDBRoutingState& /*state*/,
                                             const DatabaseId database,
                                             const Distance &targetDistance)
  {
    assert(handles.size()>database);
    RoutingProfileRef profile=handles[database].profile;
    return profile->GetCosts(profile->GetCostLimitDistance()) + profile->GetCosts(targetDistance) * profile->GetCostLimitFactor();
  }

  std::string MultiDBRoutingService::GetCostString(const MultiDBRoutingState& /*state*/,
                                                   DatabaseId database,
                                                   double cost) const
  {
    assert(handles.size()>database);
    RoutingProfileRef profile=handles[database].profile;
    return profile->GetCostString(cost);
  }


  bool MultiDBRoutingService::CanUse(const MultiDBRoutingState& /*state*/,
                                     const DatabaseId databaseId,
                                     const RouteNode& routeNode,
                                     size_t pathIndex)
  {

    return handles[databaseId].profile->CanUse(routeNode,
                                               handles[databaseId].routingDatabase->GetObjectVariantData(),
                                               pathIndex);
  }

  bool MultiDBRoutingService::GetRouteNodes(const std::set<DBId> &routeNodeIds,
                                            std::unordered_map<DBId,RouteNodeRef> &routeNodeMap)
  {
    std::unordered_map<DatabaseId,std::set<Id>> idMap;
    for (const auto &id:routeNodeIds){
      idMap[id.database].insert(id.id);
    }
    for (const auto &entry:idMap){
      std::vector<RouteNodeRef>  nodes;
      const std::set<Id>         &ids=entry.second;

      if (!handles[entry.first].routingDatabase->GetRouteNodes(ids.begin(),
                                                               ids.end(),
                                                               ids.size(),
                                                               nodes)) {
        return false;
      }
      for (const auto &node:nodes) {
        routeNodeMap[DBId(entry.first,node->GetId())]=node;
      }
    }
    return true;
  }

  bool MultiDBRoutingService::GetRouteNode(const DBId &id,
                                           RouteNodeRef &node)
  {
    return handles[id.database].routingDatabase->GetRouteNode(id.id, node);
  }

  bool MultiDBRoutingService::GetWayByOffset(const DBFileOffset &offset,
                                             WayRef &way)
  {
    return handles[offset.database].database->GetWayByOffset(offset.offset,way);
  }

  bool MultiDBRoutingService::GetWaysByOffset(const std::set<DBFileOffset> &wayOffsets,
                                              std::unordered_map<DBFileOffset,WayRef> &wayMap)
  {
    std::unordered_map<DatabaseId,std::set<FileOffset>> offsetMap;
    for (const auto &offset:wayOffsets){
      offsetMap[offset.database].insert(offset.offset);
    }
    for (const auto &entry:offsetMap){
      std::vector<WayRef> ways;
      if (!handles[entry.first].database->GetWaysByOffset(entry.second,ways)){
        return false;
      }
      for (const auto &way:ways){
        wayMap[DBFileOffset(entry.first,way->GetFileOffset())]=way;
      }
    }
    return true;
  }

  bool MultiDBRoutingService::GetAreaByOffset(const DBFileOffset &offset,
                                              AreaRef &area)
  {
    return handles[offset.database].database->GetAreaByOffset(offset.offset,area);
  }

  bool MultiDBRoutingService::GetAreasByOffset(const std::set<DBFileOffset> &areaOffsets,
                                               std::unordered_map<DBFileOffset,AreaRef> &areaMap)
  {
    std::unordered_map<DatabaseId,std::set<FileOffset>> offsetMap;
    for (const auto &offset:areaOffsets){
      offsetMap[offset.database].insert(offset.offset);
    }
    for (const auto &entry:offsetMap){
      std::vector<AreaRef> areas;
      if (!handles[entry.first].database->GetAreasByOffset(entry.second,areas)){
        return false;
      }
      for (const auto &area:areas){
        areaMap[DBFileOffset(entry.first,area->GetFileOffset())]=area;
      }
    }
    return true;
  }

  bool MultiDBRoutingService::ResolveRouteDataJunctions(RouteData& route)
  {
    for (auto const& handle : handles) {
      std::set<Id>       nodeIds;

      for (const auto& routeEntry : route.Entries()) {
        if (routeEntry.GetCurrentNodeId()!=0 && routeEntry.GetDatabaseId()==handle.dbId) {
          nodeIds.insert(routeEntry.GetCurrentNodeId());
        }
      }

      std::vector<JunctionRef> junctions;

      if (!handle.routingDatabase->GetJunctions(nodeIds,
                                           junctions)) {
        log.Error() << "Error while resolving junction ids to junctions";
        return false;
      }

      nodeIds.clear();

      std::unordered_map<Id,JunctionRef> junctionMap;

      for (const auto& junction : junctions) {
        junctionMap.emplace(junction->GetId(),junction);
      }

      junctions.clear();

      for (auto& routeEntry : route.Entries()) {
        if (routeEntry.GetCurrentNodeId()!=0 && routeEntry.GetDatabaseId()==handle.dbId) {
          auto junctionEntry=junctionMap.find(routeEntry.GetCurrentNodeId());
          if (junctionEntry!=junctionMap.end()) {
            routeEntry.SetObjects(junctionEntry->second->GetObjects());
          }
        }
      }
    }

    return true;
  }

  std::vector<DBId> MultiDBRoutingService::GetNodeTwins(const MultiDBRoutingState& /*state*/,
                                                        const DatabaseId database,
                                                        const Id id)
  {
    assert(handles.size()>database);

    std::vector<DBId> twins;

    twins.reserve(handles.size()-1);
    for (DatabaseId dbId=0; dbId<handles.size(); dbId++){
      if (dbId==database){
        continue;
      }
      if (handles[dbId].routingDatabase->ContainsNode(id)) {
        twins.emplace_back(dbId, id);
      }
    }
    return twins;
  }

  RoutingResult MultiDBRoutingService::CalculateRoute(const RoutePosition &start,
                                                      const RoutePosition &target,
                                                      const RoutingParameter &parameter)
  {
    RoutingResult result;

    if (start.GetDatabaseId()==target.GetDatabaseId()){
      DatabaseId              dbId=start.GetDatabaseId();
      SimpleRoutingServiceRef service=handles[dbId].router;

      if (!service) {
        return result;
      }

      return service->CalculateRoute(*handles[dbId].profile,
                                     start,
                                     target,
                                     parameter);
    }

    // start and target databases are different, try to find common route nodes
    if (start.GetDatabaseId()>=handles.size() ||
        !handles[start.GetDatabaseId()].database) {
      log.Error() << "Can't find start db " << start.GetDatabaseId();
      return result;
    }

    // DatabaseRef database1=handles[start.GetDatabaseId()].db;

    if (target.GetDatabaseId()>=handles.size() ||
        !handles[target.GetDatabaseId()].database) {
      log.Error() << "Can't find target db " << target.GetDatabaseId();
      return result;
    }

    MultiDBRoutingState state;
    return AbstractRoutingService<MultiDBRoutingState>::CalculateRoute(state,
                                                                       start,
                                                                       target,
                                                                       parameter);
  }

    /**
     * Calculate a route going through all the via points
     *
     * @param via
     *    A vector of via points
     * @param radius
     *    The maximum radius to search in from the search center in meter
     * @param parameter
     *    A RoutingParamater object
     * @return
     *    A RoutingResult object
     */
    RoutingResult MultiDBRoutingService::CalculateRoute(const std::vector<osmscout::GeoCoord>& via,
                                                        const Distance &radius,
                                                        const RoutingParameter& parameter)
    {
      RoutingResult              result;
      std::vector<RoutePosition> routePositions;

      assert(!via.empty());

      for (const auto& etap : via) {
        auto posRes=GetClosestRoutableNode(etap,radius);
        RoutePosition target=posRes.GetRoutePosition();

        if (!target.IsValid()) {
          return result;
        }

        routePositions.push_back(target);
      }

      RoutingResult partialResult;
      for (size_t   index=0; index<routePositions.size()-1; index++) {
        RoutePosition fromRoutePosition=routePositions[index];
        RoutePosition toRoutePosition  =routePositions[index+1];

        partialResult=CalculateRoute(fromRoutePosition,
                                     toRoutePosition,
                                     parameter);
        if (!partialResult.Success()) {
          result.GetRoute().Clear();
          result.ClearSectionLengths();
            
          return result;
        }

        /* In intermediary via points the end of the previous part is the start of the */
        /* next part, we need to remove the duplicate point in the calculated route */
        if (index<routePositions.size()-2) {
          partialResult.GetRoute().PopEntry();
        }

        result.GetRoute().Append(partialResult.GetRoute());
        result.AppendSectionLength(partialResult.GetRoute().Entries().size());
      }

      return result;
    }

  bool MultiDBRoutingService::PostProcessRouteDescription(RouteDescription &description,
                                                          const std::list<RoutePostprocessor::PostprocessorRef> &postprocessors)
  {
    std::set<std::string,std::less<>> motorwayTypeNames;
    std::set<std::string,std::less<>> motorwayLinkTypeNames;
    std::set<std::string,std::less<>> junctionTypeNames;

    junctionTypeNames.emplace("highway_motorway_junction");

    motorwayTypeNames.emplace("highway_motorway");
    motorwayLinkTypeNames.emplace("highway_motorway_link");

    motorwayTypeNames.emplace("highway_motorway_trunk");
    motorwayTypeNames.emplace("highway_trunk");

    motorwayLinkTypeNames.emplace("highway_trunk_link");
    motorwayTypeNames.emplace("highway_motorway_primary");

    RoutePostprocessor routePostprocessor;

    std::vector<RoutingProfileRef> profiles;
    std::vector<DatabaseRef>       databases;

    std::transform(handles.begin(),handles.end(),std::back_inserter(profiles),[](const DatabaseHandle& handle) {
      return handle.profile;
    });

    std::transform(handles.begin(),handles.end(),std::back_inserter(databases),[](const DatabaseHandle& handle) {
      return handle.database;
    });

    return routePostprocessor.PostprocessRouteDescription(description,
                                                          profiles,
                                                          databases,
                                                          postprocessors,
                                                          motorwayTypeNames,
                                                          motorwayLinkTypeNames,
                                                          junctionTypeNames);
  }

  std::map<DatabaseId, std::string> MultiDBRoutingService::GetDatabaseMapping() const
  {
    std::map<DatabaseId, std::string> mapping;
    for (const auto &handle:handles){
      mapping[handle.dbId]=handle.database->GetPath();
    }
    return mapping;
  }

  std::optional<DatabaseId> MultiDBRoutingService::GetDatabaseId(const std::string& databasePath) const
  {
    for (const auto &handle:handles){
      if (databasePath==handle.database->GetPath()) {
        return handle.dbId;
      }
    }
    return std::nullopt;
  }

  // FIXME: I don't understand why these methods should be here...
  RouteDescriptionResult MultiDBRoutingService::TransformRouteDataToRouteDescription(const RouteData& data)
  {
    return AbstractRoutingService<MultiDBRoutingState>::TransformRouteDataToRouteDescription(data);
  }

  RoutePointsResult MultiDBRoutingService::TransformRouteDataToPoints(const RouteData& data)
  {
    return AbstractRoutingService<MultiDBRoutingState>::TransformRouteDataToPoints(data);
  }

  RouteWayResult MultiDBRoutingService::TransformRouteDataToWay(const RouteData& data)
  {
    return AbstractRoutingService<MultiDBRoutingState>::TransformRouteDataToWay(data);
  }
}

