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

#include <osmscout/RoutingService.h>
#include <osmscout/RoutingProfile.h>
#include <osmscout/MultiDBRouting.h>
#include <osmscout/Pixel.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING

#include <iomanip>
#include <iostream>

namespace osmscout {

  MultiDBRouting::MultiDBRouting(std::vector<DatabaseRef> databases):
    isOpen(false)
  {
    for (auto &db:databases){
      this->databases[db->GetPath()]=db;
    }
  }

  MultiDBRouting::~MultiDBRouting()
  {
    Close();
  }

  bool MultiDBRouting::Open(RoutingProfileBuilder profileBuilder,
                            RouterParameter routerParameter)
  {
    if (databases.empty()){
      return false;
    }

    isOpen=true;
    for (auto &entry:databases){
      RoutingServiceRef router=std::make_shared<osmscout::RoutingService>(entry.second,
                                                      routerParameter,
                                                      osmscout::RoutingService::DEFAULT_FILENAME_BASE);
      if (!router->Open()){
        Close();
        return false;
      }
      services[entry.first]=router;
      profiles[entry.first]=profileBuilder(entry.second);
    }
    return true;
  }

  void MultiDBRouting::Close(){
    if (!isOpen){
      return;
    }
    for (auto &entry:services){
      entry.second->Close();
    }
    services.clear();
    profiles.clear();
    isOpen=false;
  }

  RoutePosition MultiDBRouting::GetClosestRoutableNode(const GeoCoord& coord,
                                                       double radius,
                                                       std::string databaseHint) const
  {
    RoutePosition position;

    // first try to find in hinted database
    auto it=services.find(databaseHint);
    if (it!=services.end()){
      position=it->second->GetClosestRoutableNode(coord,*(profiles.at(it->first)),radius);
      if (position.IsValid()){
        return position;
      }
    }

    // try all databases
    for (auto &entry:services){
      if (entry.first==databaseHint){
        continue;
      }

      position=entry.second->GetClosestRoutableNode(coord,*(profiles.at(entry.first)),radius);
      if (position.IsValid()){
        return position;
      }
    }
    return position;
  }

  const double MultiDBRouting::CELL_MAGNIFICATION=std::pow(2,16);
  const double MultiDBRouting::LAT_CELL_FACTOR=180.0/ MultiDBRouting::CELL_MAGNIFICATION;
  const double MultiDBRouting::LON_CELL_FACTOR=360.0/ MultiDBRouting::CELL_MAGNIFICATION;

  Pixel MultiDBRouting::GetCell(const osmscout::GeoCoord& coord)
  {
    return osmscout::Pixel(uint32_t((coord.GetLon()+180.0)/ LON_CELL_FACTOR),
                           uint32_t((coord.GetLat()+90.0)/ LAT_CELL_FACTOR));
  }

  bool MultiDBRouting::ReadCellsForRoutingTree(osmscout::Database& database,
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
      std::cerr << "Error while reading '" << fullFilename << "': " << e.GetDescription() << std::endl;
      return false;
    }

