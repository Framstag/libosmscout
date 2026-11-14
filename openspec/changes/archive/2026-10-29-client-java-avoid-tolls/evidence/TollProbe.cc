/*
  TollProbe — change client-java-avoid-tolls.

  Three questions, one run:

  1. Does the database carry the toll signal at all? The census prints how many object variants
     record a toll and the grade histogram of the low bits, so "the flag changed nothing" can be told
     apart from "the database knows nothing".
  2. Does avoidTolls change a route? The same grid is calculated twice: once with the flag off, once
     with it on.
  3. Is a route lost? A toll way must never make its own route unroutable.

  Run against two databases: one imported by an importer that records the signal, one imported before
  it existed. The second must load, route, and show no toll variants.

  Usage: TollProbe <database directory> [grid steps]
*/

#include <osmscout/db/Database.h>
#include <osmscout/db/ObjectVariantDataFile.h>

#include <osmscout/routing/SimpleRoutingService.h>

#include <osmscout/util/Time.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

  // The car speed map the bridge uses (OSMScoutClient.cpp, CreateRoutingProfile). The probe compares
  // two profiles that differ only in the avoidTolls flag.
  std::map<std::string,double> CarSpeedMap()
  {
    return {
      {"highway_motorway",110.0},
      {"highway_motorway_trunk",100.0},
      {"highway_motorway_primary",70.0},
      {"highway_motorway_link",60.0},
      {"highway_motorway_junction",60.0},
      {"highway_trunk",100.0},
      {"highway_trunk_link",60.0},
      {"highway_primary",70.0},
      {"highway_primary_link",60.0},
      {"highway_secondary",60.0},
      {"highway_secondary_link",50.0},
      {"highway_tertiary_link",55.0},
      {"highway_tertiary",55.0},
      {"highway_unclassified",50.0},
      {"highway_road",50.0},
      {"highway_residential",20.0},
      {"highway_roundabout",40.0},
      {"highway_living_street",10.0},
      {"highway_service",30.0}
    };
  }

  constexpr double carMaxSpeed=120.0;

  struct Pair
  {
    double startLat;
    double startLon;
    double destLat;
    double destLon;
  };

  struct Observation
  {
    bool        success=false;
    std::string failure;
    double      distance=0.0;
    double      pathLength=0.0;
    size_t      points=0;
    size_t      entries=0;
    size_t      tollEntries=0;
  };

  Observation Route(const osmscout::SimpleRoutingServiceRef& router,
                    osmscout::FastestPathRoutingProfile& profile,
                    const Pair& pair,
                    bool withGeometry)
  {
    Observation observation;

    auto startResult=router->GetClosestRoutableNode(osmscout::GeoCoord(pair.startLat,pair.startLon),
                                                    profile,
                                                    osmscout::Kilometers(1.0));
    auto targetResult=router->GetClosestRoutableNode(osmscout::GeoCoord(pair.destLat,pair.destLon),
                                                     profile,
                                                     osmscout::Kilometers(1.0));

    if (!startResult.IsValid() || !targetResult.IsValid()) {
      observation.failure="no routable node near " +
                          std::string(!startResult.IsValid() ? "start" : "target");
      return observation;
    }

    osmscout::RoutingParameter parameter;

    auto result=router->CalculateRoute(profile,
                                       startResult.GetRoutePosition(),
                                       targetResult.GetRoutePosition(),
                                       std::nullopt,
                                       parameter);

    if (!result.Success()) {
      observation.failure="routing error " + std::to_string(static_cast<int>(result.GetError()));
      return observation;
    }

    observation.success=true;
    observation.distance=result.GetOverallDistance().AsMeter();
    observation.entries=result.GetRoute().Entries().size();

    // The result's overall distance is the search's span, not the length of the chosen route, so the
    // geometry is transformed and measured: that is what tells the toll shortcut from the detour. The
    // transform asserts on some degenerate routes, so it is opt-in.
    if (withGeometry) {
      auto pointsResult=router->TransformRouteDataToPoints(result.GetRoute());

      if (pointsResult.Success()) {
        const auto points=pointsResult.GetPoints();

        observation.points=points->points.size();

        for (size_t i=1; i<observation.points; i++) {
          observation.pathLength+=osmscout::GetEllipsoidalDistance(points->points[i-1].GetCoord(),
                                                                   points->points[i].GetCoord()).AsMeter();
        }
      }
    }

    return observation;
  }

  void PrintObservation(const std::string& name,
                        const Observation& observation)
  {
    if (!observation.success) {
      std::cout << "  " << std::left << std::setw(10) << name << " -> no route ("
                << observation.failure << ")" << std::endl;
      return;
    }

    std::cout << "  " << std::left << std::setw(10) << name
              << " path=" << std::fixed << std::setprecision(1) << observation.pathLength << "m"
              << " points=" << observation.points
              << " entries=" << observation.entries << std::endl;
  }
}

