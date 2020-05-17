/*
  Routing - a demo program for libosmscout
  Copyright (C) 2009  Tim Teulings

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

#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <list>
#include <sstream>

#include <osmscout/Database.h>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Time.h>

/*
  Examples for the nordrhein-westfalen.osm:

  Long: "In den Hüchten" Dortmund => Promenadenweg Bonn
    51.5717798 7.4587852  50.6890143 7.1360549

  Medium: "In den Hüchten" Dortmund => "Zur Taubeneiche" Arnsberg
     51.5717798 7.4587852  51.3846946 8.0771719

  Short: "In den Hüchten" Dortmund => "An der Dorndelle" Bergkamen
     51.5717798 7.4587852  51.6217831 7.6026704

  Roundabout: "Am Hohen Kamp" Bergkamen => Opferweg Bergkamen
     51.6163438 7.5952355  51.6237998 7.6419474

  Oneway Routing: Viktoriastraße Dortmund => Schwanenwall Dortmund
     51.5130296 7.4681888  51.5146904 7.4725241

  Very short: "In den Hüchten" Dortmund => "Kaiserstrasse" Dortmund
     51.5717798 7.4587852  51.5143553 7.4932118
*/

struct Arguments
{
  bool                   help=false;
  std::string            router=osmscout::RoutingService::DEFAULT_FILENAME_BASE;
  osmscout::Vehicle      vehicle=osmscout::Vehicle::vehicleCar;
  bool                   gpx=false;
  std::string            databaseDirectory;
  osmscout::GeoCoord     start;
  osmscout::GeoCoord     target;
  bool                   debug=false;
  bool                   dataDebug=false;
  bool                   routeDebug=false;
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

static std::string TimeToString(osmscout::Duration duration)
{
  double hours = osmscout::DurationAsHours(duration);
  std::ostringstream stream;

  stream << std::setfill(' ') << std::setw(2) << (int)std::floor(hours) << ":";

  hours-=std::floor(hours);

  stream << std::setfill('0') << std::setw(2) << (int)floor(60*hours+0.5);

  return stream.str();
}

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

static std::string MoveToTurnCommand(osmscout::RouteDescription::DirectionDescription::Move move)
{
  switch (move) {
  case osmscout::RouteDescription::DirectionDescription::sharpLeft:
    return "Turn sharp left";
  case osmscout::RouteDescription::DirectionDescription::left:
    return "Turn left";
  case osmscout::RouteDescription::DirectionDescription::slightlyLeft:
    return "Turn slightly left";
  case osmscout::RouteDescription::DirectionDescription::straightOn:
    return "Straight on";
  case osmscout::RouteDescription::DirectionDescription::slightlyRight:
    return "Turn slightly right";
  case osmscout::RouteDescription::DirectionDescription::right:
    return "Turn right";
  case osmscout::RouteDescription::DirectionDescription::sharpRight:
    return "Turn sharp right";
  }

  assert(false);

  return "???";
}

static std::string CrossingWaysDescriptionToString(const osmscout::RouteDescription::CrossingWaysDescription& crossingWaysDescription)
{
  std::set<std::string>                          names;
  osmscout::RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription.GetOriginDesccription();
  osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription.GetTargetDesccription();

  if (originDescription) {
    std::string nameString=originDescription->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  if (targetDescription) {
    std::string nameString=targetDescription->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  for (const auto& name : crossingWaysDescription.GetDescriptions()) {
    std::string nameString=name->GetDescription();

    if (!nameString.empty()) {
      names.insert(nameString);
    }
  }

  if (!names.empty()) {
    std::ostringstream stream;

    for (auto name=names.begin();
        name!=names.end();
        ++name) {
      if (name!=names.begin()) {
        stream << ", ";
      }
      stream << "'" << *name << "'";
    }

    return stream.str();
  }
  else {
    return "";
  }
}

struct RouteDescriptionGeneratorCallback : public osmscout::RouteDescriptionPostprocessor::Callback
{
  size_t lineCount;
  double prevDistance;
  osmscout::Duration prevTime;
  double distance;
  osmscout::Duration time;
  bool  lineDrawn;
  bool routeDebug;

  RouteDescriptionGeneratorCallback(bool routeDebug)
  : lineCount(0),
    prevDistance(0.0),
    prevTime(osmscout::Duration::zero()),
    distance(0.0),
    time(osmscout::Duration::zero()),
    lineDrawn(false),
    routeDebug(routeDebug)
  {
  }

  void NextLine(size_t& lineCount)
  {
    if (lineCount==0) {
      lineDrawn=true;

      std::cout << std::setfill(' ') << std::setw(5) << std::fixed << std::setprecision(1);
      std::cout << distance << "km ";

      if (distance-prevDistance!=0.0) {
        std::cout << std::setfill(' ') << std::setw(4) << std::fixed << std::setprecision(1);
        std::cout << distance-prevDistance << "km ";
      }
      else {
        std::cout << "       ";
      }

      std::cout << TimeToString(time) << "h ";

      if (time-prevTime!=osmscout::Duration::zero()) {
        std::cout << TimeToString(time-prevTime) << "h ";
      }
      else {
        std::cout << "       ";
      }
    }
    else {
      std::cout << "                             ";
    }

    lineCount++;
  }

  void BeforeRoute() override
  {
    std::cout << "----------------------------------------------------" << std::endl;
    std::cout << "     At| After|  Time| After|" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;
  }

  void OnStart(const osmscout::RouteDescription::StartDescriptionRef& startDescription,
               const osmscout::RouteDescription::TypeNameDescriptionRef& typeNameDescription,
               const osmscout::RouteDescription::NameDescriptionRef& nameDescription) override
  {
    NextLine(lineCount);
    std::cout << "Start at '" << startDescription->GetDescription() << "'" << std::endl;

    NextLine(lineCount);
    std::cout << "Drive along";

    if (typeNameDescription) {
      std::cout << " " << typeNameDescription->GetDescription();
    }

    if (nameDescription &&
        nameDescription->HasName()) {
      std::cout << " '" << nameDescription->GetDescription() << "'";
    }
    else {
      std::cout << " <unknown name>";
    }

    std::cout << std::endl;
  }

  void OnTargetReached(const osmscout::RouteDescription::TargetDescriptionRef& targetDescription) override
  {
    NextLine(lineCount);

    std::cout << "Target reached '" << targetDescription->GetDescription() << "'" << std::endl;
  }

  void OnTurn(const osmscout::RouteDescription::TurnDescriptionRef& /*turnDescription*/,
              const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
              const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
              const osmscout::RouteDescription::TypeNameDescriptionRef& typeNameDescription,
              const osmscout::RouteDescription::NameDescriptionRef& nameDescription) override
  {
    std::string crossingWaysString;

    if (crossingWaysDescription) {
      crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
    }

    if (!crossingWaysString.empty()) {
      NextLine(lineCount);
      std::cout << "At crossing " << crossingWaysString << std::endl;
    }

    NextLine(lineCount);
    if (directionDescription) {
      std::cout << MoveToTurnCommand(directionDescription->GetCurve());
    }
    else {
      std::cout << "Turn";
    }

    std::cout << " into";

    if (typeNameDescription) {
      std::cout << " " << typeNameDescription->GetDescription();
    }

    if (nameDescription &&
        nameDescription->HasName()) {

      std::cout << " '" << nameDescription->GetDescription() << "'";
    }
    else {
      std::cout << " <unknown name>";
    }

    std::cout << std::endl;
  }

  void OnRoundaboutEnter(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
                         const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription) override
  {
    std::string crossingWaysString;

    if (crossingWaysDescription) {
      crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
    }

    if (!crossingWaysString.empty()) {
      NextLine(lineCount);
      std::cout << "At crossing " << crossingWaysString << std::endl;
    }

    NextLine(lineCount);
    std::cout << "Enter roundabout" << std::endl;
  }

  void OnRoundaboutLeave(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                         const osmscout::RouteDescription::NameDescriptionRef& nameDescription) override
  {
    NextLine(lineCount);
    std::cout << "Leave roundabout (" << roundaboutLeaveDescription->GetExitCount() << ". exit)";

    if (nameDescription &&
        nameDescription->HasName()) {
      std::cout << " into street '" << nameDescription->GetDescription() << "'";
    }

    std::cout << std::endl;
  }

  void OnMotorwayEnter(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                       const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription) override
  {
    std::string crossingWaysString;

    if (crossingWaysDescription) {
      crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
    }

    if (!crossingWaysString.empty()) {
      NextLine(lineCount);
      std::cout << "At crossing " << crossingWaysString << std::endl;
    }

    NextLine(lineCount);
    std::cout << "Enter motorway";

    if (motorwayEnterDescription->GetToDescription() &&
        motorwayEnterDescription->GetToDescription()->HasName()) {
      std::cout << " '" << motorwayEnterDescription->GetToDescription()->GetDescription() << "'";
    }

    std::cout << std::endl;
  }

  void OnMotorwayChange(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                        const osmscout::RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                        const osmscout::RouteDescription::DestinationDescriptionRef& crossingDestinationDescription) override
  {
    NextLine(lineCount);

    if (motorwayJunctionDescription &&
        motorwayJunctionDescription->GetJunctionDescription()) {
      std::cout << "At";

      if (!motorwayJunctionDescription->GetJunctionDescription()->GetName().empty()) {
        std::cout << " '" << motorwayJunctionDescription->GetJunctionDescription()->GetName() << "'";

        if (!motorwayJunctionDescription->GetJunctionDescription()->GetRef().empty()) {
          std::cout << " (exit " << motorwayJunctionDescription->GetJunctionDescription()->GetRef() << ")";
        }
      }

      std::cout << std::endl;
      NextLine(lineCount);
    }

    std::cout << "Change motorway";

    if (motorwayChangeDescription->GetFromDescription() &&
        motorwayChangeDescription->GetFromDescription()->HasName()) {
      std::cout << " from '" << motorwayChangeDescription->GetFromDescription()->GetDescription() << "'";
    }

    if (motorwayChangeDescription->GetToDescription() &&
        motorwayChangeDescription->GetToDescription()->HasName()) {
      std::cout << " to '" << motorwayChangeDescription->GetToDescription()->GetDescription() << "'";
    }

    if (crossingDestinationDescription) {
      std::cout << " destination '" << crossingDestinationDescription->GetDescription() << "'";
    }

    std::cout << std::endl;
  }

  void OnMotorwayLeave(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                       const osmscout::RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                       const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                       const osmscout::RouteDescription::NameDescriptionRef& nameDescription) override
  {
    NextLine(lineCount);

    if (motorwayJunctionDescription &&
        motorwayJunctionDescription->GetJunctionDescription()) {
      std::cout << "At";

      if (!motorwayJunctionDescription->GetJunctionDescription()->GetName().empty()) {
        std::cout << " '" << motorwayJunctionDescription->GetJunctionDescription()->GetName() << "'";

        if (!motorwayJunctionDescription->GetJunctionDescription()->GetRef().empty()) {
          std::cout << " (exit " << motorwayJunctionDescription->GetJunctionDescription()->GetRef() << ")";
        }
      }

      std::cout << std::endl;
      NextLine(lineCount);
    }

    std::cout << "Leave motorway";

    if (motorwayLeaveDescription->GetFromDescription() &&
        motorwayLeaveDescription->GetFromDescription()->HasName()) {
      std::cout << " '" << motorwayLeaveDescription->GetFromDescription()->GetDescription() << "'";
    }

    if (directionDescription &&
        directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyLeft &&
        directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::straightOn &&
        directionDescription->GetCurve()!=osmscout::RouteDescription::DirectionDescription::slightlyRight) {
      std::cout << " " << MoveToTurnCommand(directionDescription->GetCurve());
    }

    if (nameDescription &&
        nameDescription->HasName()) {
      std::cout << " into '" << nameDescription->GetDescription() << "'";
    }

    std::cout << std::endl;
  }

  void OnPathNameChange(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription) override
  {
    NextLine(lineCount);

    std::cout << "Way changes name";
    if (nameChangedDescription->GetOriginDescription()) {
      std::cout << " from ";
      std::cout << "'" << nameChangedDescription->GetOriginDescription()->GetDescription() << "'";
    }

    std::cout << " to '";
    std::cout << nameChangedDescription->GetTargetDescription()->GetDescription();
    std::cout << "'" << std::endl;
  }

  void OnMaxSpeed(const osmscout::RouteDescription::MaxSpeedDescriptionRef& /*maxSpeedDescription*/) override
  {
    //NextLine(lineCount);
    //std::cout << "MaxSpeed: " << (unsigned int)maxSpeedDescription->GetMaxSpeed() << std::endl;
  }

  void OnPOIAtRoute(const osmscout::RouteDescription::POIAtRouteDescriptionRef& poiAtRouteDescription) override
  {
    NextLine(lineCount);
    std::cout << "Pass: " << poiAtRouteDescription->GetName()->GetDescription() << std::endl;
  }

  void BeforeNode(const osmscout::RouteDescription::Node& node) override
  {
    lineCount=0;
    lineDrawn=false;
    distance=node.GetDistance().As<osmscout::Kilometer>();
    time=node.GetTime();

    if(routeDebug) {
      NextLine(lineCount);

      using namespace std::chrono;
      using hoursDouble = duration<double, std::ratio<3600>>;

      std::cout << "// at " << node.GetLocation().GetDisplayText() << std::endl;

      NextLine(lineCount);
      std::cout << "// " << duration_cast<hoursDouble>(node.GetTime()).count() << "h " << std::setw(0)
                << std::setprecision(3) << node.GetDistance() << " ";

      if (node.GetPathObject().Valid()) {
        std::cout << node.GetPathObject().GetTypeName() << " " << node.GetPathObject().GetFileOffset() << "["
                  << node.GetCurrentNodeIndex() << "] => " << node.GetPathObject().GetTypeName() << " "
                  << node.GetPathObject().GetFileOffset() << "[" << node.GetTargetNodeIndex() << "]";
      }

      std::cout << std::endl;

      for (std::list<osmscout::RouteDescription::DescriptionRef>::const_iterator d = node.GetDescriptions().begin();
           d != node.GetDescriptions().end();
           ++d) {
        osmscout::RouteDescription::DescriptionRef desc = *d;

        NextLine(lineCount);
        std::cout << "// " << desc->GetDebugString() << std::endl;
      }
    }
  }

  void AfterNode(const osmscout::RouteDescription::Node& node) override
  {
    if (lineCount==0 && lineDrawn) {
      std::cout << std::endl;
    }

    prevDistance=node.GetDistance().As<osmscout::Kilometer>();
    prevTime=node.GetTime();
  }
};

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("Routing",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.gpx=value;
                      }),
                      "gpx",
                      "Dump resulting route as GPX to std::cout",
                      false);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                      args.debug=value;
                      }),
                      "debug",
                      "Enable debug output",
                      false);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.dataDebug=value;
                      }),
                      "dataDebug",
                      "Dump data nodes to sdt::cout",
                      false);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.routeDebug=value;
                      }),
                      "routeDebug",
                      "Dump route description data to std::cout",
                      false);

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

  osmscout::log.Debug(args.debug);
  osmscout::log.Info(true);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::FastestPathRoutingProfileRef routingProfile=std::make_shared<osmscout::FastestPathRoutingProfile>(database->GetTypeConfig());
  osmscout::RouterParameter              routerParameter;

  if (!args.gpx) {
    routerParameter.SetDebugPerformance(true);
  }

  osmscout::SimpleRoutingServiceRef router=std::make_shared<osmscout::SimpleRoutingService>(database,
                                                                                            routerParameter,
                                                                                            args.router);

  if (!router->Open()) {
    std::cerr << "Cannot open routing database" << std::endl;

    return 1;
  }

  osmscout::TypeConfigRef             typeConfig=database->GetTypeConfig();
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

  auto startResult=router->GetClosestRoutableNode(args.start,
                                                  *routingProfile,
                                                  osmscout::Kilometers(1));

  if (!startResult.IsValid()) {
    std::cerr << "Error while searching for routing node near start location!" << std::endl;
    return 1;
  }

  osmscout::RoutePosition start=startResult.GetRoutePosition();
  if (start.GetObjectFileRef().GetType()==osmscout::refNode) {
    std::cerr << "Cannot find start node for start location!" << std::endl;
  }

  auto targetResult=router->GetClosestRoutableNode(args.target,
                                                   *routingProfile,
                                                   osmscout::Kilometers(1));

  if (!targetResult.IsValid()) {
    std::cerr << "Error while searching for routing node near target location!" << std::endl;
    return 1;
  }

  osmscout::RoutePosition target=targetResult.GetRoutePosition();
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

  if (args.dataDebug) {
    std::cout << "Route raw data:" << std::endl;
    for (const auto &entry : result.GetRoute().Entries()) {
      std::cout << entry.GetPathObject().GetName() << "[" << entry.GetCurrentNodeIndex() << "]" << " = "
                << entry.GetCurrentNodeId() << " => " << entry.GetTargetNodeIndex() << std::endl;
    }
  }

  auto routeDescriptionResult=router->TransformRouteDataToRouteDescription(result.GetRoute());

  if (!routeDescriptionResult.Success()) {
    std::cerr << "Error during generation of route description" << std::endl;
    return 1;
  }

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors{
    std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>("Start"),
    std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>("Target"),
    std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::WayTypePostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::LanesPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::SuggestedLanesPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::MotorwayJunctionPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::DestinationPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::MaxSpeedPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>(),
    std::make_shared<osmscout::RoutePostprocessor::POIsPostprocessor>()
  };

  osmscout::RoutePostprocessor postprocessor;

  if(args.gpx) {
    osmscout::RoutePointsResult routePointsResult=router->TransformRouteDataToPoints(result.GetRoute());

    if (routePointsResult.Success()) {
      std::cout.precision(8);
      std::cout << R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)" << std::endl;
      std::cout
        << R"(<gpx xmlns="http://www.topografix.com/GPX/1/1" creator="bin2gpx" version="1.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">)"
        << std::endl;

      std::cout << "\t<wpt lat=\"" << args.start.GetLat() << "\" lon=\"" << args.start.GetLon() << "\">" << std::endl;
      std::cout << "\t\t<name>Start</name>" << std::endl;
      std::cout << "\t\t<fix>2d</fix>" << std::endl;
      std::cout << "\t</wpt>" << std::endl;

      std::cout << "\t<wpt lat=\"" << args.target.GetLat() << "\" lon=\"" << args.target.GetLon() << "\">" << std::endl;
      std::cout << "\t\t<name>Target</name>" << std::endl;
      std::cout << "\t\t<fix>2d</fix>" << std::endl;
      std::cout << "\t</wpt>" << std::endl;

      std::cout << "\t<trk>" << std::endl;
      std::cout << "\t\t<name>Route</name>" << std::endl;
      std::cout << "\t\t<trkseg>" << std::endl;
      for (const auto& point : routePointsResult.GetPoints()->points) {
        std::cout << "\t\t\t<trkpt lat=\"" << point.GetLat() << "\" lon=\"" << point.GetLon() << "\">" << std::endl;
        std::cout << "\t\t\t\t<fix>2d</fix>" << std::endl;
        std::cout << "\t\t\t</trkpt>" << std::endl;
      }
      std::cout << "\t\t</trkseg>" << std::endl;
      std::cout << "\t</trk>" << std::endl;
      std::cout << "</gpx>" << std::endl;
    }
    else {
      std::cerr << "Error during route conversion" << std::endl;
    }
  }

  osmscout::StopClock postprocessTimer;

  std::set<std::string>                    motorwayTypeNames{"highway_motorway",
                                                             "highway_motorway_trunk",
                                                             "highway_trunk",
                                                             "highway_motorway_primary"};
  std::set<std::string>                    motorwayLinkTypeNames{"highway_motorway_link",
                                                                 "highway_trunk_link"};
  std::set<std::string>                    junctionTypeNames{"highway_motorway_junction"};

  std::vector<osmscout::RoutingProfileRef> profiles{routingProfile};
  std::vector<osmscout::DatabaseRef>       databases{database};

  if (!postprocessor.PostprocessRouteDescription(*routeDescriptionResult.GetDescription(),
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

  osmscout::StopClock                     generateTimer;
  osmscout::RouteDescriptionPostprocessor generator;
  RouteDescriptionGeneratorCallback       generatorCallback(args.routeDebug);

  generator.GenerateDescription(*routeDescriptionResult.GetDescription(),
                                generatorCallback);

  generateTimer.Stop();

  std::cout << "Description generation time: " << generateTimer.ResultString() << std::endl;

  router->Close();

  return 0;
}
