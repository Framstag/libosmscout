/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <catch2/catch_test_macros.hpp>

#include <osmscoutclient/routing/RouteStep.h>
#include <osmscoutclient/routing/RouteStepBuilder.h>
#include <osmscoutclient/routing/RouteStepCollector.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/navigation/PositionAgent.h>
#include <osmscout/navigation/RouteInstructionAgent.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/system/Assert.h>
#include <osmscout/util/Distance.h>
#include <osmscout/util/Time.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <list>
#include <regex>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

  using osmscout::client::RouteStep;
  using osmscout::client::RouteStepBuilder;
  using osmscout::client::RouteStepCollector;
  using osmscout::client::RouteSteps;

  using osmscout::DatabaseId;
  using osmscout::Distance;
  using osmscout::Duration;
  using osmscout::GeoCoord;
  using osmscout::ObjectFileRef;
  using osmscout::RouteDescription;

  constexpr DatabaseId databaseId=1;
  constexpr DatabaseId secondDatabaseId=2;

  osmscout::Meter      meter;

/**
 * A description kind that no step member represents: the shape of a kind that core would gain
 * without the model learning about it.
 */
  class SyntheticDescription : public RouteDescription::Description
  {
  public:
    std::string GetDebugString() const override
    {
      return "Synthetic description";
    }

    static constexpr std::string_view Key = "Synthetic";
  };

/**
 * The identity of the objects the fixtures travel along.
 */
  ObjectFileRef Way(uint64_t offset)
  {
    return ObjectFileRef(offset, osmscout::refWay);
  }

/**
 * Append a node with its position along the route.
 */
  RouteDescription::Node& AddNode(RouteDescription& description,
                                  size_t nodeIndex,
                                  double distanceMeters,
                                  double timeSeconds,
                                  const GeoCoord& coord)
  {
    description.AddNode(databaseId,
                        nodeIndex,
                        {},
                        Way(1000+nodeIndex),
                        nodeIndex+1);

    RouteDescription::Node & node=description.Nodes().back();

    node.SetDistance(Distance::Of<osmscout::Meter>(distanceMeters));
    node.SetTime(std::chrono::duration_cast<Duration>(std::chrono::duration<double>(timeSeconds)));
    node.SetLocation(coord);

    return node;
  }

/**
 * A node that carries a name, a type and a way name: what a plain turn carries.
 */
  void AddTurnDescriptions(RouteDescription::Node& node,
                           RouteDescription::DirectionDescription::Move move,
                           const std::string& wayName)
  {
    osmscout::RouteDescription::NameDescriptionRef name=std::make_shared<RouteDescription::NameDescription>(wayName);

    node.AddDescription(std::make_shared<RouteDescription::TurnDescription>(move));
    node.AddDescription(std::make_shared<RouteDescription::DirectionDescription>(45.0, 30.0));
    node.AddDescription(std::make_shared<RouteDescription::CrossingWaysDescription>(2, name, name));
    node.AddDescription(std::make_shared<RouteDescription::TypeNameDescription>("highway_secondary"));
    node.AddDescription(name);
  }

/**
 * Collect every string a step carries, so a test can prove that none of them is a composed
 * instruction sentence.
 */
  std::vector<std::string> Strings(const RouteStep& step)
  {
    std::vector<std::string> result;

    if (step.name) {
      result.push_back(step.name->name);
      result.push_back(step.name->ref);
    }

    if (step.nameChanged) {
      result.push_back(step.nameChanged->from.name);
      result.push_back(step.nameChanged->from.ref);
      result.push_back(step.nameChanged->to.name);
      result.push_back(step.nameChanged->to.ref);
    }

    if (step.typeName) {
      result.push_back(*step.typeName);
    }

    if (step.crossingWays) {
      if (step.crossingWays->origin) {
        result.push_back(step.crossingWays->origin->name);
        result.push_back(step.crossingWays->origin->ref);
      }
      if (step.crossingWays->target) {
        result.push_back(step.crossingWays->target->name);
        result.push_back(step.crossingWays->target->ref);
      }
      for (const auto& other : step.crossingWays->others) {
        result.push_back(other.name);
        result.push_back(other.ref);
      }
    }

    if (step.motorwayEnter) {
      result.push_back(step.motorwayEnter->to.name);
      result.push_back(step.motorwayEnter->to.ref);
    }

    if (step.motorwayChange) {
      result.push_back(step.motorwayChange->from.name);
      result.push_back(step.motorwayChange->from.ref);
      result.push_back(step.motorwayChange->to.name);
      result.push_back(step.motorwayChange->to.ref);
    }

    if (step.motorwayLeave) {
      result.push_back(step.motorwayLeave->from.name);
      result.push_back(step.motorwayLeave->from.ref);
    }

    if (step.junction) {
      result.push_back(step.junction->name);
      result.push_back(step.junction->ref);
    }

    if (step.destination) {
      result.push_back(*step.destination);
    }

    if (step.start) {
      result.push_back(*step.start);
    }

    if (step.target) {
      result.push_back(*step.target);
    }

    return result;
  }

