/*
 NavigationSimulator - a demo program for libosmscout
 Copyright (C) 2018  Tim Teulings

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

/*
  Examples for the nordrhein-westfalen.osm:

  Long: "In den Hüchten" Dortmund => Promenadenweg Bonn
    51.5717798 7.4587852  50.6890143 7.1360549

  TODOs:
  * The vehicle always has the maximum speed allowed. Acceleration and deaccelaration
    are not taken into account. This result in a little bit unrealistic GPS positions.
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <cstring>

#include <osmscout/Database.h>

#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>

#include <osmscout/util/CmdLineParsing.h>

struct Arguments
{
  bool                   help=false;
  std::string            router=osmscout::RoutingService::DEFAULT_FILENAME_BASE;
  osmscout::Vehicle      vehicle=osmscout::Vehicle::vehicleCar;
  std::string            databaseDirectory;
  osmscout::GeoCoord     start;
  osmscout::GeoCoord     target;
  std::string            gpxFile;
};

class ConsoleRoutingProgress : public osmscout::RoutingProgress
{
private:
  std::chrono::system_clock::time_point lastDump;
  double                                maxPercent;

public:
  ConsoleRoutingProgress()
    : lastDump(std::chrono::system_clock::now()),
      maxPercent(0.0)
  {
    // no code
  }

  void Reset() override
  {
    lastDump=std::chrono::system_clock::now();
    maxPercent=0.0;
  }

  void Progress(const osmscout::Distance &currentMaxDistance,
                const osmscout::Distance &overallDistance) override
  {
    double currentPercent=(currentMaxDistance.AsMeter()*100.0)/overallDistance.AsMeter();

    std::chrono::system_clock::time_point now=std::chrono::system_clock::now();

    maxPercent=std::max(maxPercent,currentPercent);

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now-lastDump).count()>500) {
      std::cout << (size_t)maxPercent << "%" << std::endl;

      lastDump=now;
    }
  }
};

struct RouteDescriptionGeneratorCallback : public osmscout::RouteDescriptionGenerator::Callback
{
};

static void GetCarSpeedTable(std::map<std::string,double>& map)
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
  map["highway_tertiary_link"]=55.0;
  map["highway_tertiary"]=55.0;
  map["highway_unclassified"]=50.0;
  map["highway_road"]=50.0;
  map["highway_residential"]=40.0;
  map["highway_roundabout"]=40.0;
  map["highway_living_street"]=10.0;
  map["highway_service"]=30.0;
}

class PathGenerator
{
public:
  struct Step
  {
    osmscout::Timestamp time;
    osmscout::GeoCoord  coord;

    Step(const osmscout::Timestamp& time,
         const osmscout::GeoCoord& coord)
    : time(time),
      coord(coord)
    {
      // no code
    }
  };

public:
  std::list<Step> steps;

public:
  PathGenerator(const osmscout::RouteDescription& description,
                double maxSpeed);
};

/**
 * Initialize the step list with way points on the route in strict
 * 1 second intervals.
 *
 * @param description
 *    Routing description
 * @param maxSpeed
 *    Max speed to use if no explicit speed limit in given on a route segment
 */
PathGenerator::PathGenerator(const osmscout::RouteDescription& description,
                             double maxSpeed)
{
  size_t             tickCount=0;
  double             totalTime=0.0;
  double             restTime=0.0;
  auto               currentNode=description.Nodes().begin();
  auto               nextNode=currentNode;
  osmscout::GeoCoord lastPosition;

  assert(currentNode!=description.Nodes().end());

  auto time=std::chrono::system_clock::now();

  lastPosition=currentNode->GetLocation();
  steps.emplace_back(time,lastPosition);

  time+=std::chrono::seconds(1);

  ++nextNode;

  while (nextNode!=description.Nodes().end()) {
    osmscout::RouteDescription::MaxSpeedDescriptionRef maxSpeedPath=std::dynamic_pointer_cast<osmscout::RouteDescription::MaxSpeedDescription>(currentNode->GetDescription(osmscout::RouteDescription::WAY_MAXSPEED_DESC));

    if (maxSpeedPath) {
      maxSpeed=maxSpeedPath->GetMaxSpeed();
    }

    osmscout::Distance distance=osmscout::GetEllipsoidalDistance(currentNode->GetLocation(),
                                                                 nextNode->GetLocation());
    double bearing=osmscout::GetSphericalBearingInitial(currentNode->GetLocation(),
                                                        nextNode->GetLocation());

    auto distanceInKilometer=distance.As<osmscout::Kilometer>();
    auto timeInHours=distanceInKilometer/maxSpeed;
    auto timeInSeconds=timeInHours*60*60;

    totalTime+=timeInHours;

    // Make sure we do not skip edges in the street
    lastPosition=currentNode->GetLocation();

    while (timeInSeconds>1.0-restTime) {
      timeInSeconds=timeInSeconds-(1.0-restTime);

      double segmentDistance=maxSpeed*(1.0-restTime)/(60*60);

      lastPosition=lastPosition.Add(bearing*180/M_PI,
                                    osmscout::Distance::Of<osmscout::Kilometer>(segmentDistance));

      steps.emplace_back(time,lastPosition);
      time+=std::chrono::seconds(1);

      restTime=0;

      tickCount++;
    }

    restTime=timeInSeconds;

    ++currentNode;
    ++nextNode;
  }

  steps.emplace_back(time,currentNode->GetLocation());
}

