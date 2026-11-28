/*
  RouteSteps - a diagnostic program for libosmscout

  Prints the two layers a route instruction passes through, for one or more routes given as
  coordinate pairs:

    1. the route description's own nodes — which node carries which description, how many objects
       intersect it, and which node carries the signposted `Destination` record;
    2. the published step model (`RouteStepCollector`) the app reads — the manoeuvre kind, the
       motorway from/to, the destination the step carries and the way it names.

  The two tables answer the one question a screen cannot: a manoeuvre that shows no signposted
  destination either carries no record at all, or the record sits on a node the manoeuvre is not
  placed on. The verdict section names which one, per motorway enter/change, with the distance to
  the node that does carry a record.

  This is a diagnostic tool, not a test: it prints, it does not assert. The postprocessor chain it
  runs is the one `libosmscout-client-java/src/OSMScoutClient.cpp` runs, in the same order, because
  the order is what decides whether the destination reaches the manoeuvre.

  This program is free software: you can redistribute it and/or modify it under the terms of the
  GNU General Public License as published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.
*/

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <osmscout/db/Database.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/routing/MultiDBRoutingService.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/RoutingService.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/log/Logger.h>

#include <osmscoutclient/routing/RouteStep.h>
#include <osmscoutclient/routing/RouteStepCollector.h>

namespace {

  /**
   * One route the tool calculates: a label for the report and the two coordinates.
   */
  struct RouteRequest
  {
    std::string label;
    double      startLat=0.0;
    double      startLon=0.0;
    double      targetLat=0.0;
    double      targetLon=0.0;
  };

  /**
   * The description classes this tool reads, named once: `RouteDescription` is a class and a
   * namespace alias would not bring its nested classes into scope.
   */
  using StartDescription=osmscout::RouteDescription::StartDescription;
  using TargetDescription=osmscout::RouteDescription::TargetDescription;
  using NameDescription=osmscout::RouteDescription::NameDescription;
  using NameChangedDescription=osmscout::RouteDescription::NameChangedDescription;
  using TypeNameDescription=osmscout::RouteDescription::TypeNameDescription;
  using TurnDescription=osmscout::RouteDescription::TurnDescription;
  using DirectionDescription=osmscout::RouteDescription::DirectionDescription;
  using RoundaboutEnterDescription=osmscout::RouteDescription::RoundaboutEnterDescription;
  using RoundaboutLeaveDescription=osmscout::RouteDescription::RoundaboutLeaveDescription;
  using MotorwayEnterDescription=osmscout::RouteDescription::MotorwayEnterDescription;
  using MotorwayChangeDescription=osmscout::RouteDescription::MotorwayChangeDescription;
  using MotorwayLeaveDescription=osmscout::RouteDescription::MotorwayLeaveDescription;
  using MotorwayJunctionDescription=osmscout::RouteDescription::MotorwayJunctionDescription;
  using MaxSpeedDescription=osmscout::RouteDescription::MaxSpeedDescription;
  using DestinationDescription=osmscout::RouteDescription::DestinationDescription;

  /**
   * The command line the tool reads.
   */
  struct Options
  {
    std::vector<std::string> mapDirs;
    std::string              routesFile;
    osmscout::Vehicle        vehicle=osmscout::vehicleCar;
    bool                     allNodes=false;   //!< print nodes that carry no description too
    bool                     allSteps=false;   //!< print the informative steps too
    bool                     help=false;
  };

  void PrintHelp(const char* program)
  {
    std::cout
      << "Usage: " << program << " [options]\n"
      << "\n"
      << "  --map-dir DIR      a directory holding an opened map database (types.dat). Repeat it\n"
      << "                     once per database; a route may cross them.\n"
      << "  --routes FILE      route requests, one per line:\n"
      << "                       <label> <startLat> <startLon> <targetLat> <targetLon>\n"
      << "                     '#' starts a comment, a blank line is skipped.\n"
      << "  --foot             route for a pedestrian (default: car)\n"
      << "  --bicycle          route for a bicycle (default: car)\n"
      << "  --all-nodes        print every route node, not only the manoeuvre and destination ones\n"
      << "  --all-steps        print every step, informative ones included\n"
      << "  --help             this text\n"
      << "\n"
      << "Example:\n"
      << "  RouteSteps --map-dir ~/maps/germany --routes roads.txt\n"
      << std::endl;
  }

