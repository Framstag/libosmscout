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

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutingProfile.h>


//#define DEBUG_ROUTING


namespace osmscout {

  /**
   * Create a new instance of the routing service.
   *
   * @param database
   *    A valid reference to a database instance
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
     isOpen(false),
     routeNodeDataFile(GetDataFilename(filenamebase),
                       GetIndexFilename(filenamebase),
                       /*indexCacheSize*/ 12000,
                       /*dataCacheSize*/ 1000),
     junctionDataFile(RoutingService::FILENAME_INTERSECTIONS_DAT,
                      RoutingService::FILENAME_INTERSECTIONS_IDX,
                      /*indexCacheSize*/ 10000,
                      /*dataCacheSize*/ 1000)
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
    for (const auto node : nodes) {
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
                                    const DatabaseId /*database*/,
                                    const RouteNode& routeNode,
                                    size_t pathIndex)
  {
    return profile.CanUse(routeNode,objectVariantDataFile.GetData(),pathIndex);
  }

  bool SimpleRoutingService::CanUseForward(const RoutingProfile& profile,
                                           const DatabaseId& /*database*/,
                                           const WayRef& way)
  {
    return profile.CanUseForward(*way);
  }

  bool SimpleRoutingService::CanUseBackward(const RoutingProfile& profile,
                                            const DatabaseId& /*database*/,
                                            const WayRef& way)
  {
    return profile.CanUseBackward(*way);
  }

  double SimpleRoutingService::GetCosts(const RoutingProfile& profile,
                                        const DatabaseId /*database*/,
                                        const RouteNode& routeNode,
                                        size_t pathIndex)
  {
    return profile.GetCosts(routeNode,objectVariantDataFile.GetData(),pathIndex);
  }

  double SimpleRoutingService::GetCosts(const RoutingProfile& profile,
                                        const DatabaseId /*database*/,
                                        const WayRef &way,
                                        double wayLength)
  {
    return profile.GetCosts(*way,wayLength);
  }

  double SimpleRoutingService::GetEstimateCosts(const RoutingProfile& profile,
                                                const DatabaseId /*database*/,
                                                double targetDistance)
  {
    return profile.GetCosts(targetDistance);
  }

  double SimpleRoutingService::GetCostLimit(const RoutingProfile& profile,
                                            const DatabaseId /*database*/,
                                            double targetDistance)
  {
    return profile.GetCosts(profile.GetCostLimitDistance())+targetDistance*profile.GetCostLimitFactor();
  }

  bool SimpleRoutingService::GetRouteNode(const DatabaseId &/*database*/,
                                          const Id &id,
                                          RouteNodeRef &node)
  {
    return routeNodeDataFile.Get(id, node);
  }

  bool SimpleRoutingService::GetRouteNodesByOffset(const std::set<DBFileOffset> &routeNodeOffsets,
                                                   std::unordered_map<DBFileOffset,RouteNodeRef> &routeNodeMap)
  {
    if (routeNodeOffsets.empty()){
      return true;
    }
    std::vector<FileOffset> offsets(routeNodeOffsets.size());
    std::transform(routeNodeOffsets.begin(),routeNodeOffsets.end(),offsets.begin(),
                   [](const DBFileOffset &dbOff){return dbOff.offset;});

    std::unordered_map<FileOffset,RouteNodeRef> nodeMap;
    if (!routeNodeDataFile.GetByOffset(offsets.begin(),
                                       offsets.end(),
                                       offsets.size(),
                                       nodeMap)){
      return false;
    }
    DatabaseId dbId=routeNodeOffsets.begin()->database;
    for (auto const &entry:nodeMap){
      routeNodeMap[DBFileOffset(dbId,entry.first)]=entry.second;
    }
    return true;
  }

  bool SimpleRoutingService::GetRouteNodeByOffset(const DBFileOffset &offset,
                                                  RouteNodeRef &node)
  {
    return routeNodeDataFile.GetByOffset(offset.offset, node);
  }

  bool SimpleRoutingService::GetRouteNodeOffset(const DatabaseId &/*database*/,
                                                const Id &id,
                                                FileOffset &offset)
  {
    return routeNodeDataFile.GetOffset(id,offset);
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

    StopClock timer;

    if (!routeNodeDataFile.Open(database->GetTypeConfig(),
                                path,
                                true,
                                database->GetRouterDataMMap())) {
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
  bool SimpleRoutingService::IsOpen() const
  {
    return isOpen;
  }

  /**
   * Close the routing service
   */
  void SimpleRoutingService::Close()
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
  TypeConfigRef SimpleRoutingService::GetTypeConfig() const
  {
    return database->GetTypeConfig();
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
  RoutingResult SimpleRoutingService::CalculateRoute(RoutingProfile& profile, // TODO: make it const!
                                                     const RoutePosition& start,
                                                     const RoutePosition& target,
                                                     const RoutingParameter& parameter)
  {
    return AbstractRoutingService<RoutingProfile>::CalculateRoute(profile,start,target,parameter);
  }

  bool SimpleRoutingService::GetWayByOffset(const DBFileOffset &offset,
                                            WayRef &way)
  {
    WayDataFileRef  wayDataFile(database->GetWayDataFile());
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
                   [](const DBFileOffset &dbOff){return dbOff.offset;});

    std::unordered_map<FileOffset,WayRef> map;
    if (!wayDataFile->GetByOffset(offsets.begin(),
                                  offsets.end(),
                                  offsets.size(),
                                  map)){
      return false;
    }
    DatabaseId dbId=wayOffsets.begin()->database;
    for (const auto &entry:map){
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

  std::vector<DBFileOffset> SimpleRoutingService::GetNodeTwins(const RoutingProfile& /*state*/,
                                                               const DatabaseId /*database*/,
                                                               const Id /*id*/)
  {
    std::vector<DBFileOffset> result;
    return result;
  }

  /**
   * Calculate a route going through all the via points
   *
   * @param profile
   *    Profile to use
   * @param via
   *    A vector of via points
   * @param radius
   *    The maximum radius to search in from the search center in meter
   * @param parameter
   *    A RoutingParamater object
   * @return
   *    A RoutingResult object
   */

  RoutingResult SimpleRoutingService::CalculateRoute(RoutingProfile& profile,
                                                     std::vector<osmscout::GeoCoord> via,
                                                     double radius,
                                                     const RoutingParameter& parameter)
  {
    RoutingResult                        result;
    std::vector<size_t>                  nodeIndexes;
    std::vector<osmscout::ObjectFileRef> objects;

    assert(!via.empty());

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

    for (size_t index=0; index<nodeIndexes.size()-1; index++) {
      size_t                     fromNodeIndex=nodeIndexes.at(index);
      osmscout::ObjectFileRef    fromObject=objects.at(index);
      size_t                     toNodeIndex=nodeIndexes.at(index+1);
      osmscout::ObjectFileRef    toObject=objects.at(index+1);
      RoutingResult              partialResult;

      partialResult=CalculateRoute(profile,
                                   RoutePosition(fromObject,fromNodeIndex,/*database*/0),
                                   RoutePosition(toObject,toNodeIndex,/*database*/0),
                                   parameter);
      if (!partialResult.Success()) {
        result.GetRoute().Clear();

        return result;
      }

      /* In intermediary via points the end of the previous part is the start of the */
      /* next part, we need to remove the duplicate point in the calculated route */
      if (index<nodeIndexes.size()-2) {
        partialResult.GetRoute().PopEntry();
      }

      result.GetRoute().Append(partialResult.GetRoute());
    }

    return result;
  }

  void SimpleRoutingService::DumpStatistics()
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
   *    Routing profile to use. It defines Vehicle to use
   * @param radius
   *    The maximum radius to search in from the search center in meter, at
   *    return is set to the minimum distance found
   * @return
   */
  RoutePosition SimpleRoutingService::GetClosestRoutableNode(const GeoCoord& coord,
                                                             const RoutingProfile& profile,
                                                             double& radius) const
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

          position=RoutePosition(area->GetObjectFileRef(),i,/*database*/0);
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
            position=RoutePosition(way->GetObjectFileRef(),i,/*database*/0);
          } else {
            position=RoutePosition(way->GetObjectFileRef(),i+1,/*database*/0);
          }
        }
      }
    }
      
    radius = minDistance;
    return position;
  }
}