/**
 * A route whose first node is the start, that turns at an undescribed node's neighbour and that
 * reaches the target, with one node in between that carries no description at all.
 */
  RouteDescription SimpleRoute()
  {
    RouteDescription description;

    description.SetDatabaseMapping({{databaseId, "/maps/region"},
                                     {secondDatabaseId, "/maps/basemap"}});

    auto & start=AddNode(description, 0, 0.0, 0.0, GeoCoord(52.0, 8.0));

    start.AddDescription(std::make_shared<RouteDescription::StartDescription>("Home"));
    start.AddDescription(std::make_shared<RouteDescription::NameDescription>("Hauptstrasse"));

    AddNode(description, 1, 300.0, 20.0, GeoCoord(52.002, 8.0));

    auto & turn=AddNode(description, 2, 3400.0, 140.0, GeoCoord(52.02, 8.01));

    AddTurnDescriptions(turn, RouteDescription::DirectionDescription::left, "Ringstrasse");

    auto & target=AddNode(description, 3, 5000.0, 300.0, GeoCoord(52.04, 8.02));

    target.AddDescription(std::make_shared<RouteDescription::TargetDescription>("Work"));

    return description;
  }

/**
 * A motorway exit: leaving the motorway at a junction that carries a name and an exit reference,
 * with sign destinations, a way type and a max speed.
 */
  RouteDescription MotorwayExitRoute()
  {
    RouteDescription description;

    description.SetDatabaseMapping({{databaseId, "/maps/region"}});

    auto & start=AddNode(description, 0, 0.0, 0.0, GeoCoord(52.0, 8.0));

    start.AddDescription(std::make_shared<RouteDescription::StartDescription>("Home"));
    start.AddDescription(std::make_shared<RouteDescription::MotorwayEnterDescription>(
                           std::make_shared<RouteDescription::NameDescription>("A2")));

    auto junctionName=std::make_shared<RouteDescription::NameDescription>("Kreuz Kamen", "12");

    auto & leave=AddNode(description, 1, 20000.0, 720.0, GeoCoord(52.1, 8.2));

    leave.AddDescription(std::make_shared<RouteDescription::MotorwayLeaveDescription>(
                           std::make_shared<RouteDescription::NameDescription>("A2"),
                           RouteDescription::DirectionDescription::right));
    leave.AddDescription(std::make_shared<RouteDescription::MotorwayJunctionDescription>(
                           junctionName));
    leave.AddDescription(std::make_shared<RouteDescription::DestinationDescription>("Dortmund;Kamen"));
    leave.AddDescription(std::make_shared<RouteDescription::TypeNameDescription>("highway_motorway_link"));
    leave.AddDescription(std::make_shared<RouteDescription::MaxSpeedDescription>(80));

    auto & target=AddNode(description, 2, 22000.0, 800.0, GeoCoord(52.12, 8.25));

    target.AddDescription(std::make_shared<RouteDescription::TargetDescription>("Work"));

    return description;
  }
}

TEST_CASE("The route step builder satisfies the instruction agent contract")
{
  // The navigation engine instantiates the agent template with the producer and the record; its
  // requirements are default-constructibility, copyability and GetDistance().
  osmscout::RouteInstructionAgent<osmscout::client::RouteStep,
                                  osmscout::client::RouteStepBuilder> agent;

  static_assert(std::is_default_constructible_v<RouteStepBuilder>);
  static_assert(std::is_copy_constructible_v<RouteStepBuilder>);
  static_assert(std::is_copy_assignable_v<RouteStepBuilder>);
  static_assert(std::is_same_v<decltype(std::declval<RouteStep>().GetDistance()),
                               osmscout::Distance>);

  // A message that is no position message produces no instruction, so the template compiles for
  // this producer/record pair.
  REQUIRE(agent.Process(nullptr).empty());
}

