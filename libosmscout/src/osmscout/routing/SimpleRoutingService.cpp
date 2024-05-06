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

#include <iomanip>
#include <iostream>
#include <algorithm>

#include <osmscout/system/Assert.h>

#include <osmscout/feature/NameFeature.h>

#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutingProfile.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/log/Logger.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING


namespace osmscout {

  /**
   * Create a new instance of the routing service.
   *
   * @param database
   *    A valid reference to a db instance
   * @param parameter
   *    An instance to the parameter object holding further paramterization
   */
  SimpleRoutingService::SimpleRoutingService(const DatabaseRef& database,
                                             const RouterParameter& parameter,
                                             const std::string& filenamebase)
   : AbstractRoutingService<RoutingProfile>(parameter),
     database(database),
     filenamebase(filenamebase),
     accessReader(*database->GetTypeConfig()),
     isOpen(false)
  {
    assert(database);
  }

  SimpleRoutingService::~SimpleRoutingService()
  {
    if (isOpen) {
      Close();
    }
  }

  bool SimpleRoutingService::HasNodeWithId(const std::vector<Point>& nodes) const
  {
    for (const auto& node : nodes) {
      if (node.IsRelevant()) {
        return true;
      }
    }

    return false;
  }

  Vehicle SimpleRoutingService::GetVehicle(const RoutingProfile& profile)
  {
    return profile.GetVehicle();
  }

  bool SimpleRoutingService::CanUse(const RoutingProfile& profile,
                                    const DatabaseId /*db*/,
                                    const RouteNode& routeNode,
                                    size_t pathIndex)
  {
    return profile.CanUse(routeNode,routingDatabase.GetObjectVariantData(),pathIndex);
  }

  bool SimpleRoutingService::CanUseForward(const RoutingProfile& profile,
                                           const DatabaseId& /*db*/,
                                           const WayRef& way)
  {
    return profile.CanUseForward(*way);
  }

  bool SimpleRoutingService::CanUseBackward(const RoutingProfile& profile,
                                            const DatabaseId& /*db*/,
                                            const WayRef& way)
  {
    return profile.CanUseBackward(*way);
  }

  double SimpleRoutingService::GetCosts(const RoutingProfile& profile,
                                        const DatabaseId /*db*/,
                                        const RouteNode& routeNode,
                                        size_t inPathIndex,
                                        size_t outPathIndex)
  {
    return profile.GetCosts(routeNode,routingDatabase.GetObjectVariantData(),inPathIndex,outPathIndex);
  }

  double SimpleRoutingService::GetCosts(const RoutingProfile& profile,
                                        const DatabaseId /*db*/,
                                        const WayRef &way,
                                        const Distance &wayLength)
  {
    return profile.GetCosts(*way,wayLength);
  }

  double SimpleRoutingService::GetUTurnCost(const RoutingProfile& profile, const DatabaseId /*databaseId*/)
  {
    return profile.GetUTurnCost();
  }

  double SimpleRoutingService::GetEstimateCosts(const RoutingProfile& profile,
                                                const DatabaseId /*db*/,
                                                const Distance &targetDistance)
  {
    return profile.GetCosts(targetDistance);
  }

  double SimpleRoutingService::GetCostLimit(const RoutingProfile& profile,
                                            const DatabaseId /*db*/,
                                            const Distance &targetDistance)
  {
    return profile.GetCosts(profile.GetCostLimitDistance()) + profile.GetCosts(targetDistance)*profile.GetCostLimitFactor();
  }

  std::string SimpleRoutingService::GetCostString(const RoutingProfile& profile,
                                                  DatabaseId /*db*/,
                                                  double cost) const
  {
    return profile.GetCostString(cost);
  }

  bool SimpleRoutingService::GetRouteNodes(const std::set<DBId> &routeNodeIds,
                                           std::unordered_map<DBId,RouteNodeRef> &routeNodeMap)
  {
    if (routeNodeIds.empty()){
      return true;
    }

    std::vector<Id> ids(routeNodeIds.size());

    std::transform(routeNodeIds.begin(),routeNodeIds.end(),ids.begin(),
                   [](const DBId& dbId) {
                     return dbId.id;
                   });

    std::unordered_map<Id,RouteNodeRef> nodeMap;

    if (!routingDatabase.GetRouteNodes(ids.begin(),
                                       ids.end(),
                                       ids.size(),
                                       nodeMap)) {
      return false;
    }

    DatabaseId dbId=routeNodeIds.begin()->database;

    for (auto const& entry : nodeMap) {
      routeNodeMap[DBId(dbId,entry.first)]=entry.second;
    }

    return true;
  }

  bool SimpleRoutingService::GetRouteNode(const DBId &id,
                                          RouteNodeRef& node)
  {
    return routingDatabase.GetRouteNode(id.id,
                                        node);
  }

