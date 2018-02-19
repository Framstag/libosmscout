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
#include <osmscout/Pixel.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING

namespace osmscout {

  const size_t MultiDBRoutingService::CELL_MAGNIFICATION=65536; // 2^16
  const double MultiDBRoutingService::LAT_CELL_FACTOR=180.0/ MultiDBRoutingService::CELL_MAGNIFICATION;
  const double MultiDBRoutingService::LON_CELL_FACTOR=360.0/ MultiDBRoutingService::CELL_MAGNIFICATION;

  MultiDBRoutingService::MultiDBRoutingService(const RouterParameter& parameter,
                                               const std::vector<DatabaseRef> &databases):
    AbstractRoutingService<MultiDBRoutingState>(parameter),
    isOpen(false)
  {
    this->handles.resize(databases.size());

    DatabaseId id=0;
    for (auto& db : databases) {
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

      handle.router=router;
      handle.profile=profileBuilder(handle.database);

      RoutingDatabaseRef routingDatabase=std::make_shared<RoutingDatabase>();
      if (!routingDatabase->Open(handle.database)) {
        Close();
        return false;
      }

      handle.routingDatabase=routingDatabase;

    }

    return true;
  }

  void MultiDBRoutingService::Close()
  {
    if (!isOpen) {
      return;
    }

    for (auto handle : handles) {
      handle.routingDatabase->Close();
      handle.routingDatabase.reset();
      handle.router->Close();
      handle.router.reset();
      handle.profile.reset();
    }

    isOpen=false;
  }

  RoutePosition MultiDBRoutingService::GetClosestRoutableNode(const GeoCoord& coord,
                                                              double radius) const
  {
    RoutePosition position, closestPosition;

    double minDistance=std::numeric_limits<double>::max();

    for (auto& handle : handles) {
      double distance = radius;
      position=handle.router->GetClosestRoutableNode(coord,
                                                     *handle.profile,
                                                     distance);
      if (position.IsValid() && distance < minDistance) {
        closestPosition=RoutePosition(position.GetObjectFileRef(),
                                      position.GetNodeIndex(),
                                      /*database*/
                                      handle.dbId);
        minDistance=distance;
      }
    }

    return closestPosition;
  }

  Pixel MultiDBRoutingService::GetCell(const osmscout::GeoCoord& coord)
  {
    return {uint32_t((coord.GetLon()+180.0)/ LON_CELL_FACTOR),
            uint32_t((coord.GetLat()+90.0)/ LAT_CELL_FACTOR)};
  }

  Vehicle MultiDBRoutingService::GetVehicle(const MultiDBRoutingState& state)
  {
    return state.GetVehicle();
  }

  bool MultiDBRoutingService::CanUseForward(const MultiDBRoutingState& state,
                                            const DatabaseId& database,
                                            const WayRef& way)
  {
    return state.GetProfile(database)->CanUseForward(*way);
  }

  bool MultiDBRoutingService::CanUseBackward(const MultiDBRoutingState& state,
                                             const DatabaseId& database,
                                             const WayRef& way)
  {
    return state.GetProfile(database)->CanUseBackward(*way);
  }

  double MultiDBRoutingService::GetCosts(const MultiDBRoutingState& state,
                                         const DatabaseId databaseId,
                                         const RouteNode& routeNode,
                                         size_t pathIndex)
  {
    return state.GetProfile(databaseId)->GetCosts(routeNode,
                                                handles[databaseId].routingDatabase->GetObjectVariantData(),
                                                pathIndex);
  }

  double MultiDBRoutingService::GetCosts(const MultiDBRoutingState& state,
                                         const DatabaseId database,
                                         const WayRef &way,
                                         double wayLength)
  {
    return state.GetProfile(database)->GetCosts(*way,wayLength);
  }

  double MultiDBRoutingService::GetEstimateCosts(const MultiDBRoutingState& state,
                                                 const DatabaseId database,
                                                 double targetDistance)
  {
    return state.GetProfile(database)->GetCosts(targetDistance);
  }