TEST_CASE("A description converts into one step per described node, in route order")
{
  RouteDescription   description=SimpleRoute();

  RouteStepCollector collector;

  RouteSteps         steps=collector.Collect(description);

  // start, turn, target - the node that carries no description produces no step.
  REQUIRE(steps.steps.size()==3);

  REQUIRE(steps.steps[0].kind==RouteStep::Kind::start);
  REQUIRE(steps.steps[1].kind==RouteStep::Kind::turn);
  REQUIRE(steps.steps[2].kind==RouteStep::Kind::target);

  REQUIRE(steps.steps[0].distanceFromStart.AsMeter()==0.0);
  REQUIRE(steps.steps[1].distanceFromStart.AsMeter()==3400.0);
  REQUIRE(steps.steps[2].distanceFromStart.AsMeter()==5000.0);

  REQUIRE(steps.steps[0].distanceFromStart.AsMeter()<=steps.steps[1].distanceFromStart.AsMeter());
  REQUIRE(steps.steps[1].distanceFromStart.AsMeter()<=steps.steps[2].distanceFromStart.AsMeter());

  // The published references: the mapping and the objects of the node.
  REQUIRE(steps.databaseMapping.size()==2);
  REQUIRE(steps.databaseMapping.at(databaseId)=="/maps/region");
}

TEST_CASE("Several descriptions of one node arrive together")
{
  RouteDescription   description=SimpleRoute();

  RouteStepCollector collector;

  RouteSteps         steps=collector.Collect(description);

  const RouteStep    & turn=steps.steps[1];

  REQUIRE(turn.turn==RouteDescription::DirectionDescription::left);
  REQUIRE(turn.direction.has_value());
  REQUIRE(turn.direction->turnAngle==45.0);
  REQUIRE(turn.direction->turn==RouteDescription::DirectionDescription::slightlyRight);
  REQUIRE(turn.direction->curve==RouteDescription::DirectionDescription::slightlyRight);
  REQUIRE(turn.crossingWays.has_value());
  REQUIRE(turn.crossingWays->exitCount==2);
  REQUIRE(turn.name.has_value());
  REQUIRE(turn.name->name=="Ringstrasse");
  REQUIRE(turn.typeName.has_value());
  REQUIRE(*turn.typeName=="highway_secondary");

  // The node's identity: two steps of the route at different nodes differ.
  REQUIRE(turn.databaseId==databaseId);
  REQUIRE(turn.currentNodeIndex==2);
  REQUIRE(turn.pathObject.GetFileOffset()==1002);
  REQUIRE(turn.currentNodeIndex!=steps.steps[0].currentNodeIndex);
}

TEST_CASE("Leg values belong to the step that ends a leg")
{
  RouteDescription   description=SimpleRoute();

  RouteStepCollector collector;

  RouteSteps         steps=collector.Collect(description);

  // First step's leg is its own node's values.
  REQUIRE(steps.steps[0].legDistance.AsMeter()==0.0);
  REQUIRE(steps.steps[1].legDistance.AsMeter()==3400.0);
  REQUIRE(steps.steps[2].legDistance.AsMeter()==1600.0);

  double legDistance=0.0;
  double legTime=0.0;

  for (const auto& step : steps.steps) {
    legDistance+=step.legDistance.AsMeter();
    legTime+=osmscout::DurationAsSeconds(step.legTime);
  }

  REQUIRE(legDistance==steps.steps.back().distanceFromStart.AsMeter());
  REQUIRE(legTime==osmscout::DurationAsSeconds(steps.steps.back().timeFromStart));
}

TEST_CASE("An absent description is absent")
{
  RouteDescription   description=SimpleRoute();

  RouteStepCollector collector;

  RouteSteps         steps=collector.Collect(description);

  const RouteStep    & turn=steps.steps[1];

  REQUIRE(!turn.maxSpeed.has_value());
  REQUIRE(!turn.lanes.has_value());
  REQUIRE(!turn.destination.has_value());
  REQUIRE(!turn.junction.has_value());
  REQUIRE(!turn.remainingDistance.has_value());
  REQUIRE(!turn.remainingTime.has_value());
}

TEST_CASE("A description kind that no conversion handles is reported")
{
  RouteDescription description=SimpleRoute();

  description.Nodes().back().AddDescription(std::make_shared<SyntheticDescription>());

  RouteStepCollector collector;

  RouteSteps         steps=collector.Collect(description);

  REQUIRE(steps.steps.size()==3);
  REQUIRE(collector.GetUnhandledDescriptions().size()==1);
  REQUIRE(collector.GetUnhandledDescriptions()[0]=="Synthetic description");
}

TEST_CASE("Every description kind of core is handled")
{
  const char * sourceRoot=getenv("SOURCE_ROOT");

  REQUIRE(sourceRoot!=nullptr);

  std::filesystem::path header=std::filesystem::path(sourceRoot) /
                                "libosmscout" / "include" / "osmscout" / "routing" /
                                "RouteDescription.h";

  REQUIRE(std::filesystem::exists(header));

  std::ifstream file(header);

  REQUIRE(file.is_open());

  std::set<std::string> coreKeys;
  std::regex            keyExpression("Key = \"([A-Za-z]+)\"");
  std::string           line;

  while (getline(file, line)) {
    std::smatch match;

    if (std::regex_search(line, match, keyExpression)) {
      coreKeys.insert(match[1]);
    }
  }

  const std::set<std::string> & handledKeys=RouteStepCollector::HandledDescriptionKeys();

  for (const auto& key : coreKeys) {
    INFO("Description kind " << key << " is not handled by the route step collector");
    REQUIRE(handledKeys.contains(key));
  }

  REQUIRE(coreKeys.size()==handledKeys.size());
}