  bool SimpleRoutingService::GetWayByOffset(const DBFileOffset &offset,
                                            WayRef &way)
  {
    WayDataFileRef wayDataFile(database->GetWayDataFile());

    if (!wayDataFile) {
      return false;
    }

    return wayDataFile->GetByOffset(offset.offset,way);
  }

  bool SimpleRoutingService::GetWaysByOffset(const std::set<DBFileOffset> &wayOffsets,
                                             std::unordered_map<DBFileOffset,WayRef> &wayMap)
  {
    if (wayOffsets.empty()){
      return true;
    }

    WayDataFileRef wayDataFile(database->GetWayDataFile());

    if (!wayDataFile) {
      return false;
    }

    std::vector<FileOffset> offsets(wayOffsets.size());
    std::transform(wayOffsets.begin(),wayOffsets.end(),offsets.begin(),
                   [](const DBFileOffset& dbOff) {
                     return dbOff.offset;
                   });

    std::unordered_map<FileOffset,WayRef> map;
    if (!wayDataFile->GetByOffset(offsets.begin(),
                                  offsets.end(),
                                  offsets.size(),
                                  map)) {
      return false;
    }

    DatabaseId dbId=wayOffsets.begin()->database;
    for (const auto &entry:map) {
      wayMap[DBFileOffset(dbId,entry.first)]=entry.second;
    }

    return true;
  }

  bool SimpleRoutingService::GetAreaByOffset(const DBFileOffset &offset,
                                             AreaRef &area)
  {
    AreaDataFileRef areaDataFile(database->GetAreaDataFile());
    if (!areaDataFile){
      return false;
    }
    return areaDataFile->GetByOffset(offset.offset,area);
  }

  bool SimpleRoutingService::GetAreasByOffset(const std::set<DBFileOffset> &areaOffsets,
                                              std::unordered_map<DBFileOffset,AreaRef> &areaMap)
  {
    if (areaOffsets.empty()){
      return true;
    }

    AreaDataFileRef areaDataFile(database->GetAreaDataFile());

    if (!areaDataFile){
      return false;
    }

    std::vector<FileOffset> offsets(areaOffsets.size());
    std::transform(areaOffsets.begin(),areaOffsets.end(),offsets.begin(),
                   [](const DBFileOffset &dbOff){return dbOff.offset;});

    std::unordered_map<FileOffset,AreaRef> map;
    if (!areaDataFile->GetByOffset(offsets.begin(),
                                   offsets.end(),
                                   offsets.size(),
                                   map)) {
      return false;
    }

    DatabaseId dbId=areaOffsets.begin()->database;
    for (const auto &entry:map){
      areaMap[DBFileOffset(dbId,entry.first)]=entry.second;
    }

    return true;
  }