  double MultiDBRoutingService::GetCostLimit(const MultiDBRoutingState& state,
                                             const DatabaseId database,
                                             double targetDistance)
  {
    RoutingProfileRef profile=state.GetProfile(database);
    return profile->GetCosts(profile->GetCostLimitDistance())+targetDistance*profile->GetCostLimitFactor();
  }

  bool MultiDBRoutingService::ReadCellsForRoutingTree(osmscout::Database& database,
                                                      std::unordered_set<uint64_t>& cells)
  {
    std::string filename=std::string(osmscout::RoutingService::DEFAULT_FILENAME_BASE)+".dat";
    std::string fullFilename=osmscout::AppendFileToDir(database.GetPath(),filename);

    osmscout::FileScanner scanner;

    try {
      uint32_t count;

      std::cout << "Opening routing file '" << fullFilename << "'" << std::endl;

      scanner.Open(fullFilename,osmscout::FileScanner::Sequential,false);

      scanner.Read(count);

      for (uint32_t i=1; i<=count; i++) {
        osmscout::RouteNode node;
        osmscout::Pixel     cell;

        node.Read(*database.GetTypeConfig(),scanner);

        cell=GetCell(node.GetCoord());

        cells.insert(cell.GetId());

        //std::cout << node.GetCoord().GetDisplayText() << " " << cell.GetId() << std::endl;
      }

      scanner.Close();
    }
    catch (osmscout::IOException& e) {
      log.Error() << "Error while reading '" << fullFilename << "': " << e.GetDescription();
      return false;
    }

    return true;
  }

  bool MultiDBRoutingService::ReadRouteNodesForCells(osmscout::Database& database,
                                                     std::unordered_set<uint64_t>& cells,
                                                     std::unordered_set<Id>& routeNodes)
  {
    std::string filename=std::string(osmscout::RoutingService::DEFAULT_FILENAME_BASE)+".dat";
    std::string fullFilename=osmscout::AppendFileToDir(database.GetPath(),filename);

    osmscout::FileScanner scanner;

    try {
      uint32_t count;

      std::cout << "Opening routing file '" << fullFilename << "'" << std::endl;

      scanner.Open(fullFilename,osmscout::FileScanner::Sequential,false);

      scanner.Read(count);

      for (uint32_t i=1; i<=count; i++) {
        osmscout::RouteNode node;
        osmscout::Pixel     cell;

        node.Read(*database.GetTypeConfig(),scanner);

        cell=GetCell(node.GetCoord());

        if (cells.find(cell.GetId())!=cells.end()) {
          routeNodes.insert(node.GetId());
        }
      }

      scanner.Close();
    }
    catch (osmscout::IOException& e) {
      log.Error() << "Error while reading '" << fullFilename << "': " << e.GetDescription();
      return false;
    }

    return true;
  }