  /**
   * Read the route requests. A line that does not parse is reported and skipped, so one typo does
   * not hide the other routes of the file.
   */
  std::vector<RouteRequest> ReadRoutes(const std::string& path,
                                       std::ostream& errorStream)
  {
    std::vector<RouteRequest> routes;
    std::ifstream             in(path);

    if (!in) {
      errorStream << "ERROR: cannot open routes file '" << path << "'" << std::endl;
      return routes;
    }

    std::string line;
    size_t      lineNumber=0;

    while (std::getline(in,line)) {
      lineNumber++;

      const auto comment=line.find('#');
      if (comment!=std::string::npos) {
        line.erase(comment);
      }

      std::istringstream fields(line);
      RouteRequest       request;

      if (!(fields >> request.label >> request.startLat >> request.startLon
                   >> request.targetLat >> request.targetLon)) {
        if (!line.empty() && line.find_first_not_of(" \t\r")!=std::string::npos) {
          errorStream << "ERROR: " << path << ":" << lineNumber
                      << ": expected '<label> <startLat> <startLon> <targetLat> <targetLon>'"
                      << std::endl;
        }
        continue;
      }

      routes.push_back(request);
    }

    return routes;
  }

  /**
   * Parse the arguments by hand: the tool takes five options and no value syntax that would need
   * the command line parser.
   */
  bool ParseArguments(int argc,
                      char** argv,
                      Options& options)
  {
    for (int index=1; index<argc; index++) {
      const std::string argument=argv[index];

      if (argument=="--help" || argument=="-h") {
        options.help=true;
      }
      else if (argument=="--map-dir") {
        if (index+1>=argc) {
          std::cerr << "ERROR: --map-dir needs a directory" << std::endl;
          return false;
        }
        options.mapDirs.emplace_back(argv[++index]);
      }
      else if (argument=="--routes") {
        if (index+1>=argc) {
          std::cerr << "ERROR: --routes needs a file" << std::endl;
          return false;
        }
        options.routesFile=argv[++index];
      }
      else if (argument=="--foot") {
        options.vehicle=osmscout::vehicleFoot;
      }
      else if (argument=="--bicycle") {
        options.vehicle=osmscout::vehicleBicycle;
      }
      else if (argument=="--all-nodes") {
        options.allNodes=true;
      }
      else if (argument=="--all-steps") {
        options.allSteps=true;
      }
      else {
        std::cerr << "ERROR: unknown argument '" << argument << "'" << std::endl;
        return false;
      }
    }

    return true;
  }

  /**
   * The routing profile the app builds for a vehicle (the car speed map and the 120 km/h ceiling
   * of `CreateRoutingProfile` in the bridge, bicycle and foot by their built-in parametrisation).
   */
  osmscout::RoutingProfileRef CreateProfile(const osmscout::TypeConfigRef& typeConfig,
                                            osmscout::Vehicle vehicle)
  {
    auto profile=std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);

    switch (vehicle) {
      case osmscout::vehicleCar: {
        std::map<std::string,double> carSpeedMap;

        carSpeedMap["highway_motorway"]=110.0;
        carSpeedMap["highway_motorway_trunk"]=100.0;
        carSpeedMap["highway_motorway_primary"]=70.0;
        carSpeedMap["highway_motorway_link"]=60.0;
        carSpeedMap["highway_motorway_junction"]=60.0;
        carSpeedMap["highway_trunk"]=100.0;
        carSpeedMap["highway_trunk_link"]=60.0;
        carSpeedMap["highway_primary"]=70.0;
        carSpeedMap["highway_primary_link"]=60.0;
        carSpeedMap["highway_secondary"]=60.0;
        carSpeedMap["highway_secondary_link"]=50.0;
        carSpeedMap["highway_tertiary_link"]=55.0;
        carSpeedMap["highway_tertiary"]=55.0;
        carSpeedMap["highway_unclassified"]=50.0;
        carSpeedMap["highway_road"]=50.0;
        carSpeedMap["highway_residential"]=20.0;
        carSpeedMap["highway_roundabout"]=40.0;
        carSpeedMap["highway_living_street"]=10.0;
        carSpeedMap["highway_service"]=30.0;

        profile->ParametrizeForCar(*typeConfig,
                                  carSpeedMap,
                                  120.0);
        break;
      }
      case osmscout::vehicleBicycle:
        profile->ParametrizeForBicycle(*typeConfig,
                                      20.0);
        break;
      case osmscout::vehicleFoot:
        profile->ParametrizeForFoot(*typeConfig,
                                    5.0);
        break;
    }