    return true;
  }

  bool MultiDBRouting::ReadRouteNodesForCells(osmscout::Database& database,
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
      std::cerr << "Error while reading '" << fullFilename << "': " << e.GetDescription() << std::endl;
      return false;
    }

    return true;
  }

  bool MultiDBRouting::FindCommonRoutingNodes(DatabaseRef &database1,
                                              DatabaseRef &database2,
                                              std::set<Id> &commonRouteNodes)
  {

    std::unordered_set<uint64_t> cells1;
    std::unordered_set<uint64_t> cells2;

    std::cout << "Reading route node cells of database 1..." << std::endl;

    if (!ReadCellsForRoutingTree(*database1,
                                 cells1)){
      return false;
    }

    std::cout << "Found route nodes in " << cells1.size() << " cells" << std::endl;

    std::cout << "Reading route node cells of database 2..." << std::endl;

    if (!ReadCellsForRoutingTree(*database2,
                                 cells2)){
      return false;
    }

    std::cout << "Found route nodes in " << cells2.size() << " cells" << std::endl;

    std::cout << "Detecting common cells..." << std::endl;

    std::unordered_set<uint64_t> commonCells;

    for (const auto cell : cells1) {
      if (cells2.find(cell)!=cells2.end()) {
        commonCells.insert(cell);
      }
    }

    cells1.clear();
    cells2.clear();

    std::cout << "There are " << commonCells.size() << " common cells" << std::endl;

    std::unordered_set<Id> routeNodes1;
    std::unordered_set<Id> routeNodes2;

    std::cout << "Reading route nodes in common cells of database 1..." << std::endl;

    if (!ReadRouteNodesForCells(*database1,
                                commonCells,
                                routeNodes1)){
      return false;
    }

    std::cout << "Found " << routeNodes1.size() << " route nodes in common cells" << std::endl;

    std::cout << "Reading route nodes in common cells of database 2..." << std::endl;

    if (!ReadRouteNodesForCells(*database2,
                                commonCells,
                                routeNodes2)){
      return false;
    }

    std::cout << "Found " << routeNodes2.size() << " route nodes in common cells" << std::endl;

    std::cout << "Detecting common route nodes..." << std::endl;


    for (const auto routeNode : routeNodes1) {
      if (routeNodes2.find(routeNode)!=routeNodes2.end()) {
        commonRouteNodes.insert(routeNode);
      }
    }

    routeNodes1.clear();
    routeNodes2.clear();

    std::cout << "There are " << commonRouteNodes.size() << " common route nodes" << std::endl;
    return true;
  }

  bool MultiDBRouting::ReadRouteNodeEntries(DatabaseRef &database,
                                            std::set<Id> &commonRouteNodes,
                                            std::unordered_map<Id,osmscout::RouteNodeRef> &routeNodeEntries)
  {
    osmscout::IndexedDataFile<Id,osmscout::RouteNode> routeNodeFile(std::string(osmscout::RoutingService::DEFAULT_FILENAME_BASE)+".dat",
                                                                    std::string(osmscout::RoutingService::DEFAULT_FILENAME_BASE)+".idx",
                                                                    /*indexCacheSize*/12000,
                                                                    /*dataCacheSize*/1000);

    std::cout << "Opening routing database..." << std::endl;

    if (!routeNodeFile.Open(database->GetTypeConfig(),
                            database->GetPath(),
                            true,
                            true)) {
      std::cerr << "Cannot open routing database" << std::endl;
      return false;
    }



    if (!routeNodeFile.Get(commonRouteNodes,
                           routeNodeEntries)) {
      std::cerr << "Cannot retrieve route nodes" << std::endl;
      routeNodeFile.Close();
      return false;
    }
    routeNodeFile.Close();
    return true;
  }

  bool MultiDBRouting::CalculateRoute(const RoutePosition &start,
                                      const RoutePosition &target,
                                      const RoutingParameter &parameter)
  {
    if (start.GetDatabasePath()==target.GetDatabasePath()){
      auto it=services.find(start.GetDatabasePath());
      if (it==services.end()){
        return false;
      }
      RoutingServiceRef service=it->second;
      RoutingResult result=service->CalculateRoute(*(profiles.at(it->first)),start,target,parameter);
      return result.Success();
    }

    // start and target databases are different, try to find common route nodes
    auto it=databases.find(start.GetDatabasePath());
    if (it==databases.end()){
      std::cerr << "Can't find start database " << start.GetDatabasePath() << std::endl;
      return false;
    }
    DatabaseRef database1=it->second;

    it=databases.find(target.GetDatabasePath());
    if (it==databases.end()){
      std::cerr << "Can't find target database " << target.GetDatabasePath() << std::endl;
      return false;
    }
    DatabaseRef database2=it->second;

    std::set<Id> commonRouteNodes;
    if (!FindCommonRoutingNodes(database1,database2,commonRouteNodes)){
      std::cerr << "Can't find common routing nodes for databases " <<
        database1->GetPath() << ", " <<
        database2->GetPath() << std::endl;
      return false;
    }
    if (commonRouteNodes.empty()){
      std::cerr << "Can't find common routing nodes for databases " <<
        database1->GetPath() << ", " <<
        database2->GetPath() << std::endl;
      return false;
    }

    std::unordered_map<Id,osmscout::RouteNodeRef> routeNodeEntries1;
    if (!ReadRouteNodeEntries(database1,commonRouteNodes,routeNodeEntries1)){
      return false;
    }
    std::unordered_map<Id,osmscout::RouteNodeRef> routeNodeEntries2;
    if (!ReadRouteNodeEntries(database2,commonRouteNodes,routeNodeEntries2)){
      return false;
    }

    for (const auto& routeNodeEntry : routeNodeEntries1) {
      std::cout << routeNodeEntry.second->GetId() << " " << routeNodeEntry.second->GetCoord().GetDisplayText() << std::endl;
    }

    // first step is done
    // TODO: second step
    // third step: profit!

    std::cerr << "Not implemented yet" << std::endl;
    return false;
  }
}
