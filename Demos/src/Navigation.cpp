/*
 Navigation - a demo program for libosmscout
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

/*

 Sample usage :

 > ./Navigation --router routecar  ~/Documents/OSM/Czech\ Republic.osmscout 50.06645 14.39931 50.088 14.40415 50.06675 14.39934
 No speed for type 'highway_residential_area' defined!
 No speed for type 'highway_service_area' defined!
 Distance to route: 8.13049e-05
 Distance from start: 0.0512167
 Time from start:  0:00
 Distance to destination: 3.57549
 Time to destination:  0:04
 Next routing instructions: At crossing 'Na Zatlance', 'Ostrovského', 'U Santošky'
 Turn right to 'Ostrovského'
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <cstring>

#include <osmscout/Database.h>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/RoutePostprocessor.h>

#include "Navigation.h"

static std::string TimeToString(double time)
{
    std::ostringstream stream;
    stream << std::setfill(' ') << std::setw(2) << (int)std::floor(time) << ":";
    time-=std::floor(time);
    stream << std::setfill('0') << std::setw(2) << (int)floor(60*time+0.5);
    return stream.str();
}

namespace osmscout {

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
        std::set<std::string> names;
        osmscout::RouteDescription::NameDescriptionRef originDescription=crossingWaysDescription.GetOriginDesccription();
        osmscout::RouteDescription::NameDescriptionRef targetDescription=crossingWaysDescription.GetTargetDesccription();

        if (originDescription) {
            std::string nameString=originDescription->GetDescription();

            if (!nameString.empty() && nameString.compare("unnamed road")) {
                names.insert(nameString);
            }
        }

        if (targetDescription) {
            std::string nameString=targetDescription->GetDescription();

            if (!nameString.empty() && nameString.compare("unnamed road")) {
                names.insert(nameString);
            }
        }

        for (const auto& name : crossingWaysDescription.GetDescriptions()) {
            std::string nameString=name->GetDescription();

            if (!nameString.empty() && nameString.compare("unnamed road")) {
                names.insert(nameString);
            }
        }

        if (names.size()>1) {
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

    bool HasRelevantDescriptions(const osmscout::RouteDescription::Node& node)
    {
        if (node.HasDescription(osmscout::RouteDescription::NODE_START_DESC)) {
            return true;
        }

        if (node.HasDescription(osmscout::RouteDescription::NODE_TARGET_DESC)) {
            return true;
        }

        if (node.HasDescription(RouteDescription::WAY_NAME_CHANGED_DESC)) {
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

        return false;
    }

    std::string DumpStartDescription(const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                                     const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
    {
        std::ostringstream stream;
        stream << startDescription->GetDescription();

        if (nameDescription &&
            nameDescription->HasName()) {
            stream << ", drive along '" << nameDescription->GetDescription() << "'";
        }
        return stream.str();
    }

    std::string DumpTargetDescription(const osmscout::RouteDescription::TargetDescriptionRef& /*targetDescription*/)
    {
        std::ostringstream stream;
        stream << "Target reached";
        return stream.str();
    }

    NodeDescription DumpTurnDescription(const osmscout::RouteDescription::TurnDescriptionRef& /*turnDescription*/,
                                        const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                                        const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                        const osmscout::RouteDescription::NameDescriptionRef& nameDescription)
    {
        NodeDescription description;
        description.roundaboutExitNumber = -1;
        std::ostringstream stream;
        std::string crossingWaysString;

        if (crossingWaysDescription) {
            crossingWaysString = CrossingWaysDescriptionToString(*crossingWaysDescription);
        }

        if (!crossingWaysString.empty()) {
            stream << "At crossing " << crossingWaysString << std::endl;
        }

        if (directionDescription) {
            osmscout::RouteDescription::DirectionDescription::Move move = directionDescription->GetCurve();
            if (!crossingWaysString.empty()) {
                stream << " " << MoveToTurnCommand(move);
            } else {
                stream << MoveToTurnCommand(move);
            }
        } else {
            if (!crossingWaysString.empty()) {
                stream << " turn";
            } else {
                stream << "Turn";
            }
        }

        if (nameDescription &&
            nameDescription->HasName()) {
            stream << " to '" << nameDescription->GetDescription() << "'";
        }
        description.instructions = stream.str();
        return description;
    }

    std::string DumpRoundaboutEnterDescription(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& /*roundaboutEnterDescription*/,
                                               const osmscout::RouteDescription::CrossingWaysDescriptionRef& /*crossingWaysDescription*/)
    {
        std::ostringstream stream;
        std::string crossingWaysString;
        stream << "Enter in the roundabout, then ";
        return stream.str();
    }

    static std::string digitToOrdinal(size_t digit){
        switch(digit){
            case 1:
                return "first";
            case 2:
                return "second";
            case 3:
                return "third";
            case 4:
                return "fourth";
            default: {
                char str[32];
                std::snprintf(str, sizeof(str), "number %zu", digit);
                return str;
            }
        }
    }

    std::string DumpRoundaboutLeaveDescription(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                               const osmscout::RouteDescription::NameDescriptionRef& nameDescription, size_t /*roundaboutCrossingCounter*/)
    {
        std::ostringstream stream;
        size_t exitCount = roundaboutLeaveDescription->GetExitCount();
        if(exitCount>0 && exitCount<4){
            stream << "take the "<<digitToOrdinal(exitCount)<<" exit";
        } else {
            stream << "take the exit " << exitCount;
        }

        if (nameDescription &&
            nameDescription->HasName()) {
            stream << ", to '" << nameDescription->GetDescription() << "'";
        }

        return stream.str();
    }

    NodeDescription DumpMotorwayEnterDescription(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                                 const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription)
    {
        NodeDescription desc;
        std::ostringstream stream;
        std::string crossingWaysString;

        if (crossingWaysDescription) {
            crossingWaysString=CrossingWaysDescriptionToString(*crossingWaysDescription);
        }

        if (!crossingWaysString.empty()) {
            stream << "At the crossing " << crossingWaysString << std::endl;
        }

        if (motorwayEnterDescription->GetToDescription() &&
            motorwayEnterDescription->GetToDescription()->HasName()) {
            if (!crossingWaysString.empty()) {
                stream << " enter the motorway";
            } else {
                stream << "Enter the motorway";
            }
            stream << " '" << motorwayEnterDescription->GetToDescription()->GetDescription() << "'";
        }
        desc.instructions = stream.str();
        return desc;
    }

    NodeDescription DumpMotorwayChangeDescription(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription)
    {
        NodeDescription desc;
        std::ostringstream stream;

        if (motorwayChangeDescription->GetFromDescription() &&
            motorwayChangeDescription->GetFromDescription()->HasName()) {
            stream << "Change motorway";
            stream << " from '" << motorwayChangeDescription->GetFromDescription()->GetDescription() << "'";
        }

        if (motorwayChangeDescription->GetToDescription() &&
            motorwayChangeDescription->GetToDescription()->HasName()) {
            stream << " to '" << motorwayChangeDescription->GetToDescription()->GetDescription() << "'";
        }
        desc.instructions = stream.str();
        return desc;
    }

    NodeDescription DumpMotorwayLeaveDescription(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                                 const osmscout::RouteDescription::DirectionDescriptionRef& /*directionDescription*/,
                                                 const osmscout::RouteDescription::NameDescriptionRef& nameDescription,
                                                 const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunction)
    {
        std::ostringstream stream;
        NodeDescription desc;
        desc.roundaboutExitNumber = 0;

        if (motorwayLeaveDescription->GetFromDescription() &&
            motorwayLeaveDescription->GetFromDescription()->HasName()) {
            stream << "Leave the motorway";
            stream << " '" << motorwayLeaveDescription->GetFromDescription()->GetDescription() << "'";
        }

        if (nameDescription &&
            nameDescription->HasName()) {
            stream << " to '" << nameDescription->GetDescription() << "'";
        }
        if (motorwayJunction){
            if(!motorwayJunction->GetJunctionDescription()->GetName().empty()){
                stream << " exit '" << motorwayJunction->GetJunctionDescription()->GetName();
                if(!motorwayJunction->GetJunctionDescription()->GetRef().empty()){
                    stream << " (" << motorwayJunction->GetJunctionDescription()->GetRef() << ")";
                }
                stream << "'";
            } else {
                stream << " exit " << motorwayJunction->GetJunctionDescription()->GetRef();
            }
        }
        desc.instructions = stream.str();
        return desc;
    }

    NodeDescription DumpNameChangedDescription(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription)
    {
        NodeDescription desc;
        std::ostringstream stream;
        if (nameChangedDescription->GetOriginDesccription() && nameChangedDescription->GetTargetDesccription()) {
            std::string originNameString=nameChangedDescription->GetOriginDesccription()->GetDescription();
            std::string targetNameString=nameChangedDescription->GetTargetDesccription()->GetDescription();

            if (!originNameString.empty() && originNameString.compare("unnamed road") &&
                !targetNameString.empty() && targetNameString.compare("unnamed road")) {
                stream << "Way changes name";
                stream << " from '" << nameChangedDescription->GetOriginDesccription()->GetDescription() << "'";
                stream << " to '" << nameChangedDescription->GetTargetDesccription()->GetDescription() << "'";

            }
        }
        desc.instructions = stream.str();
        return desc;
    }

    bool advanceToNextWaypoint(std::list<RouteDescription::Node>::const_iterator &waypoint,
                               std::list<RouteDescription::Node>::const_iterator end) {
        if(waypoint == end) return false;
        std::list<RouteDescription::Node>::const_iterator next = waypoint;
        std::advance(next,1);
        if (next == end){
            return false;
        } else {
            waypoint = next;
            return true;
        }
    }
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

