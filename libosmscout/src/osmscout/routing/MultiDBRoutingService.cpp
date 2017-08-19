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

#include <iomanip>
#include <iostream>

namespace osmscout {

  RouterDBFiles::RouterDBFiles():
     routeNodeDataFile(RoutingService::GetDataFilename(osmscout::RoutingService::DEFAULT_FILENAME_BASE),
                       RoutingService::GetIndexFilename(osmscout::RoutingService::DEFAULT_FILENAME_BASE),
                       /*indexCacheSize*/ 12000,
                       /*dataCacheSize*/ 1000),
     junctionDataFile(RoutingService::FILENAME_INTERSECTIONS_DAT,
                      RoutingService::FILENAME_INTERSECTIONS_IDX,
                      /*indexCacheSize*/ 10000,
                      /*dataCacheSize*/ 1000)
  {
  }

  RouterDBFiles::~RouterDBFiles()
  {
  }

  bool RouterDBFiles::Open(DatabaseRef database)
  {
    if (!objectVariantDataFile.Load(*(database->GetTypeConfig()),
                                    AppendFileToDir(database->GetPath(),
                                                    RoutingService::GetData2Filename(osmscout::RoutingService::DEFAULT_FILENAME_BASE)))){
      return false;
    }
    if (!routeNodeDataFile.Open(database->GetTypeConfig(),
                                database->GetPath(),
                                true,
                                database->GetRouterDataMMap())) {
      log.Error() << "Cannot open '" <<  database->GetPath() << "'!";
      return false;
    }

    if (!junctionDataFile.Open(database->GetTypeConfig(),
                               database->GetPath(),
                               false,
                               false)) {
      return false;
    }

    return true;
  }

  void RouterDBFiles::Close()
  {
    routeNodeDataFile.Close();
    junctionDataFile.Close();
  }

  MultiDBRoutingService::MultiDBRoutingService(const RouterParameter& parameter,
                                               const std::vector<DatabaseRef> &databases):
    AbstractRoutingService<MultiDBRoutingState>(parameter),
    isOpen(false)
  {
    DatabaseId id=0;
    for (auto &db:databases){
      this->databaseMap[db->GetPath()]=id;
      this->databases[id]=db;
      id++;
    }
  }

  MultiDBRoutingService::~MultiDBRoutingService()
  {
    Close();
  }

  bool MultiDBRoutingService::Open(RoutingProfileBuilder profileBuilder)
  {
    if (databases.empty()){
      return false;
    }
    RouterParameter routerParameter;
    routerParameter.SetDebugPerformance(debugPerformance);

    isOpen=true;
    for (auto &entry:databases){
      DatabaseRef database=entry.second;
      SimpleRoutingServiceRef router=std::make_shared<osmscout::SimpleRoutingService>(
                                              entry.second,
                                              routerParameter,
                                              osmscout::RoutingService::DEFAULT_FILENAME_BASE);
      if (!router->Open()){
        Close();
        return false;
      }
      services[entry.first]=router;
      profiles[entry.first]=profileBuilder(database);

      RouterDBFilesRef dataFile=std::make_shared<RouterDBFiles>();
      if (!dataFile->Open(database)) {
        Close();
        return false;
      }
      routerFiles[entry.first]=dataFile;

    }
    return true;
  }

  void MultiDBRoutingService::Close(){
    if (!isOpen){
      return;
    }
    for (auto &entry:services){
      entry.second->Close();
    }
    for (auto entry:routerFiles){
      entry.second->Close();
    }
    services.clear();
    profiles.clear();
    isOpen=false;
  }

  RoutePosition MultiDBRoutingService::GetClosestRoutableNode(const GeoCoord& coord,
                                                              double radius,
                                                              std::string databasePathHint) const
  {
    RoutePosition position;

    // first try to find in hinted database
    auto databaseHint=databaseMap.find(databasePathHint);
    if (databaseHint!=databaseMap.end()){
      auto it=services.find(databaseHint->second);
      if (it!=services.end()){
        position=it->second->GetClosestRoutableNode(coord,*(profiles.at(it->first)),radius);
        if (position.IsValid()){
          return RoutePosition(position.GetObjectFileRef(),
                               position.GetNodeIndex(),
                               /*database*/ it->first);
        }
      }
    }

    // try all other databases
    for (auto &entry:services){
      if (databaseHint!=databaseMap.end() && entry.first==databaseHint->second){
        continue;
      }

      position=entry.second->GetClosestRoutableNode(coord,*(profiles.at(entry.first)),radius);
      if (position.IsValid()){
          return RoutePosition(position.GetObjectFileRef(),
                               position.GetNodeIndex(),
                               /*database*/ entry.first);
      }
    }
    return position;
  }

  const double MultiDBRoutingService::CELL_MAGNIFICATION=std::pow(2,16);
  const double MultiDBRoutingService::LAT_CELL_FACTOR=180.0/ MultiDBRoutingService::CELL_MAGNIFICATION;
  const double MultiDBRoutingService::LON_CELL_FACTOR=360.0/ MultiDBRoutingService::CELL_MAGNIFICATION;