int main(int argc,char* argv[])
{
  if (argc<2 || argc>4) {
    std::cerr << "usage: TollProbe <database directory> [grid steps] [geometry]" << std::endl;
    return 2;
  }

  const std::string directory=argv[1];
  const size_t      gridSteps=argc==3 ? std::max(2,std::atoi(argv[2])) : 6;
  const bool        withGeometry=argc>=4 && std::string(argv[3])=="geometry";

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(directory)) {
    std::cerr << "cannot open database '" << directory << "'" << std::endl;
    return 1;
  }

  osmscout::GeoBox boundingBox;

  if (database->GetBoundingBox(boundingBox)) {
    std::cout << "bounding box: "
              << boundingBox.GetMinCoord().GetLat() << "," << boundingBox.GetMinCoord().GetLon()
              << " .. "
              << boundingBox.GetMaxCoord().GetLat() << "," << boundingBox.GetMaxCoord().GetLon()
              << std::endl;
  }

  // 1. The census.
  osmscout::ObjectVariantDataFile objectVariantDataFile;

  bool loaded=objectVariantDataFile.Load(*database->GetTypeConfig(),directory+"/router2.dat");

  if (!loaded) {
    loaded=objectVariantDataFile.Load(*database->GetTypeConfig(),directory+"/route2.dat");
  }

  if (!loaded) {
    std::cerr << "cannot load the object variant data — is the routing index built?" << std::endl;
    return 1;
  }

  std::array<size_t,6> byGrade{};
  size_t               tollVariants=0;

  for (const auto& variant : objectVariantDataFile.GetData()) {
    if (variant.IsToll()) {
      tollVariants++;
    }

    const uint8_t grade=variant.GetGrade();

    if (grade>=1 && grade<=5) {
      byGrade[grade]++;
    }
  }

  std::cout << "object variants: " << objectVariantDataFile.GetData().size()
            << "  recording a toll: " << tollVariants
            << "  grades: g1=" << byGrade[1] << " g2=" << byGrade[2] << " g3=" << byGrade[3]
            << " g4=" << byGrade[4] << " g5=" << byGrade[5] << std::endl;

  // 2. Does the flag change a route?
  osmscout::RouterParameter routerParameter;
  auto router=std::make_shared<osmscout::SimpleRoutingService>(database,
                                                               routerParameter,
                                                               osmscout::RoutingService::DEFAULT_FILENAME_BASE);

  if (!router->Open()) {
    std::cerr << "cannot open routing service" << std::endl;
    return 1;
  }

  const osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();
  const auto                    speedMap=CarSpeedMap();

  osmscout::FastestPathRoutingProfileRef plainProfile=
    std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);
  plainProfile->ParametrizeForCar(*typeConfig,speedMap,carMaxSpeed);

  osmscout::FastestPathRoutingProfileRef tollProfile=
    std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);
  tollProfile->ParametrizeForCar(*typeConfig,speedMap,carMaxSpeed);
  tollProfile->SetAvoidTolls(true);

  std::cout << "toll penalty factor: " << tollProfile->GetTollPenaltyFactor() << std::endl;

  const double minLat=boundingBox.GetMinCoord().GetLat();
  const double maxLat=boundingBox.GetMaxCoord().GetLat();
  const double minLon=boundingBox.GetMinCoord().GetLon();
  const double maxLon=boundingBox.GetMaxCoord().GetLon();
  const double latStep=(maxLat-minLat)/gridSteps;
  const double lonStep=(maxLon-minLon)/gridSteps;

  size_t routed=0;
  size_t differing=0;
  size_t lost=0;
  size_t examples=0;

  for (size_t i=0; i<gridSteps; i++) {
    for (size_t j=0; j<gridSteps; j++) {
      Pair pair{minLat+latStep*(i+0.5),minLon+lonStep*(j+0.5),
                minLat+latStep*(i+1.5),minLon+lonStep*(j+1.5)};

      auto plain=Route(router,*plainProfile,pair,withGeometry);

      if (!plain.success) {
        continue;
      }

      routed++;

      auto toll=Route(router,*tollProfile,pair,withGeometry);

      const bool differs=!toll.success ||
                         plain.entries!=toll.entries ||
                         plain.points!=toll.points ||
                         std::abs(plain.pathLength-toll.pathLength)>1.0;

      if (differs) {
        differing++;
      }
      if (!toll.success) {
        lost++;
      }

      if (differs && examples<3) {
        examples++;
        std::cout << "  differing pair (" << std::fixed << std::setprecision(4)
                  << pair.startLat << "," << pair.startLon << " -> "
                  << pair.destLat << "," << pair.destLon << ")" << std::endl;
        PrintObservation("plain",plain);
        PrintObservation("avoidTolls",toll);
      }
    }
  }

  std::cout << "pairs routed: " << routed << ", differing with avoidTolls: " << differing
            << ", lost: " << lost << std::endl;

  // The corner pairs of the bounding box: on a hand-written map the interesting route is between two
  // named places at the box's corners, which a grid of small hops need not contain.
  const std::vector<Pair> corners{
    {minLat,minLon,minLat,maxLon},
    {minLat,maxLon,minLat,minLon},
    {minLat,minLon,maxLat,maxLon},
    {maxLat,minLon,minLat,maxLon}
  };

  for (const auto& pair : corners) {
    auto plain=Route(router,*plainProfile,pair,withGeometry);
    auto toll=Route(router,*tollProfile,pair,withGeometry);

    std::cout << "corner pair (" << std::fixed << std::setprecision(4)
              << pair.startLat << "," << pair.startLon << " -> "
              << pair.destLat << "," << pair.destLon << ")" << std::endl;
    PrintObservation("plain",plain);
    PrintObservation("avoidTolls",toll);
  }

  router->Close();
  database->Close();

  return 0;
}
