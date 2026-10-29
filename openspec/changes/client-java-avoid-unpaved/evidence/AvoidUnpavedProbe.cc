/*
  AvoidUnpavedProbe — change client-java-avoid-unpaved.

  Two questions, one run:

  1. Does the routing index of a real imported database carry grade data at all?
     The bridge's avoidUnpaved no-op rests on the assumption that "grade" either is
     not imported or is not consulted; ObjectVariantData carries it and
     FastestPathRoutingProfile reads it, so the census is the fact to record.

  2. Does a grade-aware speed table change a calculated route?
     "baseline" builds the profile exactly as CreateRoutingProfile does today
     (AddType(type, speed) => SpeedVariant::Fill(speed), every grade the same
     speed). "graded" re-adds the same types with a per-grade table, which is what
     avoidUnpaved will do. If no candidate pair differs, the mechanism is not
     reachable through the profile and the change has no basis.

  Usage: AvoidUnpavedProbe <database directory>
*/

#include <osmscout/Way.h>
#include <osmscout/db/Database.h>
#include <osmscout/db/ObjectVariantDataFile.h>

#include <osmscout/feature/GradeFeature.h>

#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/util/Time.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

  // Speed factor per grade (index 0..4 => grade 1..5), applied when avoidUnpaved is
  // requested. The first probe run showed that an aggressive table (0.6/0.3/0.1) makes a
  // 4 km route cost more than the profile's cost limit (10 km / factor 5.0,
  // RoutingProfile.cpp:37-38) and the calculation fails with NoRouteFound: "avoid" must not
  // become "cannot route". The tables below are the candidates to choose from.
  constexpr size_t maxFactors=4;

  struct Table
  {
    const char *name;
    std::array<double,5> factors;
    double minSpeedKmh;
  };

  const std::array<Table,maxFactors> tables{{
    {"aggressive",  {1.0,0.85,0.6,0.3,0.1}, 3.0},
    {"moderate",    {1.0,0.9,0.75,0.5,0.3}, 5.0},
    {"gentle",      {1.0,1.0,0.85,0.6,0.4}, 5.0},
    {"very-gentle", {1.0,1.0,0.9,0.75,0.5}, 8.0}
  }};

  osmscout::SpeedVariant GradedVariant(double baseSpeed,
                                      const Table& table)
  {
    osmscout::SpeedVariant variant;

    for (size_t i=0; i<5; i++) {
      variant.speed[i]=std::max(table.minSpeedKmh,baseSpeed*table.factors[i]);
    }

    return variant;
  }

  // The car speed map CreateRoutingProfile uses today, kept in step with it on purpose:
  // the probe compares two profile shapes, not two speed tables.
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

  void ParametrizeGraded(osmscout::FastestPathRoutingProfile& profile,
                         const osmscout::TypeConfig& typeConfig,
                         const std::map<std::string,double>& speedMap,
                         double maxSpeed,
                         const Table& table)
  {
    // Run the normal parametrization first: the FastestPathRoutingProfile override also enables
    // the junction penalty and doubles the cost limit and factor (RoutingProfile.h:390-401).
    // Rebuilding the type list by hand left those unset, and the first probe run failed with
    // NoRouteFound on exactly the pairs whose route needs a high-grade way.
    profile.ParametrizeForCar(typeConfig,speedMap,maxSpeed);

    // Then re-add the same types with a per-grade table. AddType replaces the type's speed entry
    // and keeps the profile-level setup intact.
    for (const auto& type : typeConfig.GetTypes()) {
      if (type->GetIgnore() || !type->CanRouteCar()) {
        continue;
      }

      auto speed=speedMap.find(type->GetName());

      if (speed==speedMap.end()) {
        continue;
      }

      profile.AddType(type,GradedVariant(std::min(speed->second,maxSpeed),table));
    }
  }

  void ParametrizeBaseline(osmscout::FastestPathRoutingProfile& profile,
                           const osmscout::TypeConfig& typeConfig,
                           const std::map<std::string,double>& speedMap,
                           double maxSpeed)
  {
    profile.ParametrizeForCar(typeConfig,speedMap,maxSpeed);
  }

  struct Pair
  {
    double startLat;
    double startLon;
    double destLat;
    double destLon;
  };

  std::vector<Pair> GridPairs()
  {
    std::vector<Pair> pairs;

    // The extract's bounding box is padded (52.71,7.89 .. 54.29,9.15) while its coverage is two
    // cities, so a coarse box-wide grid finds almost nothing routable. Walk the covered areas
    // densely instead: Bremen city, the rural north (Blockland/Wümme tracks, where the grade 3+
    // ways are) and Bremerhaven.
    for (double lat : {53.03,53.06,53.09,53.12,53.15,53.18}) {
      for (double lon : {8.65,8.70,8.75,8.80,8.85,8.90,8.95}) {
        pairs.push_back({lat,lon,lat+0.03,lon+0.03});
      }
    }

    for (double lat : {53.52,53.55,53.58}) {
      for (double lon : {8.54,8.58,8.62}) {
        pairs.push_back({lat,lon,lat+0.03,lon+0.03});
      }
    }

    return pairs;
  }

  struct Observation
  {
    bool                  success=false;
    std::string           failure;
    double                distance=0.0;
    double                duration=0.0;
    size_t                entries=0;
    std::map<std::string,size_t> types;
  };

  Observation Route(const osmscout::SimpleRoutingServiceRef& router,
                    osmscout::FastestPathRoutingProfile& profile,
                    const Pair& pair)
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

    const auto& route=result.GetRoute();

    observation.success=true;
    observation.distance=result.GetOverallDistance().AsMeter();
    observation.duration=0.0;
    observation.entries=route.Entries().size();

    for (const auto& entry : route.Entries()) {
      observation.types[entry.GetPathObject().GetName()]++;
    }

    return observation;
  }

  // One kilometre on a way of the given type and grade, as the profile prices it. This is the
  // measurement the change's spec scenario can assert: it needs no route, no coordinates and no
  // map extract, only a profile and a type.
  // One kilometre of a way of the given type and grade, as the profile prices it. This is the
  // measurement the change's spec scenario asserts: no route, no coordinates, no map extract —
  // just the profile, a type and a grade.
  double SecondsForGrade(const osmscout::RoutingProfile& profile,
                         const osmscout::TypeInfoRef& type,
                         uint8_t grade)
  {
    osmscout::FeatureValueBuffer buffer;

    buffer.SetType(type);

    for (size_t i=0; i<buffer.GetFeatureCount(); i++) {
      auto feature=buffer.GetFeature(i);

      if (feature.GetFeature()->GetName()!=std::string(osmscout::GradeFeature::NAME)) {
        continue;
      }

      auto *value=dynamic_cast<osmscout::GradeFeatureValue*>(buffer.AllocateValue(i));

      if (value!=nullptr) {
        value->SetGrade(grade);
      }
      break;
    }

    osmscout::Way way;

    way.SetType(type);
    way.SetFeatures(buffer);

    return osmscout::DurationAsHours(profile.GetTime(way,osmscout::Kilometers(1.0)))*3600.0;
  }

  void PrintObservation(const std::string& name,
                        const Observation& observation)
  {
    if (!observation.success) {
      std::cout << "  " << std::left << std::setw(12) << name << " -> no route ("
                << observation.failure << ")" << std::endl;
      return;
    }

    std::ostringstream types;

    types << observation.types.size() << " distinct";

    size_t shown=0;
    for (const auto& [type,count] : observation.types) {
      if (shown++>=4) {
        types << ",...";
        break;
      }
      types << (shown==1 ? " " : ",") << count << 'x' << type;
    }

    std::cout << "  " << std::left << std::setw(12) << name
              << " distance=" << std::fixed << std::setprecision(1) << observation.distance << "m"
              << " duration=" << std::setprecision(1) << observation.duration << "s"
              << " entries=" << observation.entries
              << " types=[" << types.str() << "]"
              << std::endl;
  }
}

