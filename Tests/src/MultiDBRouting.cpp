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
#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/MultiDBRoutingService.h>

#include <osmscout/util/FileScanner.h>

void GetCarSpeedTable(std::map<std::string,double>& map)
{
  map["highway_motorway"]=110.0;
  map["highway_motorway_trunk"]=100.0;
  map["highway_motorway_primary"]=70.0;
  map["highway_motorway_link"]=60.0;
  map["highway_motorway_junction"]=60.0;
  map["highway_trunk"]=100.0;
  map["highway_trunk_link"]=60.0;
  map["highway_primary"]=70.0;
  map["highway_primary_link"]=60.0;
  map["highway_secondary"]=60.0;
  map["highway_secondary_link"]=50.0;
  map["highway_tertiary"]=55.0;
  map["highway_tertiary_link"]=55.0;
  map["highway_unclassified"]=50.0;
  map["highway_road"]=50.0;
  map["highway_residential"]=40.0;
  map["highway_roundabout"]=40.0;
  map["highway_living_street"]=10.0;
  map["highway_service"]=30.0;
}

int main(int argc, char* argv[])
{
  if (argc!=7) {
    std::cerr << "MultiDBRouting" << std::endl;
    std::cerr << "  <database directory1> <database directory2>" << std::endl;
    std::cerr << "  <start lat> <start lon>" << std::endl;
    std::cerr << "  <target lat> <target lon>" << std::endl;

    return 1;
  }

  double  startLat;
  double  startLon;

  double  targetLat;
  double  targetLon;

  if (sscanf(argv[3],"%lf",&startLat)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }
  if (sscanf(argv[4],"%lf",&startLon)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }
  if (sscanf(argv[5],"%lf",&targetLat)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }
  if (sscanf(argv[6],"%lf",&targetLon)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }

  osmscout::GeoCoord startCoord(startLat,startLon);
  osmscout::GeoCoord targetCoord(targetLat,targetLon);

  // Database

  osmscout::DatabaseParameter dbParameter;
  osmscout::DatabaseRef       database1=std::make_shared<osmscout::Database>(dbParameter);
  osmscout::DatabaseRef       database2=std::make_shared<osmscout::Database>(dbParameter);
  osmscout::RouterParameter   routerParameter;

  osmscout::log.Debug(true);
  osmscout::log.Info(true);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);

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

  std::vector<osmscout::DatabaseRef> databases(2);
  databases[0]=database1;
  databases[1]=database2;

  osmscout::RouterParameter routerParam;
  routerParam.SetDebugPerformance(true);
  osmscout::MultiDBRoutingServiceRef router=std::make_shared<osmscout::MultiDBRoutingService>(routerParam,databases);

  std::cout << "Opening router..." << std::endl;

  osmscout::MultiDBRoutingService::RoutingProfileBuilder profileBuilder=
      [](const osmscout::DatabaseRef &database){
        auto profile=std::make_shared<osmscout::FastestPathRoutingProfile>(database->GetTypeConfig());
        std::map<std::string,double> speedMap;
        GetCarSpeedTable(speedMap);
        profile->ParametrizeForCar(*(database->GetTypeConfig()),speedMap,160.0);
        return profile;
      };

  if (!router->Open(profileBuilder)) {
    
    std::cerr << "Cannot open router" << std::endl;
    return 1;
  }
  std::cout << "Done." << std::endl;

  osmscout::RoutePosition startNode=router->GetClosestRoutableNode(startCoord);
  if (!startNode.IsValid()){
    std::cerr << "Can't found route node near start coord " << startCoord.GetDisplayText() << std::endl;
    return 1;
  }
  osmscout::RoutePosition targetNode=router->GetClosestRoutableNode(targetCoord);
  if (!targetNode.IsValid()){
    std::cerr << "Can't found route node near target coord " << targetCoord.GetDisplayText() << std::endl;
    return 1;
  }

  osmscout::RoutingParameter parameter;
  osmscout::RoutingResult route=router->CalculateRoute(startNode,targetNode,parameter);
  if (!route.Success()){
    std::cerr << "Route failed" << std::endl;
    return 1;
  }

  osmscout::RouteDescription description;
  router->TransformRouteDataToRouteDescription(route.GetRoute(),description);

  std::cout << "Closing RoutingServices and databases..." << std::endl;

  router->Close();

  database1->Close();
  database1=NULL;

  database2->Close();
  database2=NULL;

  std::cout << "Done." << std::endl;

  return 0;
}