  bool MultiDBRoutingService::FindCommonRoutingNodes(const BreakerRef &breaker,
                                                     DatabaseRef &database1,
                                                     DatabaseRef &database2,
                                                     std::set<Id> &commonRouteNodes)
  {

    std::unordered_set<uint64_t> cells1;
    std::unordered_set<uint64_t> cells2;

#if defined(DEBUG_ROUTING)
    std::cout << "Reading route node cells of database 1..." << std::endl;
#endif

    if (!ReadCellsForRoutingTree(*database1,
                                 cells1)){
      return false;
    }
    if (breaker &&
        breaker->IsAborted()) {
      return true;
    }

#if defined(DEBUG_ROUTING)
    std::cout << "Found route nodes in " << cells1.size() << " cells" << std::endl;
    std::cout << "Reading route node cells of database 2..." << std::endl;
#endif

    if (!ReadCellsForRoutingTree(*database2,
                                 cells2)){
      return false;
    }
    if (breaker &&
        breaker->IsAborted()) {
      return true;
    }

#if defined(DEBUG_ROUTING)
    std::cout << "Found route nodes in " << cells2.size() << " cells" << std::endl;
    std::cout << "Detecting common cells..." << std::endl;
#endif

    std::unordered_set<uint64_t> commonCells;

    for (const auto cell : cells1) {
      if (cells2.find(cell)!=cells2.end()) {
        commonCells.insert(cell);
      }
    }

    cells1.clear();
    cells2.clear();

#if defined(DEBUG_ROUTING)
    std::cout << "There are " << commonCells.size() << " common cells" << std::endl;
#endif

    std::unordered_set<Id> routeNodes1;
    std::unordered_set<Id> routeNodes2;

#if defined(DEBUG_ROUTING)
    std::cout << "Reading route nodes in common cells of database 1..." << std::endl;
#endif

    if (!ReadRouteNodesForCells(*database1,
                                commonCells,
                                routeNodes1)){
      return false;
    }
    if (breaker &&
        breaker->IsAborted()) {
      return true;
    }

#if defined(DEBUG_ROUTING)
    std::cout << "Found " << routeNodes1.size() << " route nodes in common cells" << std::endl;
    std::cout << "Reading route nodes in common cells of database 2..." << std::endl;
#endif

    if (!ReadRouteNodesForCells(*database2,
                                commonCells,
                                routeNodes2)){
      return false;
    }
    if (breaker &&
        breaker->IsAborted()) {
      return true;
    }

#if defined(DEBUG_ROUTING)
    std::cout << "Found " << routeNodes2.size() << " route nodes in common cells" << std::endl;
    std::cout << "Detecting common route nodes..." << std::endl;
#endif

    for (const auto routeNode : routeNodes1) {
      if (routeNodes2.find(routeNode)!=routeNodes2.end()) {
        commonRouteNodes.insert(routeNode);
      }
    }

    routeNodes1.clear();
    routeNodes2.clear();

#if defined(DEBUG_ROUTING)
    std::cout << "There are " << commonRouteNodes.size() << " common route nodes" << std::endl;
#endif
    return true;
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

  bool MultiDBRoutingService::GetRouteNodesByOffset(const std::set<DBFileOffset> &routeNodeOffsets,
                                                    std::unordered_map<DBFileOffset,RouteNodeRef> &routeNodeMap)
  {
    std::unordered_map<DatabaseId,std::set<FileOffset>> offsetMap;
    for (const auto &offset:routeNodeOffsets){
      offsetMap[offset.database].insert(offset.offset);
    }
    for (const auto &entry:offsetMap){
      std::vector<RouteNodeRef>  nodes;
      const std::set<FileOffset> &offsets=entry.second;

      if (!handles[entry.first].routingDatabase->GetRouteNodesByOffset(offsets,
                                                                       nodes)) {
        return false;
      }
      for (const auto &node:nodes){
        routeNodeMap[DBFileOffset(entry.first,node->GetFileOffset())]=node;
      }
    }
    return true;
  }

  bool MultiDBRoutingService::GetRouteNodeByOffset(const DBFileOffset &offset,
                                                   RouteNodeRef &node)
  {
    return handles[offset.database].routingDatabase->GetRouteNodeByOffset(offset.offset, node);
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
    for (auto& handle : handles) {
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
        junctionMap.insert(std::make_pair(junction->GetId(),junction));
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

  std::vector<DBFileOffset> MultiDBRoutingService::GetNodeTwins(const MultiDBRoutingState& state,
                                                                const DatabaseId database,
                                                                const Id id)
  {
    std::set<DatabaseId> overlappingDatabases;
    state.GetOverlappingDatabases(database,id,overlappingDatabases);

    std::vector<DBFileOffset> twins;

    twins.reserve(overlappingDatabases.size());
    for (const auto &dbId:overlappingDatabases){
      FileOffset offset;
      if (!handles[dbId].routingDatabase->GetRouteNodeOffset(id,offset)){
        log.Error() << "Failed to retrieve file offset";
        continue;
      }
      twins.emplace_back(dbId,offset);
    }
    return twins;
  }

  bool MultiDBRoutingService::GetRouteNode(const DatabaseId &databaseId,
                                           const Id &id,
                                           RouteNodeRef &node)
  {
    return handles[databaseId].routingDatabase->GetRouteNode(id, node);
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
      log.Error() << "Can't find start database " << start.GetDatabaseId();
      return result;
    }

    DatabaseRef database1=handles[start.GetDatabaseId()].database;

    if (target.GetDatabaseId()>=handles.size() ||
        !handles[target.GetDatabaseId()].database) {
      log.Error() << "Can't find target database " << target.GetDatabaseId();
      return result;
    }

    DatabaseRef database2=handles[target.GetDatabaseId()].database;

    std::set<Id> commonRouteNodes;

    if (!FindCommonRoutingNodes(parameter.GetBreaker(),
                                database1,
                                database2,
                                commonRouteNodes)) {
      log.Error() << "Can't find common routing nodes for databases " <<
        database1->GetPath() << ", " <<
        database2->GetPath();
      return result;
    }

    if (parameter.GetBreaker() &&
        parameter.GetBreaker()->IsAborted()) {
      return result;
    }

    if (commonRouteNodes.empty()){
      log.Warn() << "Can't find common routing nodes for databases " <<
        database1->GetPath() << ", " <<
        database2->GetPath();
      return result;
    }

    MultiDBRoutingState state(start.GetDatabaseId(),
                              target.GetDatabaseId(),
                              handles[start.GetDatabaseId()].profile,
                              handles[target.GetDatabaseId()].profile,
                              commonRouteNodes);

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
    RoutingResult MultiDBRoutingService::CalculateRoute(std::vector<osmscout::GeoCoord> via,
                                                        double radius,
                                                        const RoutingParameter& parameter)
    {
      RoutingResult              result;
      std::vector<RoutePosition> routePositions;

      assert(!via.empty());

      for (const auto& etap : via) {
        RoutePosition target=GetClosestRoutableNode(etap,
                                                    radius);

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

          return result;
        }

        /* In intermediary via points the end of the previous part is the start of the */
        /* next part, we need to remove the duplicate point in the calculated route */
        if (index<routePositions.size()-2) {
          partialResult.GetRoute().PopEntry();
        }

        result.GetRoute().Append(partialResult.GetRoute());
      }

      return result;
    }

  bool MultiDBRoutingService::PostProcessRouteDescription(RouteDescription &description,
                                                          const std::list<RoutePostprocessor::PostprocessorRef> &postprocessors)
  {
    std::set<std::string> motorwayTypeNames;
    std::set<std::string> motorwayLinkTypeNames;
    std::set<std::string> junctionTypeNames;

    junctionTypeNames.insert("highway_motorway_junction");

    motorwayTypeNames.insert("highway_motorway");
    motorwayLinkTypeNames.insert("highway_motorway_link");

    motorwayTypeNames.insert("highway_motorway_trunk");
    motorwayTypeNames.insert("highway_trunk");

    motorwayLinkTypeNames.insert("highway_trunk_link");
    motorwayTypeNames.insert("highway_motorway_primary");

    RoutePostprocessor routePostprocessor;

    std::vector<RoutingProfileRef> profiles;
    std::vector<DatabaseRef>       databases;

    std::transform(handles.begin(),handles.end(),std::back_inserter(profiles),[](DatabaseHandle& handle) {
      return handle.profile;
    });

    std::transform(handles.begin(),handles.end(),std::back_inserter(databases),[](DatabaseHandle& handle) {
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

  // FIXME: I don't understand why these methods should be here...
  bool MultiDBRoutingService::TransformRouteDataToRouteDescription(const RouteData& data,
                                                                   RouteDescription& description)
  {
    return AbstractRoutingService<MultiDBRoutingState>::TransformRouteDataToRouteDescription(data,description);
  }

  bool MultiDBRoutingService::TransformRouteDataToPoints(const RouteData& data,
                                                         std::list<Point>& points)
  {
    return AbstractRoutingService<MultiDBRoutingState>::TransformRouteDataToPoints(data,points);
  }

  bool MultiDBRoutingService::TransformRouteDataToWay(const RouteData& data,
                                                      Way& way)
  {
    return AbstractRoutingService<MultiDBRoutingState>::TransformRouteDataToWay(data,way);
  }
}