TEST_CASE("A route step carries data, not a sentence")
{
  RouteDescription   description=MotorwayExitRoute();

  RouteStepCollector collector;

  RouteSteps         steps=collector.Collect(description);

  REQUIRE(steps.steps.size()==3);

  const RouteStep & leave=steps.steps[1];

  REQUIRE(leave.kind==RouteStep::Kind::motorwayLeave);

  // The exit reference and the junction name are their own values.
  REQUIRE(leave.junction.has_value());
  REQUIRE(leave.junction->ref=="12");
  REQUIRE(leave.junction->name=="Kreuz Kamen");

  // The sign destinations arrive as the raw tag, not split and not formatted.
  REQUIRE(leave.destination.has_value());
  REQUIRE(*leave.destination=="Dortmund;Kamen");

  // The way type and the max speed are values of the model data.
  REQUIRE(leave.typeName.has_value());
  REQUIRE(*leave.typeName=="highway_motorway_link");
  REQUIRE(leave.maxSpeed.has_value());
  REQUIRE(*leave.maxSpeed==80);

  // The motorway left, with its own name.
  REQUIRE(leave.motorwayLeave.has_value());
  REQUIRE(leave.motorwayLeave->from.name=="A2");

  // No member is a composed instruction sentence: every string is one of the values the fixture
  // put into the description.
  const std::set<std::string> fixtureValues{"A2", "Kreuz Kamen", "12", "Dortmund;Kamen",
                                            "highway_motorway_link", "Home", "Work", ""};

  for (const auto& value : Strings(leave)) {
    INFO("Step carried the string '" << value << "'");
    REQUIRE(fixtureValues.contains(value));
  }
}

TEST_CASE("Live guidance extends the same step with progress")
{
  RouteDescription description=SimpleRoute();

  RouteStepBuilder builder;

  auto             first=description.Nodes().cbegin();
  auto             last=description.Nodes().cend();

  RouteStep        firstStep=builder.GenerateNextRouteInstruction(first,
                                                                  last,
                                                                  GeoCoord(52.002, 8.0),
                                                                  0.25);
  RouteStep laterStep=builder.GenerateNextRouteInstruction(first,
                                                           last,
                                                           GeoCoord(52.002, 8.0),
                                                           0.75);

  // Both describe the same manoeuvre as the list entry of that node.
  REQUIRE(firstStep.kind==RouteStep::Kind::turn);
  REQUIRE(laterStep.kind==RouteStep::Kind::turn);
  REQUIRE(firstStep.currentNodeIndex==laterStep.currentNodeIndex);

  REQUIRE(firstStep.remainingDistance.has_value());
  REQUIRE(firstStep.remainingTime.has_value());
  REQUIRE(laterStep.remainingDistance.has_value());
  REQUIRE(laterStep.remainingTime.has_value());

  REQUIRE(laterStep.remainingDistance->AsMeter()<firstStep.remainingDistance->AsMeter());
  REQUIRE(osmscout::DurationAsSeconds(*laterStep.remainingTime)<
          osmscout::DurationAsSeconds(*firstStep.remainingTime));

  REQUIRE(firstStep.remainingDistance->AsMeter()>=0.0);
  REQUIRE(laterStep.remainingDistance->AsMeter()>=0.0);
  REQUIRE(osmscout::DurationAsSeconds(*firstStep.remainingTime)>=0.0);
  REQUIRE(osmscout::DurationAsSeconds(*laterStep.remainingTime)>=0.0);
}

TEST_CASE("List entries carry absolute values and the arrival is a step")
{
  RouteDescription     description=MotorwayExitRoute();

  RouteStepBuilder     builder;

  std::list<RouteStep> steps=builder.GenerateRouteInstructions(description.Nodes().cbegin(),
                                                               description.Nodes().cend());

  REQUIRE(steps.size()==3);

  for (const auto& step : steps) {
    REQUIRE(!step.remainingDistance.has_value());
    REQUIRE(!step.remainingTime.has_value());
  }

  const RouteStep & arrival=steps.back();

  REQUIRE(arrival.kind==RouteStep::Kind::target);
  REQUIRE(arrival.distanceFromStart.AsMeter()>0.0);
  REQUIRE(arrival.distanceFromStart.AsMeter()==22000.0);

  for (const auto& step : steps) {
    REQUIRE(arrival.distanceFromStart.AsMeter()>=step.distanceFromStart.AsMeter());
  }
}