  Pixel MultiDBRoutingService::GetCell(const osmscout::GeoCoord& coord)
  {
    return osmscout::Pixel(uint32_t((coord.GetLon()+180.0)/ LON_CELL_FACTOR),
                           uint32_t((coord.GetLat()+90.0)/ LAT_CELL_FACTOR));
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
                                         const DatabaseId database,
                                         const RouteNode& routeNode,
                                         size_t pathIndex)
  {
    ;
    return state.GetProfile(database)->GetCosts(routeNode,
                                                routerFiles[database]->objectVariantDataFile.GetData(),
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
                                     const DatabaseId database,
                                     const RouteNode& routeNode,
                                     size_t pathIndex)
  {

    RoutingProfileRef profile=profiles[database];
    RouterDBFilesRef dataFiles=routerFiles[database];
    return profile->CanUse(routeNode,dataFiles->objectVariantDataFile.GetData(),pathIndex);
  }

  bool MultiDBRoutingService::GetRouteNodesByOffset(const std::set<DBFileOffset> &routeNodeOffsets,
                                                    std::unordered_map<DBFileOffset,RouteNodeRef> &routeNodeMap)
  {
    std::unordered_map<DatabaseId,std::set<FileOffset>> offsetMap;
    for (const auto &offset:routeNodeOffsets){
      offsetMap[offset.database].insert(offset.offset);
    }
    for (const auto &entry:offsetMap){
      std::vector<RouteNodeRef> nodes;
      const std::set<FileOffset> &offsets=entry.second;
      if (!routerFiles[entry.first]->routeNodeDataFile.GetByOffset(offsets.begin(),
                                                                   offsets.end(),
                                                                   offsets.size(),
                                                                   nodes)){
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
    RouterDBFilesRef dataFiles=routerFiles[offset.database];
    return dataFiles->routeNodeDataFile.GetByOffset(offset.offset, node);
  }

  bool MultiDBRoutingService::GetRouteNodeOffset(const DatabaseId &database,
                                                 const Id &id,
                                                 FileOffset &offset)
  {
    return routerFiles[database]->routeNodeDataFile.GetOffset(id,offset);
  }

  bool MultiDBRoutingService::GetWayByOffset(const DBFileOffset &offset,
                                             WayRef &way)
  {    
    return databases[offset.database]->GetWayByOffset(offset.offset,way);
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
      if (!databases[entry.first]->GetWaysByOffset(entry.second,ways)){
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
    return databases[offset.database]->GetAreaByOffset(offset.offset,area);
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
      if (!databases[entry.first]->GetAreasByOffset(entry.second,areas)){
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
    for (auto &filesEntry:routerFiles){
      DatabaseId dbId=filesEntry.first;
      std::set<Id> nodeIds;

      for (const auto& routeEntry : route.Entries()) {
        if (routeEntry.GetCurrentNodeId()!=0 && routeEntry.GetDatabaseId()==dbId) {
          nodeIds.insert(routeEntry.GetCurrentNodeId());
        }
      }

      std::vector<JunctionRef> junctions;

      if (!filesEntry.second->junctionDataFile.Get(nodeIds,
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
        if (routeEntry.GetCurrentNodeId()!=0 && routeEntry.GetDatabaseId()==dbId) {
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
      if (!routerFiles[dbId]->routeNodeDataFile.GetOffset(id,offset)){
        log.Error() << "Failed to retrieve file offset";
        continue;
      }
      twins.push_back(DBFileOffset(dbId,offset));
    }
    return twins;
  }

  bool MultiDBRoutingService::GetRouteNode(const DatabaseId &database,
                                           const Id &id,
                                           RouteNodeRef &node)
  {
    RouterDBFilesRef dataFiles=routerFiles[database];
    return dataFiles->routeNodeDataFile.Get(id, node);
  }

  RoutingResult MultiDBRoutingService::CalculateRoute(const RoutePosition &start,
                                                      const RoutePosition &target,
                                                      const RoutingParameter &parameter)
  {
    RoutingResult result;

    if (start.GetDatabaseId()==target.GetDatabaseId()){
      auto it=services.find(start.GetDatabaseId());
      if (it==services.end()){
        return result;
      }
      SimpleRoutingServiceRef service=it->second;
      return service->CalculateRoute(*(profiles.at(it->first)),start,target,parameter);
    }

    // start and target databases are different, try to find common route nodes
    auto it=databases.find(start.GetDatabaseId());
    if (it==databases.end()){
      log.Error() << "Can't find start database " << start.GetDatabaseId();
      return result;
    }
    DatabaseRef database1=it->second;

    it=databases.find(target.GetDatabaseId());
    if (it==databases.end()){
      log.Error() << "Can't find target database " << target.GetDatabaseId();
      return result;
    }
    DatabaseRef database2=it->second;

    std::set<Id> commonRouteNodes;
    if (!FindCommonRoutingNodes(parameter.GetBreaker(),database1,database2,commonRouteNodes)){
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
                              profiles[start.GetDatabaseId()],
                              profiles[target.GetDatabaseId()],
                              commonRouteNodes);
    
    return AbstractRoutingService<MultiDBRoutingState>::CalculateRoute(state,
                                                                       start,
                                                                       target,
                                                                       parameter);
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
    if (!routePostprocessor.PostprocessRouteDescription(description,
                                                        profiles,
                                                        databases,
                                                        postprocessors,
                                                        motorwayTypeNames,
                                                        motorwayLinkTypeNames,
                                                        junctionTypeNames)) {
      return false;
    }

    return true;
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