int main(int argc, char *argv[]){
    osmscout::Navigation<osmscout::NodeDescription> navigation(new osmscout::NavigationDescription<osmscout::NodeDescription>);
    std::string                                     routerFilenamebase=osmscout::RoutingService::DEFAULT_FILENAME_BASE;
    osmscout::Vehicle                               vehicle=osmscout::vehicleCar;
    std::string                                     mapDirectory;
    bool                                            argumentError=false;

    double                                          startLat;
    double                                          startLon;

    double                                          targetLat;
    double                                          targetLon;

    double                                          latitude;
    double                                          longitude;

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
        else {
            // No more "special" arguments
            break;
        }
    }

    if (argumentError ||
        argc-currentArg!=7) {
        std::cout << "Routing" << std::endl;
        std::cout << "  [--router <router filename base>]" << std::endl;
        std::cout << "  [--foot | --bicycle | --car]" << std::endl;
        std::cout << "  <map directory>" << std::endl;
        std::cout << "  <start lat> <start lon>" << std::endl;
        std::cout << "  <target lat> <target lon>" << std::endl;
        std::cout << "  <location latitude> <location longitude>" << std::endl;
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

    if (sscanf(argv[currentArg],"%lf",&latitude)!=1) {
        std::cerr << "lat is not numeric!" << std::endl;
        return 1;
    }
    currentArg++;

    if (sscanf(argv[currentArg],"%lf",&longitude)!=1) {
        std::cerr << "lon is not numeric!" << std::endl;
        return 1;
    }
    currentArg++;

    osmscout::DatabaseParameter databaseParameter;
    osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

    if (!database->Open(mapDirectory)) {
        std::cerr << "Cannot open database" << std::endl;

        return 1;
    }

    osmscout::FastestPathRoutingProfileRef routingProfile=std::make_shared<osmscout::FastestPathRoutingProfile>(database->GetTypeConfig());
    osmscout::RouterParameter              routerParameter;

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

    std::set<std::string> motorwayTypeNames;
    std::set<std::string> motorwayLinkTypeNames;
    std::set<std::string> junctionTypeNames;
    auto*instructionProcessor=new osmscout::RoutePostprocessor::InstructionPostprocessor();
    if(vehicle == osmscout::vehicleCar){
        motorwayTypeNames = {"highway_motorway", "highway_motorway_trunk", "highway_motorway_primary"};
        motorwayLinkTypeNames = {"highway_motorway_link", "highway_trunk_link"};
        junctionTypeNames = {"highway_motorway_junction"};
        postprocessors.push_back(osmscout::RoutePostprocessor::PostprocessorRef(new osmscout::RoutePostprocessor::MaxSpeedPostprocessor()));
    }
    postprocessors.push_back(osmscout::RoutePostprocessor::PostprocessorRef(instructionProcessor));

    std::vector<osmscout::RoutingProfileRef> profiles = {routingProfile};
    std::vector<osmscout::DatabaseRef> databases = {database};
    osmscout::RoutePostprocessor postprocessor;
    if (!postprocessor.PostprocessRouteDescription(description,
                                                   profiles,
                                                   databases,
                                                   postprocessors,
                                                   motorwayTypeNames,
                                                   motorwayLinkTypeNames,
                                                   junctionTypeNames)) {
        std::cerr << "Error during route postprocessing" << std::endl;
        return 1;
    }

    //
    // Navigation
    //

    // Snap to route distance set to 100m
    navigation.SetSnapDistance(100.0);
    navigation.SetRoute(&description);

    osmscout::GeoCoord location(latitude, longitude);
    double minDistance = 0.0;

    navigation.UpdateCurrentLocation(location, minDistance);

    std::cout << "Distance to route: " << minDistance << std::endl;

    std::cout << "Distance from start: " << navigation.GetDistanceFromStart() << std::endl;
    std::cout << "Time from start: " << TimeToString(navigation.GetDurationFromStart()) << std::endl;

    std::cout << "Distance to destination: " << navigation.GetDistance() << std::endl;
    std::cout << "Time to destination: " << TimeToString(navigation.GetDuration()) << std::endl;

    osmscout::NodeDescription nextWaypointDescription = navigation.nextWaypointDescription();
    std::cout << "Next routing instructions: " <<  nextWaypointDescription.instructions << std::endl;

    return 0;
}
