/*
  MultiDBRouting - a test program for libosmscout
  Copyright (C) 2016  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include <osmscout/Database.h>
#include <osmscout/Pixel.h>
#include <osmscout/RoutingService.h>

#include <osmscout/util/FileScanner.h>

const double cellMagnification=std::pow(2,16);
const double latCellFactor=180.0/cellMagnification;
const double lonCellFactor=360.0/cellMagnification;

osmscout::Pixel GetCell(const osmscout::GeoCoord& coord)
{
  return osmscout::Pixel(uint32_t((coord.GetLon()+180.0)/lonCellFactor),
                         uint32_t((coord.GetLat()+90.0)/latCellFactor));
}

bool ReadCellsForRoutingTree(osmscout::Database& database,
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

bool ReadRouteNodesForCells(osmscout::Database& database,
                            std::unordered_set<uint64_t>& cells,
                            std::unordered_set<osmscout::Id>& routeNodes)
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

int main(int argc, char* argv[])
{
  if (argc!=3) {
    std::cerr << "MultiDBRouting <database directory1> <database directory2>" << std::endl;

    return 1;
  }

  // Database

  osmscout::DatabaseParameter dbParameter;
  osmscout::DatabaseRef       database1=std::make_shared<osmscout::Database>(dbParameter);
  osmscout::DatabaseRef       database2=std::make_shared<osmscout::Database>(dbParameter);
  osmscout::RouterParameter   routerParameter;

  routerParameter.SetDebugPerformance(true);

  std::cout << "Opening database 1..." << std::endl;

  if (!database1->Open(argv[1])) {
    std::cerr << "Cannot open database 1" << std::endl;

    return 1;
  }

  std::cout << "Done." << std::endl;

  std::cout << "Opening database 2..." << std::endl;

  if (!database2->Open(argv[2])) {
    std::cerr << "Cannot open database 2" << std::endl;

    return 1;
  }

  std::cout << "Done." << std::endl;

  osmscout::RoutingServiceRef routingService1=std::make_shared<osmscout::RoutingService>(database1,
                                                                                         routerParameter,
                                                                                         osmscout::RoutingService::DEFAULT_FILENAME_BASE);
  osmscout::RoutingServiceRef routingService2=std::make_shared<osmscout::RoutingService>(database2,
                                                                                         routerParameter,
                                                                                         osmscout::RoutingService::DEFAULT_FILENAME_BASE);


  std::cout << "Opening RoutingService 1..." << std::endl;

  if (!routingService1->Open()) {
    std::cerr << "Cannot open RoutingService 1" << std::endl;

    return 1;
  }

  std::cout << "Done." << std::endl;

  std::cout << "Opening RoutingService 2..." << std::endl;

  if (!routingService2->Open()) {
    std::cerr << "Cannot open RoutingService 2" << std::endl;

    return 1;
  }

  std::cout << "Done." << std::endl;

  std::unordered_set<uint64_t> cells1;
  std::unordered_set<uint64_t> cells2;

  std::cout << "Reading route node cells of database 1..." << std::endl;

  ReadCellsForRoutingTree(*database1,
                          cells1);

  std::cout << "Found route nodes in " << cells1.size() << " cells" << std::endl;

  std::cout << "Reading route node cells of database 2..." << std::endl;

  ReadCellsForRoutingTree(*database2,
                          cells2);

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

  std::unordered_set<osmscout::Id> routeNodes1;
  std::unordered_set<osmscout::Id> routeNodes2;

  std::cout << "Reading route nodes in common cells of database 1..." << std::endl;

  ReadRouteNodesForCells(*database1,
                         commonCells,
                         routeNodes1);

  std::cout << "Found " << routeNodes1.size() << " route nodes in common cells" << std::endl;

  std::cout << "Reading route nodes in common cells of database 2..." << std::endl;

  ReadRouteNodesForCells(*database2,
                         commonCells,
                         routeNodes2);

  std::cout << "Found " << routeNodes2.size() << " route nodes in common cells" << std::endl;

  std::cout << "Detecting common route nodes..." << std::endl;

  std::set<osmscout::Id> commonRouteNodes;

  for (const auto routeNode : routeNodes1) {
    if (routeNodes2.find(routeNode)!=routeNodes2.end()) {
      commonRouteNodes.insert(routeNode);
    }
  }

  routeNodes1.clear();
  routeNodes2.clear();

  std::cout << "There are " << commonRouteNodes.size() << " common route nodes" << std::endl;

  osmscout::IndexedDataFile<osmscout::Id,osmscout::RouteNode> routeNodeFile(std::string(osmscout::RoutingService::DEFAULT_FILENAME_BASE)+".dat",
                                                                            std::string(osmscout::RoutingService::DEFAULT_FILENAME_BASE)+".idx",
                                                                            12000);

  std::cout << "Opening routing database 1..." << std::endl;

  if (!routeNodeFile.Open(database1->GetTypeConfig(),
                          database1->GetPath(),
                          true,
                          true)) {
    std::cerr << "Cannot open routing database" << std::endl;
  }

  std::unordered_map<osmscout::Id,osmscout::RouteNodeRef> routeNodeEntries;

  if (!routeNodeFile.Get(commonRouteNodes,
                         routeNodeEntries)) {
    std::cerr << "Cannot retrieve route nodes" << std::endl;
  }

  for (const auto& routeNodeEntry : routeNodeEntries) {
    std::cout << routeNodeEntry.second->GetId() << " " << routeNodeEntry.second->GetCoord().GetDisplayText() << std::endl;
  }

  routeNodeFile.Close();

  std::cout << "Closing RoutingServices and databases..." << std::endl;

  routingService1->Close();
  routingService1=NULL;
  database1->Close();
  database1=NULL;

  routingService2->Close();
  routingService2=NULL;
  database2->Close();
  database2=NULL;

  std::cout << "Done." << std::endl;

  return 0;
}