  bool SimpleRoutingService::ResolveRouteDataJunctions(RouteData& route)
  {
    std::set<Id>             nodeIds;
    std::vector<JunctionRef> junctions;

    for (const auto& routeEntry : route.Entries()) {
      if (routeEntry.GetCurrentNodeId()!=0) {
        nodeIds.insert(routeEntry.GetCurrentNodeId());
      }
    }

    if (!routingDatabase.GetJunctions(nodeIds,junctions)) {
      return false;
    }

    nodeIds.clear();

    std::unordered_map<Id,JunctionRef> junctionMap;

    for (const auto& junction : junctions) {
      junctionMap.emplace(junction->GetId(),junction);
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

    return true;
  }

  std::vector<DBId> SimpleRoutingService::GetNodeTwins(const RoutingProfile& /*state*/,
                                                       const DatabaseId /*db*/,
                                                       const Id /*id*/)
  {
    std::vector<DBId> result;

    return result;
  }

  /**
   * Opens the routing service. This loads the routing graph for the given vehicle
   *
   * @return
   *    false on error, else true
   */
  bool SimpleRoutingService::Open()
  {
    path=database->GetPath();

    assert(!path.empty());

    if (!routingDatabase.Open(database)) {
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
  bool SimpleRoutingService::IsOpen() const
  {
    return isOpen;
  }

  /**
   * Close the routing service
   */
  void SimpleRoutingService::Close()
  {
    routingDatabase.Close();

    isOpen=false;
  }

  /**
   * Returns the type configuration of the underlying db instance
   *
   * @return
   *    A valid type configuration or null if the db is not valid
   */
  TypeConfigRef SimpleRoutingService::GetTypeConfig() const
  {
    return database->GetTypeConfig();
  }

  /**
   * Calculate a route going through all the via points
   *
   * @param profile
   *    Profile to use
   * @param via
   *    A vector of via points
   * @param radius
   *    The maximum radius to search in from the search center
   * @param parameter
   *    A RoutingParamater object
   * @return
   *    A RoutingResult object
   */

  RoutingResult SimpleRoutingService::CalculateRouteViaCoords(RoutingProfile& profile,
                                                              const std::vector<osmscout::GeoCoord>& via,
                                                              const Distance &radius,
                                                              const RoutingParameter& parameter)
  {
    RoutingResult                        result;
    std::vector<size_t>                  nodeIndexes;
    std::vector<osmscout::ObjectFileRef> objects;

    assert(!via.empty());

    for (const auto& etap : via) {
      auto posResult=GetClosestRoutableNode(etap, profile, radius);
      RoutePosition target=posResult.GetRoutePosition();

      if (!target.IsValid()) {
        return result;
      }

      nodeIndexes.push_back(target.GetNodeIndex());
      objects.push_back(target.GetObjectFileRef());
    }

    for (size_t index=0; index<nodeIndexes.size()-1; index++) {
      size_t                     fromNodeIndex=nodeIndexes.at(index);
      osmscout::ObjectFileRef    fromObject=objects.at(index);
      size_t                     toNodeIndex=nodeIndexes.at(index+1);
      osmscout::ObjectFileRef    toObject=objects.at(index+1);
      RoutingResult              partialResult;

      partialResult=CalculateRoute(profile,
                                   RoutePosition(fromObject,
                                                 fromNodeIndex,/*db*/
                                                 0),
                                   RoutePosition(toObject,
                                                 toNodeIndex,/*db*/
                                                 0),
                                   std::nullopt,
                                   parameter);
      if (!partialResult.Success()) {
        result.GetRoute().Clear();
        result.ClearSectionLengths();

        return result;
      }

      /* In intermediary via points the end of the previous part is the start of the */
      /* next part, we need to remove the duplicate point in the calculated route */
      if (index<nodeIndexes.size()-2) {
        partialResult.GetRoute().PopEntry();
      }

      result.GetRoute().Append(partialResult.GetRoute());
      result.AppendSectionLength(partialResult.GetRoute().Entries().size());
    }

    return result;
  }

  std::map<DatabaseId, std::string> SimpleRoutingService::GetDatabaseMapping() const
  {
    std::map<DatabaseId, std::string> mapping;
    mapping[0] = database->GetPath();
    return mapping;
  }

  void SimpleRoutingService::DumpStatistics()
  {
    if (database) {
      database->DumpStatistics();
    }
  }

  RoutePositionResult SimpleRoutingService::GetRoutableNode(const ObjectFileRef& objRef,
                                                            const RoutingProfile& profile) const
  {
    TypeConfigRef typeConfig=database->GetTypeConfig();
    RoutePositionResult position;

    if (!typeConfig) {
      log.Error() << "TypeConfig is invalid!";
      return position;
    }

    if (objRef.IsWay()) {
      WayDataFileRef wayDataFile=database->GetWayDataFile();
      if (!wayDataFile) {
        log.Error() << "Way data file is invalid!";
        return position;
      }
      WayRef way;
      if (!wayDataFile->GetByOffset(objRef.GetFileOffset(), way)) {
        log.Error() << "Cannot read " << objRef.GetName();
        return position;
      }
      if (!profile.CanUse(*way)) {
        return position;
      }
      if (!HasNodeWithId(way->nodes)) {
        return position;
      }
      // way distance center
      Distance wayLength;
      for (size_t i=0; i<way->nodes.size()-1; i++){
        wayLength+=GetSphericalDistance(way->nodes[i].GetCoord(), way->nodes[i+1].GetCoord());
      }
      size_t wayCenter=0;
      for (Distance d; wayCenter < way->nodes.size() - 1; wayCenter++){
        d+=GetSphericalDistance(way->nodes[wayCenter].GetCoord(), way->nodes[wayCenter+1].GetCoord());
        if (d>wayLength/2) {
          break;
        }
      }

      position=RoutePositionResult(RoutePosition(way->GetObjectFileRef(), wayCenter,/*db*/0), Distance::Zero());
    }
    // TODO: support areas in router

    return position;
  }

  RoutePositionResult SimpleRoutingService::GetClosestRoutableNode(const GeoCoord& coord,
                                                                   const RoutingProfile& profile,
                                                                   const Distance &radius) const
  {
    TypeConfigRef       typeConfig=database->GetTypeConfig();
    AreaAreaIndexRef    areaAreaIndex=database->GetAreaAreaIndex();
    AreaWayIndexRef     areaWayIndex=database->GetAreaWayIndex();
    AreaDataFileRef     areaDataFile=database->GetAreaDataFile();
    WayDataFileRef      wayDataFile=database->GetWayDataFile();
    RoutePositionResult position;

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

    if (!wayDataFile->GetByOffset(wayWayOffsets.begin(),
                                  wayWayOffsets.end(),
                                  wayWayOffsets.size(),
                                  ways)) {
      log.Error() << "Error reading ways in area!";
      return position;
    }

    if (!areaDataFile->GetByBlockSpans(wayAreaSpans.begin(),
                                       wayAreaSpans.end(),
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

          position=RoutePositionResult(RoutePosition(area->GetObjectFileRef(),i,/*db*/0),
                                       GetEllipsoidalDistance(coord, area->rings[0].nodes[i].GetCoord()));
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


      for (size_t i=0;  i<way->nodes.size()-1; i++) {
        double r, intersectLon, intersectLat;
        double distance=DistanceToSegment(coord.GetLon(),coord.GetLat(),way->nodes[i].GetLon(),way->nodes[i].GetLat(),
                                          way->nodes[i+1].GetLon(),way->nodes[i+1].GetLat(), r, intersectLon, intersectLat);
        if (distance<minDistance) {
          minDistance=distance;
          if(r<0.5){
            position=RoutePositionResult(RoutePosition(way->GetObjectFileRef(),i,/*db*/0),
                                         GetEllipsoidalDistance(coord, GeoCoord(intersectLat, intersectLon)));
          } else {
            position=RoutePositionResult(RoutePosition(way->GetObjectFileRef(),i+1,/*db*/0),
                                         GetEllipsoidalDistance(coord, GeoCoord(intersectLat, intersectLon)));
          }
        }
      }
    }

    return position;
  }

  /**
   * Returns the closest routeable object (area or way) relative
   * to the given coordinate.
   *
   * The result should be use for typical "object you are traveling on"
   * information as used by routing applications.
   *
   * @note The returned node may in fact not be routable, it is just
   * the closest node to the given position on a routable way or area.
   *
   * @note The actual object may not be within the given radius
   * due to internal search index resolution.
   *
   * @note This is a simple solution that does not track any spast state.
   * A better implementation should hold on recently travels coordinates and
   * ways or areas and do some tolerance error handling in case of GPS
   * jitter effects.
   *
   * @param coord
   *    coordinate of the search center
   * @param vehicle
   *    The vehicle to use
   * @param maxRadius
   *    The maximum radius to search in from the search center
   * @return
   *    A convinient description of the clostest routable object (if valid)
   */
  ClosestRoutableObjectResult SimpleRoutingService::GetClosestRoutableObject(const GeoCoord& location,
                                                                             Vehicle vehicle,
                                                                             const Distance &maxRadius)
  {
    ClosestRoutableObjectResult result;
    TypeInfoSet                 routeableWayTypes;
    TypeInfoSet                 routeableAreaTypes;

    for (const auto& type : database->GetTypeConfig()->GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanBeWay() &&
          type->CanRoute(vehicle)) {
        routeableWayTypes.Set(type);
      }

      if (!type->GetIgnore() &&
          type->CanBeArea() &&
          type->CanRoute(vehicle)) {
        routeableAreaTypes.Set(type);
      }
    }

    Distance closestDistance=Distance::Max();
    WayRef   closestWay;
    AreaRef  closestArea;

    if (!routeableWayTypes.Empty()) {
      WayRegionSearchResult waySearchResult=database->LoadWaysInRadius(location,
                                                                       routeableWayTypes,
                                                                       maxRadius);

      for (const auto& entry : waySearchResult.GetWayResults()) {
        if (entry.GetDistance()<closestDistance) {
          closestDistance=entry.GetDistance();
          closestWay=entry.GetWay();
          closestArea.reset();
        }
      }
    }

    if (!routeableAreaTypes.Empty()) {
      AreaRegionSearchResult areaSearchResult=database->LoadAreasInRadius(location,
                                                                          routeableAreaTypes,
                                                                          maxRadius);
      for (const auto& entry : areaSearchResult.GetAreaResults()) {
        if (entry.GetDistance()<closestDistance) {
          closestDistance=entry.GetDistance();
          closestWay.reset();
          closestArea=entry.GetArea();
        }
      }
    }

    if (!closestWay && !closestArea) {
      result.distance=Distance::Max();
      return result;
    }
    else {
      NameFeatureLabelReader nameFeatureLabelReader(*database->GetTypeConfig());

      result.distance=closestDistance;
      if (closestWay) {
        result.way=std::move(closestWay);
        result.object=result.way->GetObjectFileRef();
        result.name=nameFeatureLabelReader.GetLabel(result.way->GetFeatureValueBuffer());
      }
      else {
        result.area=std::move(closestArea);
        result.object=result.area->GetObjectFileRef();
        result.name=nameFeatureLabelReader.GetLabel(result.area->GetFeatureValueBuffer());
      }
    }

    return result;
  }
}