int main(int argc,char* argv[])
{
  if (argc!=2) {
    std::cerr << "usage: AvoidUnpavedProbe <database directory>" << std::endl;
    return 2;
  }

  const std::string directory=argv[1];

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

  // 1. The census: can the profile even see a grade? RoutingDatabase is not exported
  //    (hidden visibility), so the index is read through ObjectVariantDataFile, which is.
  //    The object variant file is <router filename base>2.dat: 'router2.dat' for the default
  //    'router' base, 'route2.dat' for a database imported with a 'route' base.
  osmscout::ObjectVariantDataFile objectVariantDataFile;

  bool loaded=objectVariantDataFile.Load(*database->GetTypeConfig(),directory+"/router2.dat");

  if (!loaded) {
    loaded=objectVariantDataFile.Load(*database->GetTypeConfig(),directory+"/route2.dat");
  }

  if (!loaded) {
    std::cerr << "cannot load the object variant data (router2.dat) — is the routing index built?"
              << std::endl;
    return 1;
  }

  std::array<size_t,6> byGrade{};
  size_t               aboveGrade5=0;

  for (const auto& variant : objectVariantDataFile.GetData()) {
    if (variant.grade>=1 && variant.grade<=5) {
      byGrade[variant.grade]++;
    }
    if (variant.grade>5) {
      aboveGrade5++;
    }
  }

  std::cout << "object variants: " << objectVariantDataFile.GetData().size()
            << "  grade1=" << byGrade[1]
            << " grade2=" << byGrade[2]
            << " grade3=" << byGrade[3]
            << " grade4=" << byGrade[4]
            << " grade5=" << byGrade[5]
            << " above-grade5=" << aboveGrade5
            << std::endl;

  // 2. Does a graded table change a route?
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
  const double                  carMaxSpeed=120.0;

  // 2. The object-level measurement: what one kilometre of a track costs at each grade.
  // The type must be one the car speed map parametrizes, or its speed entry is 0 and every time is
  // infinite: the bridge's car map has no highway_track entry, so tracks are not car-routable at all.
  const std::vector<std::string> gradeCarTypes{
    "highway_unclassified","highway_residential","highway_service","highway_tertiary"
  };

  osmscout::TypeInfoRef trackType=nullptr;

  for (const auto& name : gradeCarTypes) {
    trackType=typeConfig->GetTypeInfo(name);
    if (trackType!=nullptr) {
      break;
    }
  }

  if (trackType!=nullptr) {
    osmscout::FastestPathRoutingProfileRef baselineProfile=
      std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);
    ParametrizeBaseline(*baselineProfile,*typeConfig,speedMap,carMaxSpeed);

    std::cout << "1 km of " << trackType->GetName() << " (seconds), baseline:";
    for (uint8_t grade=1; grade<=5; grade++) {
      std::cout << " g" << int(grade) << "=" << std::fixed << std::setprecision(1)
                << SecondsForGrade(*baselineProfile,trackType,grade);
    }
    std::cout << std::endl;

    for (const auto& table : tables) {
      osmscout::FastestPathRoutingProfileRef profile=
        std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);
      ParametrizeGraded(*profile,*typeConfig,speedMap,carMaxSpeed,table);

      std::cout << "1 km of " << trackType->GetName() << " (seconds), " << table.name << "  :";
      for (uint8_t grade=1; grade<=5; grade++) {
        std::cout << " g" << int(grade) << "=" << std::fixed << std::setprecision(1)
                  << SecondsForGrade(*profile,trackType,grade);
      }
      std::cout << std::endl;
    }

    std::cout << std::endl;
  }

  const std::vector<Pair> pairs=GridPairs();

  // Baseline once per pair: the route the bridge calculates today.
  std::vector<Observation> baseline(pairs.size());
  size_t                   baselineRouted=0;

  for (size_t i=0; i<pairs.size(); i++) {
    osmscout::FastestPathRoutingProfileRef profile=
      std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);
    ParametrizeBaseline(*profile,*typeConfig,speedMap,carMaxSpeed);

    baseline[i]=Route(router,*profile,pairs[i]);

    if (baseline[i].success) {
      baselineRouted++;
    }
  }

  std::cout << "pairs: " << pairs.size() << ", routed by the baseline profile: " << baselineRouted
            << std::endl;

  for (const auto& table : tables) {
    size_t differing=0;
    size_t lost=0;
    size_t examples=0;

    for (size_t i=0; i<pairs.size(); i++) {
      if (!baseline[i].success) {
        continue;
      }

      osmscout::FastestPathRoutingProfileRef profile=
        std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);
      ParametrizeGraded(*profile,*typeConfig,speedMap,carMaxSpeed,table);

      auto graded=Route(router,*profile,pairs[i]);

      const bool differs=!graded.success ||
                         baseline[i].entries!=graded.entries ||
                         baseline[i].types!=graded.types ||
                         std::abs(baseline[i].distance-graded.distance)>1.0;

      if (differs) {
        differing++;
      }
      if (!graded.success) {
        lost++;
      }

      if (differs && examples<2) {
        examples++;
        std::cout << "  " << table.name << " example pair " << i
                  << " (" << pairs[i].startLat << "," << pairs[i].startLon << " -> "
                  << pairs[i].destLat << "," << pairs[i].destLon << ")" << std::endl;
        PrintObservation("baseline",baseline[i]);
        PrintObservation("graded",graded);
      }
    }

    std::cout << table.name << ": differing=" << differing
              << " of " << baselineRouted << " routed pairs, lost (no route)=" << lost
              << std::endl << std::endl;
  }

  router->Close();
  database->Close();

  return 0;
}