    return profile;
  }

  /** The step kind, spelt as the app's `RouteStep.Kind` spells it. */
  std::string KindName(osmscout::client::RouteStep::Kind kind)
  {
    switch (kind) {
      case osmscout::client::RouteStep::Kind::start:
        return "START";
      case osmscout::client::RouteStep::Kind::target:
        return "TARGET";
      case osmscout::client::RouteStep::Kind::informative:
        return "INFORMATIVE";
      case osmscout::client::RouteStep::Kind::nameChange:
        return "NAME_CHANGE";
      case osmscout::client::RouteStep::Kind::turn:
        return "TURN";
      case osmscout::client::RouteStep::Kind::roundaboutEnter:
        return "ROUNDABOUT_ENTER";
      case osmscout::client::RouteStep::Kind::roundaboutLeave:
        return "ROUNDABOUT_LEAVE";
      case osmscout::client::RouteStep::Kind::motorwayEnter:
        return "MOTORWAY_ENTER";
      case osmscout::client::RouteStep::Kind::motorwayChange:
        return "MOTORWAY_CHANGE";
      case osmscout::client::RouteStep::Kind::motorwayLeave:
        return "MOTORWAY_LEAVE";
      case osmscout::client::RouteStep::Kind::unknown:
        return "UNKNOWN";
    }

    return "UNKNOWN";
  }

  std::string MoveName(osmscout::RouteDescription::DirectionDescription::Move move)
  {
    using Move=osmscout::RouteDescription::DirectionDescription::Move;

    switch (move) {
      case Move::sharpLeft:
        return "sharpLeft";
      case Move::left:
        return "left";
      case Move::slightlyLeft:
        return "slightlyLeft";
      case Move::straightOn:
        return "straightOn";
      case Move::slightlyRight:
        return "slightlyRight";
      case Move::right:
        return "right";
      case Move::sharpRight:
        return "sharpRight";
    }

    return "straightOn";
  }

  /**
   * The descriptions of one node, compactly: the manoeuvre descriptions the step model keys on, the
   * name and type, and — in capitals, so it greps — the signposted destination record.
   */
  std::string DescribeNode(const osmscout::RouteDescription::Node& node)
  {
    std::string text;

    auto append=[&text](const std::string& part) {
      if (!text.empty()) {
        text+=" ";
      }
      text+=part;
    };

    if (auto description=node.GetDescription<StartDescription>()) {
      append("START '"+description->GetDescription()+"'");
    }

    if (auto description=node.GetDescription<TargetDescription>()) {
      append("TARGET '"+description->GetDescription()+"'");
    }

    if (auto description=node.GetDescription<NameDescription>()) {
      std::string name="name '"+description->GetName()+"'";

      if (!description->GetRef().empty()) {
        name+=" ref '"+description->GetRef()+"'";
      }

      append(name);
    }

    if (auto description=node.GetDescription<NameChangedDescription>()) {
      if (description->GetTargetDescription()) {
        append("name->'" + description->GetTargetDescription()->GetName() + "'");
      }
    }

    if (auto description=node.GetDescription<TypeNameDescription>()) {
      append("type "+description->GetName());
    }

    if (auto description=node.GetDescription<TurnDescription>()) {
      append("turn "+MoveName(description->GetDirection()));
    }

    if (auto description=node.GetDescription<DirectionDescription>()) {
      append("direction "+MoveName(description->GetTurn()));
    }

    if (auto description=node.GetDescription<RoundaboutEnterDescription>()) {
      append(std::string("roundabout-enter ")+(description->IsClockwise() ? "clockwise" : "counter-clockwise"));
    }

    if (auto description=node.GetDescription<RoundaboutLeaveDescription>()) {
      append("roundabout-leave exit="+std::to_string(description->GetExitCount()));
    }

    if (auto description=node.GetDescription<MotorwayEnterDescription>()) {
      append("MOTORWAY_ENTER onto '"+description->GetToDescription()->GetName()+"'");
    }

    if (auto description=node.GetDescription<MotorwayChangeDescription>()) {
      append("MOTORWAY_CHANGE from '"+description->GetFromDescription()->GetName()+
             "' to '"+description->GetToDescription()->GetName()+"'");
    }

    if (auto description=node.GetDescription<MotorwayLeaveDescription>()) {
      append("MOTORWAY_LEAVE from '"+description->GetFromDescription()->GetName()+"'");
    }

    if (auto description=node.GetDescription<MotorwayJunctionDescription>()) {
      append("junction '"+description->GetJunctionDescription()->GetName()+"'");
    }

    if (auto description=node.GetDescription<MaxSpeedDescription>()) {
      append("maxspeed="+std::to_string(description->GetMaxSpeed()));
    }

    // The record this whole tool exists for: the signposted destination of the way the node
    // continues onto. Capitals, so a grep finds it in a long table.
    if (auto description=node.GetDescription<DestinationDescription>()) {
      append("DESTINATION '"+description->GetDescription()+"'");
    }

    return text;
  }

  /**
   * Whether a node is worth printing when the tool is not asked for every node: it carries a
   * manoeuvre, a destination record, or the start/target label. `WayName`/`WayType`/
   * `Direction` describe nearly every node of a route, and printing those hides the four nodes a
   * destination question is about.
   */
  bool IsInterestingNode(const osmscout::RouteDescription::Node& node)
  {
    return node.HasDescription<MotorwayEnterDescription>() ||
           node.HasDescription<MotorwayChangeDescription>() ||
           node.HasDescription<MotorwayLeaveDescription>() ||
           node.HasDescription<RoundaboutEnterDescription>() ||
           node.HasDescription<RoundaboutLeaveDescription>() ||
           node.HasDescription<TurnDescription>() ||
           node.HasDescription<DestinationDescription>() ||
           node.HasDescription<StartDescription>() ||
           node.HasDescription<TargetDescription>();
  }

  /**
   * Whether a published step is a manoeuvre a driver acts on: the same cut the step builder makes
   * when it looks for "the first manoeuvre after the position".
   */
  bool IsManoeuvreStep(const osmscout::client::RouteStep& step)
  {
    return step.kind!=osmscout::client::RouteStep::Kind::informative &&
           step.kind!=osmscout::client::RouteStep::Kind::unknown;
  }

  bool CarriesManoeuvre(const osmscout::RouteDescription::Node& node)
  {
    return node.HasDescription<MotorwayEnterDescription>() ||
           node.HasDescription<MotorwayChangeDescription>();
  }

  /**
   * The index of the next node at or after `from` that carries a destination record.
   */
  std::optional<size_t> NextDestinationNode(const std::vector<const osmscout::RouteDescription::Node*>& nodes,
                                            size_t from)
  {
    for (size_t index=from; index<nodes.size(); index++) {
      if (nodes[index]->HasDescription<osmscout::RouteDescription::DestinationDescription>()) {
        return index;
      }
    }

    return std::nullopt;
  }

  /** The node index a published step was built from: same path object, same node index. */
  std::optional<size_t> NodeOfStep(const std::vector<const osmscout::RouteDescription::Node*>& nodes,
                                   const osmscout::client::RouteStep& step)
  {
    for (size_t index=0; index<nodes.size(); index++) {
      const auto& node=*nodes[index];

      if (node.GetPathObject()==step.pathObject &&
          node.GetCurrentNodeIndex()==step.currentNodeIndex) {
        return index;
      }
    }

    return std::nullopt;
  }

  std::string MotorwayText(const osmscout::client::RouteStep& step)
  {
    auto plain=[](const osmscout::client::RouteStep::Name& name) {
      if (!name.name.empty() && !name.ref.empty()) {
        return name.name+" ("+name.ref+")";
      }

      return !name.name.empty() ? name.name : name.ref;
    };

    if (step.motorwayEnter) {
      return "-> "+plain(step.motorwayEnter->to);
    }

    if (step.motorwayChange) {
      return plain(step.motorwayChange->from)+" -> "+plain(step.motorwayChange->to);
    }

    if (step.motorwayLeave) {
      return plain(step.motorwayLeave->from)+" -> (leave)";
    }

    return "";
  }

  std::string StepName(const osmscout::client::RouteStep& step)
  {
    if (step.nameChanged) {
      const auto& name=step.nameChanged->to;

      if (!name.name.empty() && !name.ref.empty()) {
        return name.name+" ("+name.ref+")";
      }

      return !name.name.empty() ? name.name : name.ref;
    }

    if (step.name) {
      if (!step.name->name.empty() && !step.name->ref.empty()) {
        return step.name->name+" ("+step.name->ref+")";
      }

      return !step.name->name.empty() ? step.name->name : step.name->ref;
    }

    return "";
  }

  /** Whole kilometres with one decimal, as the census prints distances. */
  std::string Kilometres(double meters)
  {
    std::ostringstream text;

    text << std::fixed << std::setprecision(1) << (meters/1000.0);

    return text.str();
  }

  /**
   * Calculate one route and print both tables and the verdict.
   *
   * Returns false when the route could not be calculated; the caller counts the routes it reported,
   * so a failing request does not hide the others in the same file.
   */
  bool ReportRoute(const RouteRequest& request,
                   osmscout::MultiDBRoutingService& router,
                   const std::vector<osmscout::DatabaseRef>& databases,
                   const std::vector<osmscout::RoutingProfileRef>& profiles,
                   const std::set<std::string,std::less<>>& motorwayTypeNames,
                   const std::set<std::string,std::less<>>& motorwayLinkTypeNames,
                   const std::set<std::string,std::less<>>& junctionTypeNames,
                   bool allNodes,
                   bool allSteps)
  {
    std::cout << std::endl;
    std::cout << "=== route: " << request.label
              << " start=" << request.startLat << "," << request.startLon
              << " target=" << request.targetLat << "," << request.targetLon << std::endl;

    const osmscout::GeoCoord start{request.startLat,request.startLon};
    const osmscout::GeoCoord target{request.targetLat,request.targetLon};

    auto startResult=router.GetClosestRoutableNode(start,
                                                  osmscout::Kilometers(1));

    if (!startResult.IsValid()) {
      std::cout << "ERROR: no routable node near the start" << std::endl;
      return false;
    }

    auto targetResult=router.GetClosestRoutableNode(target,
                                                    osmscout::Kilometers(1));

    if (!targetResult.IsValid()) {
      std::cout << "ERROR: no routable node near the target" << std::endl;
      return false;
    }

    osmscout::RoutingParameter parameter;

    auto result=router.CalculateRoute(startResult.GetRoutePosition(),
                                      targetResult.GetRoutePosition(),
                                      std::nullopt,
                                      parameter);

    if (!result.Success()) {
      std::cout << "ERROR: the router returned no route" << std::endl;
      return false;
    }

    auto descriptionResult=router.TransformRouteDataToRouteDescription(result.GetRoute());

    if (!descriptionResult.Success() || !descriptionResult.GetDescription()) {
      std::cout << "ERROR: the route was not transformed into a description" << std::endl;
      return false;
    }

    osmscout::RouteDescriptionRef description=descriptionResult.GetDescription();

    // The postprocessor chain of libosmscout-client-java/src/OSMScoutClient.cpp, in its order: the
    // destination record is attached before the instruction postprocessor places the manoeuvres,
    // and the instruction postprocessor is what carries a record onto the node it words.
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

    if (!postprocessor.PostprocessRouteDescription(*description,
                                                   profiles,
                                                   databases,
                                                   postprocessors,
                                                   motorwayTypeNames,
                                                   motorwayLinkTypeNames,
                                                   junctionTypeNames)) {
      std::cout << "ERROR: the route description was not postprocessed" << std::endl;
      return false;
    }

    std::vector<const osmscout::RouteDescription::Node*> nodes;

    nodes.reserve(description->Nodes().size());

    for (const auto& node : description->Nodes()) {
      nodes.push_back(&node);
    }

    std::cout << "nodes=" << nodes.size()
              << " length_km=" << Kilometres(std::prev(description->Nodes().end())->GetDistance().AsMeter())
              << std::endl;

    std::cout << std::endl;
    std::cout << "--- route description nodes (what the description carries) ---" << std::endl;
    std::cout << std::left
              << std::setw(5)  << "node"
              << std::setw(8)  << "km"
              << std::setw(9)  << "objects"
              << std::setw(34) << "path"
              << "descriptions" << std::endl;

    for (size_t index=0; index<nodes.size(); index++) {
      const auto& node=*nodes[index];

      if (!allNodes && !IsInterestingNode(node)) {
        continue;
      }

      std::string path;

      if (node.HasPathObject()) {
        path=std::string(node.GetPathObject().GetTypeName())+"["+
             std::to_string(node.GetCurrentNodeIndex())+"->"+
             std::to_string(node.GetTargetNodeIndex())+"]";
      }
      else {
        path="(none)";
      }

      std::cout << std::left
                << std::setw(5)  << index
                << std::setw(8)  << Kilometres(node.GetDistance().AsMeter())
                << std::setw(9)  << node.GetObjects().size()
                << std::setw(34) << path
                << DescribeNode(node) << std::endl;
    }

    // The published step model: what the app's screens, notification and voice read.
    osmscout::client::RouteStepCollector collector;
    auto                             steps=collector.Collect(*description);

    std::cout << std::endl;
    std::cout << "--- published steps (RouteStepCollector) ---" << std::endl;
    std::cout << std::left
              << std::setw(5)  << "step"
              << std::setw(5)  << "node"
              << std::setw(8)  << "km"
              << std::setw(20) << "kind"
              << std::setw(26) << "motorway"
              << std::setw(26) << "destination"
              << "name" << std::endl;

    for (size_t index=0; index<steps.steps.size(); index++) {
      const auto& step=steps.steps[index];

      if (!allSteps && !IsManoeuvreStep(step)) {
        continue;
      }

      std::string nodeText="-";

      if (auto nodeIndex=NodeOfStep(nodes, step)) {
        nodeText=std::to_string(*nodeIndex);
      }

      std::cout << std::left
                << std::setw(5)  << index
                << std::setw(5)  << nodeText
                << std::setw(8)  << Kilometres(step.distanceFromStart.AsMeter())
                << std::setw(20) << KindName(step.kind)
                << std::setw(26) << MotorwayText(step)
                << std::setw(26) << (step.destination ? "'"+*step.destination+"'" : "-")
                << StepName(step) << std::endl;
    }

    // The verdict: per motorway enter/change, whether the manoeuvre's node carries the record, and
    // where the next record actually sits. This is the question the screen cannot answer.
    std::cout << std::endl;
    std::cout << "--- verdict: motorway enter/change and the destination record ---" << std::endl;

    size_t manoeuvres=0;
    size_t withRecord=0;
    size_t recordElsewhere=0;
    size_t noRecord=0;

    for (size_t index=0; index<nodes.size(); index++) {
      const auto& node=*nodes[index];

      if (!CarriesManoeuvre(node)) {
        continue;
      }

      manoeuvres++;

      const bool hasRecord=node.HasDescription<osmscout::RouteDescription::DestinationDescription>();

      if (hasRecord) {
        withRecord++;
      }

      std::ostringstream manoeuvre;

      if (auto description=node.GetDescription<osmscout::RouteDescription::MotorwayEnterDescription>()) {
        manoeuvre << "MOTORWAY_ENTER onto '" << description->GetToDescription()->GetName() << "'";
      }
      else if (auto description=node.GetDescription<osmscout::RouteDescription::MotorwayChangeDescription>()) {
        manoeuvre << "MOTORWAY_CHANGE from '" << description->GetFromDescription()->GetName()
                  << "' to '" << description->GetToDescription()->GetName() << "'";
      }

      std::cout << "node " << index
                << "  km=" << Kilometres(node.GetDistance().AsMeter())
                << "  " << manoeuvre.str();

      if (hasRecord) {
        std::cout << "  DESTINATION '"
                  << node.GetDescription<osmscout::RouteDescription::DestinationDescription>()->GetDescription()
                  << "'" << std::endl;
        continue;
      }

      if (auto next=NextDestinationNode(nodes, index+1)) {
        recordElsewhere++;

        std::cout << "  NO RECORD"
                  << "  (the next record is node " << *next
                  << " at +" << Kilometres(nodes[*next]->GetDistance().AsMeter()-
                                            node.GetDistance().AsMeter()) << " km: '"
                  << nodes[*next]->GetDescription<osmscout::RouteDescription::DestinationDescription>()->GetDescription()
                  << "')" << std::endl;
      }
      else {
        noRecord++;

        std::cout << "  NO RECORD  (no destination record anywhere ahead)" << std::endl;
      }
    }

    if (manoeuvres==0) {
      std::cout << "(no motorway enter or change on this route)" << std::endl;
    }

    std::cout << "summary: motorway_enter_or_change=" << manoeuvres
              << " with_destination=" << withRecord
              << " record_on_a_later_node=" << recordElsewhere
              << " no_record_ahead=" << noRecord << std::endl;

    return true;
  }
}

