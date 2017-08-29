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

#include <osmscout/util/Geometry.h>

//#define ROUTE_DEBUG
//#define NODE_DEBUG
//#define DATA_DEBUG

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

  void Reset()
  {
    lastDump=std::chrono::system_clock::now();
    maxPercent=0.0;
  }

  void Progress(double currentMaxDistance,
                double overallDistance)
  {
    double currentPercent=(currentMaxDistance*100.0)/overallDistance;

    std::chrono::system_clock::time_point now=std::chrono::system_clock::now();

    maxPercent=std::max(maxPercent,currentPercent);

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now-lastDump).count()>500) {
      std::cout << (size_t)maxPercent << "%" << std::endl;

      lastDump=now;
    }
  }
};

static std::string TimeToString(double time)
{
  std::ostringstream stream;

  stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";

  time-=std::floor(time);

  stream << std::setfill('0') << std::setw(2) << (int)floor(60*time+0.5);

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

  if (names.size()>0) {
    std::ostringstream stream;

    for (std::set<std::string>::const_iterator name=names.begin();
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

static bool HasRelevantDescriptions(const osmscout::RouteDescription::Node& node)
{
#if defined(ROUTE_DEBUG)
  unused(node);

  return true;
#else
  if (node.HasDescription(osmscout::RouteDescription::NODE_START_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::ROUNDABOUT_ENTER_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::ROUNDABOUT_LEAVE_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::TURN_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::MOTORWAY_ENTER_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::MOTORWAY_CHANGE_DESC)) {
    return true;
  }

  if (node.HasDescription(osmscout::RouteDescription::MOTORWAY_LEAVE_DESC)) {
    return true;
  }

  /*
  if (node.HasDescription(osmscout::RouteDescription::WAY_MAXSPEED_DESC)) {
    return true;
  }*/

  return false;
#endif
}

static void NextLine(size_t& lineCount)
{
  if (lineCount>0) {
    std::cout << "                             ";
  }

  lineCount++;
}

static void DumpStartDescription(size_t& lineCount,
                                 const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                                 const osmscout::RouteDescription::TypeNameDescriptionRef& typeNameDescription,
                                 const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
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

static void DumpTargetDescription(size_t& lineCount,
                                  const osmscout::RouteDescription::TargetDescriptionRef& targetDescription)
{
  NextLine(lineCount);

  std::cout << "Target reached '" << targetDescription->GetDescription() << "'" << std::endl;
}

static void DumpTurnDescription(size_t& lineCount,
                                const osmscout::RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                const osmscout::RouteDescription::TypeNameDescriptionRef& typeNameDescription,
                                const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
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

static void DumpRoundaboutEnterDescription(size_t& lineCount,
                                           const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
                                           const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
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

static void DumpRoundaboutLeaveDescription(size_t& lineCount,
                                           const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                           const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
{
  NextLine(lineCount);
  std::cout << "Leave roundabout (" << roundaboutLeaveDescription->GetExitCount() << ". exit)";

  if (nameDescription &&
      nameDescription->HasName()) {
    std::cout << " into street '" << nameDescription->GetDescription() << "'";
  }

  std::cout << std::endl;
}

static void DumpMotorwayEnterDescription(size_t& lineCount,
                                         const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                         const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
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

static void DumpMotorwayChangeDescription(size_t& lineCount,
                                          const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                                          const osmscout::RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                                          const osmscout::RouteDescription::DestinationDescriptionRef& crossingDestinationDescription)
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

static void DumpMotorwayLeaveDescription(size_t& lineCount,
                                         const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                         const osmscout::RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                                         const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                         const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
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

static void DumpNameChangedDescription(size_t& lineCount,
                                       const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
{
  NextLine(lineCount);

  std::cout << "Way changes name";
  if (nameChangedDescription->GetOriginDesccription()) {
    std::cout << " from ";
    std::cout << "'" << nameChangedDescription->GetOriginDesccription()->GetDescription() << "'";
  }

  std::cout << " to '";
  std::cout << nameChangedDescription->GetTargetDesccription()->GetDescription();
  std::cout << "'" << std::endl;
}

int main(int argc, char* argv[])
{
  std::string                               routerFilenamebase=osmscout::RoutingService::DEFAULT_FILENAME_BASE;
  osmscout::Vehicle                         vehicle=osmscout::vehicleCar;
  std::string                               mapDirectory;
  bool                                      outputGPX=false;
  bool                                      argumentError=false;

  double                                    startLat;
  double                                    startLon;

  double                                    targetLat;
  double                                    targetLon;

  int currentArg=1;
  while (currentArg<argc) {
    if (strcmp(argv[currentArg],"--router")==0) {
      currentArg++;

      if (currentArg>=argc) {
        argumentError=true;
      }
      else {
        routerFilenamebase=argv[currentArg];
        currentArg++;
      }
    }
    else if (strcmp(argv[currentArg],"--foot")==0) {
      vehicle=osmscout::vehicleFoot;
      currentArg++;
    }
    else if (strcmp(argv[currentArg],"--bicycle")==0) {
      vehicle=osmscout::vehicleBicycle;
      currentArg++;
    }
    else if (strcmp(argv[currentArg],"--car")==0) {
      vehicle=osmscout::vehicleCar;
      currentArg++;
    }
    else if (strcmp(argv[currentArg],"--gpx")==0) {
      outputGPX=true;
      currentArg++;
    }
    else {
      // No more "special" arguments
      break;
    }
  }

  if (argumentError ||
      argc-currentArg!=5) {
    std::cout << "Routing" << std::endl;
    std::cout << "  [--router <router filename base>]" << std::endl;
    std::cout << "  [--foot | --bicycle | --car]" << std::endl;
    std::cout << "  [--gpx]" << std::endl;
    std::cout << "  <map directory>" << std::endl;
    std::cout << "  <start lat> <start lon>" << std::endl;
    std::cout << "  <target lat> <target lon>" << std::endl;
    return 1;
  }

  mapDirectory=argv[currentArg];
  currentArg++;

  if (sscanf(argv[currentArg],"%lf",&startLat)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }
  currentArg++;

  if (sscanf(argv[currentArg],"%lf",&startLon)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }
  currentArg++;

  if (sscanf(argv[currentArg],"%lf",&targetLat)!=1) {
    std::cerr << "lat is not numeric!" << std::endl;
    return 1;
  }
  currentArg++;

  if (sscanf(argv[currentArg],"%lf",&targetLon)!=1) {
    std::cerr << "lon is not numeric!" << std::endl;
    return 1;
  }
  currentArg++;

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(mapDirectory.c_str())) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  osmscout::FastestPathRoutingProfileRef routingProfile=std::make_shared<osmscout::FastestPathRoutingProfile>(database->GetTypeConfig());
  osmscout::RouterParameter              routerParameter;

  if (!outputGPX) {
    routerParameter.SetDebugPerformance(true);
  }

  osmscout::SimpleRoutingServiceRef router=std::make_shared<osmscout::SimpleRoutingService>(
                                                    database,
                                                    routerParameter,
                                                    routerFilenamebase);

  if (!router->Open()) {
    std::cerr << "Cannot open routing database" << std::endl;

    return 1;
  }

  osmscout::TypeConfigRef             typeConfig=database->GetTypeConfig();
  osmscout::RouteDescription          description;
  std::map<std::string,double>        carSpeedTable;
  osmscout::RoutingParameter          parameter;

  parameter.SetProgress(std::make_shared<ConsoleRoutingProgress>());

  switch (vehicle) {
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

  double radius = 1000.0;
  osmscout::RoutePosition start=router->GetClosestRoutableNode(osmscout::GeoCoord(startLat,startLon),
                                                               *routingProfile,
                                                               radius);

  if (!start.IsValid()) {
    std::cerr << "Error while searching for routing node near start location!" << std::endl;
    return 1;
  }

  if (start.GetObjectFileRef().GetType()==osmscout::refNode) {
    std::cerr << "Cannot find start node for start location!" << std::endl;
  }

  radius = 1000.0;
  osmscout::RoutePosition target=router->GetClosestRoutableNode(osmscout::GeoCoord(targetLat,targetLon),
                                                               *routingProfile,
                                                               radius);

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

#ifdef DATA_DEBUG
  std::cout << "Route raw data:" << std::endl;
  for (const auto &entry : result.GetRoute().Entries()) {
    std::cout << entry.GetPathObject().GetName() << "[" << entry.GetCurrentNodeIndex() << "]" << " = " << entry.GetCurrentNodeId() << " => " << entry.GetTargetNodeIndex() << std::endl;
  }
#endif

  router->TransformRouteDataToRouteDescription(result.GetRoute(),
                                               description);

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>("Start"));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>("Target"));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayTypePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::MotorwayJunctionPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DestinationPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::MaxSpeedPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>());

  osmscout::RoutePostprocessor postprocessor;
  size_t                       roundaboutCrossingCounter=0;

  std::list<osmscout::Point> points;

  if(outputGPX) {
    if (!router->TransformRouteDataToPoints(result.GetRoute(),
                                            points)) {
      std::cerr << "Error during route conversion" << std::endl;
    }
    std::cout.precision(8);
    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>" << std::endl;
    std::cout << "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" creator=\"bin2gpx\" version=\"1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">" << std::endl;

    std::cout << "\t<wpt lat=\""<< startLat << "\" lon=\""<< startLon << "\">" << std::endl;
    std::cout << "\t\t<name>Start</name>" << std::endl;
    std::cout << "\t\t<fix>2d</fix>" << std::endl;
    std::cout << "\t</wpt>" << std::endl;

    std::cout << "\t<wpt lat=\""<< targetLat << "\" lon=\""<< targetLon << "\">" << std::endl;
    std::cout << "\t\t<name>Target</name>" << std::endl;
    std::cout << "\t\t<fix>2d</fix>" << std::endl;
    std::cout << "\t</wpt>" << std::endl;

    std::cout << "\t<trk>" << std::endl;
    std::cout << "\t\t<name>Route</name>" << std::endl;
    std::cout << "\t\t<trkseg>" << std::endl;
    for (const auto &point : points) {
      std::cout << "\t\t\t<trkpt lat=\""<< point.GetLat() << "\" lon=\""<< point.GetLon() <<"\">" << std::endl;
      std::cout << "\t\t\t\t<fix>2d</fix>" << std::endl;
      std::cout << "\t\t\t</trkpt>" << std::endl;
    }
    std::cout << "\t\t</trkseg>" << std::endl;
    std::cout << "\t</trk>" << std::endl;
    std::cout << "</gpx>" << std::endl;

    return 0;
  }

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

  std::map<osmscout::DatabaseId,osmscout::RoutingProfileRef> profiles;
  std::map<osmscout::DatabaseId,osmscout::DatabaseRef> databases;

  profiles[0]=routingProfile;
  databases[0]=database;

  if (!postprocessor.PostprocessRouteDescription(description,
                                                 profiles,
                                                 databases,
                                                 postprocessors,
                                                 motorwayTypeNames,
                                                 motorwayLinkTypeNames,
                                                 junctionTypeNames)) {
    std::cerr << "Error during route postprocessing" << std::endl;
  }

  std::cout << "----------------------------------------------------" << std::endl;
  std::cout << "     At| After|  Time| After|" << std::endl;
  std::cout << "----------------------------------------------------" << std::endl;
  std::list<osmscout::RouteDescription::Node>::const_iterator prevNode=description.Nodes().end();

  for (std::list<osmscout::RouteDescription::Node>::const_iterator node=description.Nodes().begin();
       node!=description.Nodes().end();
       ++node) {
    osmscout::RouteDescription::DescriptionRef                 desc;
    osmscout::RouteDescription::NameDescriptionRef             nameDescription;
    osmscout::RouteDescription::DirectionDescriptionRef        directionDescription;
    osmscout::RouteDescription::NameChangedDescriptionRef      nameChangedDescription;
    osmscout::RouteDescription::CrossingWaysDescriptionRef     crossingWaysDescription;

    osmscout::RouteDescription::StartDescriptionRef            startDescription;
    osmscout::RouteDescription::TargetDescriptionRef           targetDescription;
    osmscout::RouteDescription::TurnDescriptionRef             turnDescription;
    osmscout::RouteDescription::RoundaboutEnterDescriptionRef  roundaboutEnterDescription;
    osmscout::RouteDescription::RoundaboutLeaveDescriptionRef  roundaboutLeaveDescription;
    osmscout::RouteDescription::MotorwayEnterDescriptionRef    motorwayEnterDescription;
    osmscout::RouteDescription::MotorwayChangeDescriptionRef   motorwayChangeDescription;
    osmscout::RouteDescription::MotorwayLeaveDescriptionRef    motorwayLeaveDescription;
    osmscout::RouteDescription::MotorwayJunctionDescriptionRef motorwayJunctionDescription;
    osmscout::RouteDescription::DestinationDescriptionRef      crossingDestinationDescription;
    osmscout::RouteDescription::MaxSpeedDescriptionRef         maxSpeedDescription;
    osmscout::RouteDescription::TypeNameDescriptionRef         typeNameDescription;

    desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_DESC);
    if (desc) {
      nameDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::NameDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::DIRECTION_DESC);
    if (desc) {
      directionDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::DirectionDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::WAY_NAME_CHANGED_DESC);
    if (desc) {
      nameChangedDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::NameChangedDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::CROSSING_WAYS_DESC);
    if (desc) {
      crossingWaysDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::CrossingWaysDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::NODE_START_DESC);
    if (desc) {
      startDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::StartDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::NODE_TARGET_DESC);
    if (desc) {
      targetDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::TargetDescription>(desc);
    }


    desc=node->GetDescription(osmscout::RouteDescription::TURN_DESC);
    if (desc) {
      turnDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::TurnDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::ROUNDABOUT_ENTER_DESC);
    if (desc) {
      roundaboutEnterDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::RoundaboutEnterDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::ROUNDABOUT_LEAVE_DESC);
    if (desc) {
      roundaboutLeaveDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::RoundaboutLeaveDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_ENTER_DESC);
    if (desc) {
      motorwayEnterDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayEnterDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_CHANGE_DESC);
    if (desc) {
      motorwayChangeDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayChangeDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_LEAVE_DESC);
    if (desc) {
      motorwayLeaveDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayLeaveDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::MOTORWAY_JUNCTION_DESC);
    if (desc) {
      motorwayJunctionDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MotorwayJunctionDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::CROSSING_DESTINATION_DESC);
    if (desc) {
      crossingDestinationDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::DestinationDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::WAY_MAXSPEED_DESC);
    if (desc) {
      maxSpeedDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::MaxSpeedDescription>(desc);
    }

    desc=node->GetDescription(osmscout::RouteDescription::WAY_TYPE_NAME_DESC);
    if (desc) {
      typeNameDescription=std::dynamic_pointer_cast<osmscout::RouteDescription::TypeNameDescription>(desc);
    }

    if (crossingWaysDescription &&
        roundaboutCrossingCounter>0 &&
        crossingWaysDescription->GetExitCount()>1) {
      roundaboutCrossingCounter+=crossingWaysDescription->GetExitCount()-1;
    }

    if (!HasRelevantDescriptions(*node)) {
      continue;
    }

    std::cout << std::setfill(' ') << std::setw(5) << std::fixed << std::setprecision(1);
    std::cout << node->GetDistance() << "km ";

    if (prevNode!=description.Nodes().end() && node->GetDistance()-prevNode->GetDistance()!=0.0) {
      std::cout << std::setfill(' ') << std::setw(4) << std::fixed << std::setprecision(1);
      std::cout << node->GetDistance()-prevNode->GetDistance() << "km ";
    }
    else {
      std::cout << "       ";
    }

    std::cout << TimeToString(node->GetTime()) << "h ";

    if (prevNode!=description.Nodes().end() && node->GetTime()-prevNode->GetTime()!=0.0) {
      std::cout << TimeToString(node->GetTime()-prevNode->GetTime()) << "h ";
    }
    else {
      std::cout << "       ";
    }

    /*
    if (maxSpeedDescription) {
      std::cout << std::setfill(' ') << std::setw(5) << std::fixed << std::setprecision(1);
      std::cout << (size_t)maxSpeedDescription->GetMaxSpeed() << " km/h ";
    }
    else {
      std::cout << "           ";
    }*/

    size_t lineCount=0;

#if defined(ROUTE_DEBUG) || defined(NODE_DEBUG)
    NextLine(lineCount);

    std::cout << "// " << node->GetTime() << "h " << std::setw(0) << std::setprecision(3) << node->GetDistance() << "km ";

    if (node->GetPathObject().Valid()) {
      std::cout << node->GetPathObject().GetTypeName() << " " << node->GetPathObject().GetFileOffset() << "[" << node->GetCurrentNodeIndex() << "] => " << node->GetPathObject().GetTypeName() << " " << node->GetPathObject().GetFileOffset() << "[" << node->GetTargetNodeIndex() << "]";
    }

    std::cout << std::endl;

    for (std::list<osmscout::RouteDescription::DescriptionRef>::const_iterator d=node->GetDescriptions().begin();
        d!=node->GetDescriptions().end();
        ++d) {
      osmscout::RouteDescription::DescriptionRef desc=*d;

      NextLine(lineCount);
      std::cout << "// " << desc->GetDebugString() << std::endl;
    }
#endif

    if (startDescription) {
      DumpStartDescription(lineCount,
                           startDescription,
                           typeNameDescription,
                           nameDescription);
    }
    else if (targetDescription) {
      DumpTargetDescription(lineCount,targetDescription);
    }
    else if (turnDescription) {
      DumpTurnDescription(lineCount,
                          turnDescription,
                          crossingWaysDescription,
                          directionDescription,
                          typeNameDescription,
                          nameDescription);
    }
    else if (roundaboutEnterDescription) {
      DumpRoundaboutEnterDescription(lineCount,
                                     roundaboutEnterDescription,
                                     crossingWaysDescription);

      roundaboutCrossingCounter=1;
    }
    else if (roundaboutLeaveDescription) {
      DumpRoundaboutLeaveDescription(lineCount,
                                     roundaboutLeaveDescription,
                                     nameDescription);

      roundaboutCrossingCounter=0;
    }
    else if (motorwayEnterDescription) {
      DumpMotorwayEnterDescription(lineCount,
                                   motorwayEnterDescription,
                                   crossingWaysDescription);
    }
    else if (motorwayChangeDescription) {
      DumpMotorwayChangeDescription(lineCount,
                                    motorwayChangeDescription,
                                    motorwayJunctionDescription,
                                    crossingDestinationDescription);
    }
    else if (motorwayLeaveDescription) {
      DumpMotorwayLeaveDescription(lineCount,
                                   motorwayLeaveDescription,
                                   motorwayJunctionDescription,
                                   directionDescription,
                                   nameDescription);
    }
    else if (nameChangedDescription) {
      DumpNameChangedDescription(lineCount,
                                 nameChangedDescription);
    }

    if (lineCount==0) {
      std::cout << std::endl;
    }

    prevNode=node;
  }

  router->Close();

  return 0;
}