void DumpGpxFile(const std::string& fileName,
                 const std::list<osmscout::Point>& points,
                 const PathGenerator& generator)
{
  std::ofstream stream;

  std::cout << "Writing gpx file '" << fileName << "'..." << std::endl;

  stream.open(fileName,std::ofstream::trunc);

  if (!stream.is_open()) {
    std::cerr << "Cannot open gpx file!" << std::endl;
    return;
  }

  stream.precision(8);
  stream << R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)" << std::endl;
  stream << R"(<gpx xmlns="http://www.topografix.com/GPX/1/1" creator="bin2gpx" version="1.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">)"
            << std::endl;

  stream << "\t<wpt lat=\""<< generator.steps.front().coord.GetLat() << "\" lon=\""<< generator.steps.front().coord.GetLon() << "\">" << std::endl;
  stream << "\t\t<name>Start</name>" << std::endl;
  stream << "\t\t<fix>2d</fix>" << std::endl;
  stream << "\t</wpt>" << std::endl;

  stream << "\t<wpt lat=\""<< generator.steps.back().coord.GetLat() << "\" lon=\""<< generator.steps.back().coord.GetLon() << "\">" << std::endl;
  stream << "\t\t<name>Target</name>" << std::endl;
  stream << "\t\t<fix>2d</fix>" << std::endl;
  stream << "\t</wpt>" << std::endl;

  stream << "\t<rte>" << std::endl;
  stream << "\t\t<name>Route</name>" << std::endl;
  for (const auto &point : points) {
    stream << "\t\t\t<rtept lat=\""<< point.GetLat() << "\" lon=\""<< point.GetLon() <<"\">" << std::endl;
    stream << "\t\t\t</rtept>" << std::endl;
  }
  stream << "\t</rte>" << std::endl;

  stream << "\t<trk>" << std::endl;
  stream << "\t\t<name>GPS</name>" << std::endl;
  stream << "\t\t<trkseg>" << std::endl;
  for (const auto &point : generator.steps) {
    stream << "\t\t\t<trkpt lat=\""<< point.coord.GetLat() << "\" lon=\""<< point.coord.GetLon() <<"\">" << std::endl;
    stream << "\t\t\t\t<time>" << osmscout::TimestampToISO8601TimeString(point.time) << "</time>" << std::endl;
    stream << "\t\t\t\t<fix>2d</fix>" << std::endl;
    stream << "\t\t\t</trkpt>" << std::endl;
  }
  stream << "\t\t</trkseg>" << std::endl;
  stream << "\t</trk>" << std::endl;
  stream << "</gpx>" << std::endl;

  stream.close();

  std::cout << "Writing gpx file done." << std::endl;
}

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("NavigationSimulator",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineAlternativeFlag([&args](const std::string& value) {
                        if (value=="foot") {
                          args.vehicle=osmscout::Vehicle::vehicleFoot;
                        }
                        else if (value=="bicycle") {
                          args.vehicle=osmscout::Vehicle::vehicleBicycle;
                        }
                        else if (value=="car") {
                          args.vehicle=osmscout::Vehicle::vehicleCar;
                        }
                      }),
                      {"foot","bicycle","car"},
                      "Vehicle type to use for routing");

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.router=value;
                      }),
                      "router",
                      "Router filename base");

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.gpxFile=value;
                      }),
                      "gpxFile",
                      "Name of the gpx file containing the route");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the first database to use");

  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& value) {
                            args.start=value;
                          }),
                          "START",
                          "start coordinate");

  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& value) {
                            args.target=value;
                          }),
                          "TARGET",
                          "target coordinate");

  osmscout::CmdLineParseResult cmdLineParseResult=argParser.Parse();

  if (cmdLineParseResult.HasError()) {
    std::cerr << "ERROR: " << cmdLineParseResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::FastestPathRoutingProfileRef routingProfile=std::make_shared<osmscout::FastestPathRoutingProfile>(database->GetTypeConfig());
  osmscout::RouterParameter              routerParameter;

  osmscout::SimpleRoutingServiceRef router=std::make_shared<osmscout::SimpleRoutingService>(database,
                                                                                            routerParameter,
                                                                                            args.router);

  if (!router->Open()) {
    std::cerr << "Cannot open routing database" << std::endl;

    return 1;
  }

  osmscout::TypeConfigRef             typeConfig=database->GetTypeConfig();
  osmscout::RouteDescription          description;
  std::map<std::string,double>        carSpeedTable;
  osmscout::RoutingParameter          parameter;

  parameter.SetProgress(std::make_shared<ConsoleRoutingProgress>());

  switch (args.vehicle) {
  case osmscout::vehicleFoot:
    routingProfile->ParametrizeForFoot(*typeConfig,
                                       5.0);
    break;
  case osmscout::vehicleBicycle:
    routingProfile->ParametrizeForBicycle(*typeConfig,
                                          20.0);
    break;
  case osmscout::vehicleCar:
    GetCarSpeedTable(carSpeedTable);
    routingProfile->ParametrizeForCar(*typeConfig,
                                      carSpeedTable,
                                      160.0);
    break;
  }

  osmscout::RoutePosition start=router->GetClosestRoutableNode(args.start,
                                                               *routingProfile,
                                                               osmscout::Distance::Of<osmscout::Kilometer>(1));

  if (!start.IsValid()) {
    std::cerr << "Error while searching for routing node near start location!" << std::endl;
    return 1;
  }

  if (start.GetObjectFileRef().GetType()==osmscout::refNode) {
    std::cerr << "Cannot find start node for start location!" << std::endl;
  }

  osmscout::RoutePosition target=router->GetClosestRoutableNode(args.target,
                                                                *routingProfile,
                                                                osmscout::Distance::Of<osmscout::Kilometer>(1));

  if (!target.IsValid()) {
    std::cerr << "Error while searching for routing node near target location!" << std::endl;
    return 1;
  }

  if (target.GetObjectFileRef().GetType()==osmscout::refNode) {
    std::cerr << "Cannot find start node for target location!" << std::endl;
  }

  osmscout::RoutingResult result=router->CalculateRoute(*routingProfile,
                                                        start,
                                                        target,
                                                        parameter);

  if (!result.Success()) {
    std::cerr << "There was an error while calculating the route!" << std::endl;
    router->Close();
    return 1;
  }

  router->TransformRouteDataToRouteDescription(result.GetRoute(),
                                               description);

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors={
    std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>("Start"),
    std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>("Target"),
    std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::WayTypePostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::MotorwayJunctionPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::DestinationPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::MaxSpeedPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::POIsPostprocessor>()
  };

  osmscout::RoutePostprocessor             postprocessor;
  std::set<std::string>                    motorwayTypeNames{"highway_motorway",
                                                             "highway_motorway_trunk",
                                                             "highway_trunk",
                                                             "highway_motorway_primary"};
  std::set<std::string>                    motorwayLinkTypeNames{"highway_motorway_link",
                                                                 "highway_trunk_link"};
  std::set<std::string>                    junctionTypeNames{"highway_motorway_junction"};

  std::vector<osmscout::RoutingProfileRef> profiles{routingProfile};
  std::vector<osmscout::DatabaseRef>       databases{database};

  osmscout::StopClock postprocessTimer;

  if (!postprocessor.PostprocessRouteDescription(description,
                                                 profiles,
                                                 databases,
                                                 postprocessors,
                                                 motorwayTypeNames,
                                                 motorwayLinkTypeNames,
                                                 junctionTypeNames)) {
    std::cerr << "Error during route postprocessing" << std::endl;
  }

  postprocessTimer.Stop();

  std::cout << "Postprocessing time: " << postprocessTimer.ResultString() << std::endl;

  osmscout::StopClock                 generateTimer;
  osmscout::RouteDescriptionGenerator generator;
  RouteDescriptionGeneratorCallback   generatorCallback;

  generator.GenerateDescription(description,
                                generatorCallback);

  generateTimer.Stop();

  std::cout << "Description generation time: " << generateTimer.ResultString() << std::endl;

  PathGenerator pathGenerator(description,routingProfile->GetVehicleMaxSpeed());

  if (!args.gpxFile.empty()) {
    std::list<osmscout::Point> points;

    if (!router->TransformRouteDataToPoints(result.GetRoute(),
                                            points)) {
      std::cerr << "Error during route conversion" << std::endl;
    }

    DumpGpxFile(args.gpxFile,
                points,
                pathGenerator);
  }

  router->Close();

  return 0;
}