int main(int argc,
         char** argv)
{
  Options options;

  if (!ParseArguments(argc,
                      argv,
                      options)) {
    return 1;
  }

  if (options.help) {
    PrintHelp(argv[0]);
    return 0;
  }

  if (options.mapDirs.empty()) {
    std::cerr << "ERROR: at least one --map-dir is required" << std::endl;
    return 1;
  }

  if (options.routesFile.empty()) {
    std::cerr << "ERROR: --routes is required" << std::endl;
    return 1;
  }

  auto requests=ReadRoutes(options.routesFile,
                           std::cerr);

  if (requests.empty()) {
    std::cerr << "ERROR: no route request in '" << options.routesFile << "'" << std::endl;
    return 1;
  }

  osmscout::log.Info(false);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);

  osmscout::DatabaseParameter databaseParameter;
  std::vector<osmscout::DatabaseRef>     databases;
  std::vector<osmscout::RoutingProfileRef> profiles;

  for (const auto& dir : options.mapDirs) {
    auto database=std::make_shared<osmscout::Database>(databaseParameter);

    if (!database->Open(dir)) {
      std::cerr << "ERROR: cannot open the map database in '" << dir << "'" << std::endl;
      return 1;
    }

    databases.push_back(database);
    profiles.push_back(CreateProfile(database->GetTypeConfig(),
                                     options.vehicle));
  }

  osmscout::RouterParameter        routerParameter;
  osmscout::MultiDBRoutingService  router(routerParameter,
                                          databases);

  osmscout::Vehicle vehicle=options.vehicle;

  if (!router.Open([&profiles,&databases,vehicle](const osmscout::DatabaseRef& database) {
        for (size_t index=0; index<databases.size(); index++) {
          if (databases[index]==database) {
            return profiles[index];
          }
        }

        return CreateProfile(database->GetTypeConfig(),
                             vehicle);
      })) {
    std::cerr << "ERROR: cannot open the routing service" << std::endl;
    return 1;
  }

  const std::set<std::string,std::less<>> motorwayTypeNames{"highway_motorway",
                                                            "highway_motorway_trunk",
                                                            "highway_trunk",
                                                            "highway_motorway_primary"};
  const std::set<std::string,std::less<>> motorwayLinkTypeNames{"highway_motorway_link",
                                                                "highway_trunk_link"};
  const std::set<std::string,std::less<>> junctionTypeNames{"highway_motorway_junction"};

  std::cout << "RouteSteps: map_dirs=" << databases.size()
            << " vehicle=" << (options.vehicle==osmscout::vehicleCar ? "car" :
                               options.vehicle==osmscout::vehicleBicycle ? "bicycle" : "foot")
            << " routes=" << requests.size() << std::endl;

  size_t reported=0;

  for (const auto& request : requests) {
    if (ReportRoute(request,
                    router,
                    databases,
                    profiles,
                    motorwayTypeNames,
                    motorwayLinkTypeNames,
                    junctionTypeNames,
                    options.allNodes,
                    options.allSteps)) {
      reported++;
    }
  }

  std::cout << std::endl;
  std::cout << "routes reported=" << reported << " of " << requests.size() << std::endl;

  router.Close();

  return reported==0 ? 1 : 0;
}
