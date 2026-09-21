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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <chrono>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <osmscout/GeoCoord.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/Way.h>
#include <osmscout/navigation/Agents.h>
#include <osmscout/navigation/DataAgent.h>
#include <osmscout/navigation/PositionAgent.h>
#include <osmscout/navigation/RouteInstructionAgent.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/util/Bearing.h>
#include <osmscout/util/Distance.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Time.h>

/*
 * Tests for how the navigation agents handle progress along the route
 * (spec turn-by-turn-instructions, javascout-navigation): the position agent reports the abscissa
 * of the snapped position and keeps its place on the route when the snap search fails, and the
 * route instruction agent publishes the next instruction for a position that is off route or has
 * no fresh GPS fix.
 *
 * The tests drive the agents directly with synthetic messages, a synthetic route and synthetic
 * routable objects, so they need neither a database nor a GPS device.
 */

namespace {

  using osmscout::GeoCoord;
  using osmscout::NavigationMessageRef;
  using osmscout::Timestamp;

  // The route of the fixtures runs west to east at a fixed latitude, so that a
  // position on the route is described by a fraction of the segment length.
  constexpr double               ROUTE_LAT = 51.5;
  constexpr double               NODE_0_LON = 7.40;
  constexpr double               NODE_1_LON = 7.41;
  constexpr double               NODE_2_LON = 7.42;

  constexpr osmscout::DatabaseId DATABASE_ID = 1;
  constexpr osmscout::FileOffset WAY_OFFSET = 1;

  Timestamp At(uint64_t seconds)
  {
    return Timestamp(std::chrono::seconds(seconds));
  }

  /**
   * A position `meters` away from `coord` in the given bearing.
   */
  GeoCoord Move(const GeoCoord& coord, double bearingDegrees, double meters)
  {
    return osmscout::GetEllipsoidalDistance(coord,
                                            osmscout::Bearing::Degrees(bearingDegrees),
                                            osmscout::Meters(meters));
  }

  /**
   * A position on the route between the two given longitudes, given as a fraction of that span.
   */
  GeoCoord OnRoute(double fromLon, double toLon, double fraction)
  {
    return GeoCoord(ROUTE_LAT, fromLon + fraction * (toLon - fromLon));
  }

  /**
   * The synthetic route, the way it travels on and the routable objects the agents
   * resolve against. The route has three nodes and the way is the straight line through all
   * three, so a position off the line is off route as well.
   */
  struct Fixture
  {
    osmscout::TypeConfigRef       typeConfig;
    osmscout::WayRef              way;
    osmscout::RouteDescriptionRef route;
    osmscout::RoutableObjectsRef  routableObjects;
    Timestamp                     t0{At(10000)};
  };

  Fixture MakeFixture()
  {
    Fixture fixture;

    fixture.typeConfig = std::make_shared<osmscout::TypeConfig>();

    auto wayType = std::make_shared<osmscout::TypeInfo>("test_way");

    wayType->CanBeWay(true);
    fixture.typeConfig->RegisterType(wayType);

    fixture.way = std::make_shared<osmscout::Way>();
    fixture.way->nodes.push_back(osmscout::Point(1, GeoCoord(ROUTE_LAT, NODE_0_LON)));
    fixture.way->nodes.push_back(osmscout::Point(2, GeoCoord(ROUTE_LAT, NODE_2_LON)));
    // A leg to the north, so that the way's bounding box reaches over a position that
    // is off the route: the nearest-object search only considers ways whose bounding
    // box intersects its lookup area.
    fixture.way->nodes.push_back(osmscout::Point(3, GeoCoord(ROUTE_LAT + 0.002, NODE_2_LON)));

    osmscout::FeatureValueBuffer wayFeatures;

    wayFeatures.SetType(wayType);
    fixture.way->SetFeatures(wayFeatures);

    const osmscout::ObjectFileRef              pathToNext(WAY_OFFSET, osmscout::refWay);
    const std::vector<osmscout::ObjectFileRef> noObjects;

    fixture.route = std::make_shared<osmscout::RouteDescription>();
    fixture.route->AddNode(DATABASE_ID, 0, noObjects, pathToNext, 1);
    fixture.route->AddNode(DATABASE_ID, 1, noObjects, pathToNext, 2);
    fixture.route->AddNode(DATABASE_ID, 2, noObjects, osmscout::ObjectFileRef(), 0);

    size_t index = 0;

    for (auto& node : fixture.route->Nodes()) {
      switch (index) {
      case 0:
        node.SetLocation(GeoCoord(ROUTE_LAT, NODE_0_LON));
        node.SetDistance(osmscout::Meters(0));
        break;
      case 1:
        node.SetLocation(GeoCoord(ROUTE_LAT, NODE_1_LON));
        node.SetDistance(osmscout::Meters(700));
        break;
      default:
        node.SetLocation(GeoCoord(ROUTE_LAT, NODE_2_LON));
        node.SetDistance(osmscout::Meters(1400));
        break;
      }
      index++;
    }

    fixture.routableObjects = std::make_shared<osmscout::RoutableObjects>();
    fixture.routableObjects->dbMap[DATABASE_ID].typeConfig = fixture.typeConfig;
    fixture.routableObjects->dbMap[DATABASE_ID].ways[WAY_OFFSET] = fixture.way;
    fixture.routableObjects->bbox =osmscout::GeoBox(GeoCoord(51.45, 7.35), GeoCoord(51.55, 7.45));

    return fixture;
  }

  /**
   * The index of a route node iterator within the route, or -1 when the iterator
   * does not point into that route at all.
   */
  int NodeIndex(const osmscout::RouteDescriptionRef& route,
                const osmscout::RouteDescription::NodeIterator& node)
  {
    int index = 0;

    for (auto current = route->Nodes().begin(); current != route->Nodes().end(); current++) {
      if (current == node) {
        return index;
      }
      index++;
    }

    return -1;
  }

  std::shared_ptr<osmscout::PositionAgent::PositionMessage> FindPosition(
    const std::list<NavigationMessageRef>& messages)
  {
    for (const auto& message : messages) {
      if (dynamic_cast<osmscout::PositionAgent::PositionMessage*>(message.get()) != nullptr) {
        return std::static_pointer_cast<osmscout::PositionAgent::PositionMessage>(message);
      }
    }

    return nullptr;
  }

  NavigationMessageRef MakePositionMessage(const Timestamp& timestamp,
                                           const osmscout::RouteDescriptionRef& route,
                                           osmscout::PositionAgent::PositionState state,
                                           const GeoCoord& coord,
                                           const osmscout::RouteDescription::NodeIterator& routeNode)
  {
    osmscout::PositionAgent::Position position;

    position.state = state;
    position.coord = coord;
    position.routeNode = routeNode;

    return std::make_shared<osmscout::PositionAgent::PositionMessage>(timestamp, route, position);
  }

  // ---------------------------------------------------------------------------
  // Route instruction agent
  // ---------------------------------------------------------------------------

  struct TestInstruction
  {
    double distance;

    osmscout::Distance GetDistance() const
    {
      return osmscout::Meters(distance);
    }
  };

  /**
   * A builder that only counts calls: the tests are about *whether* the agent asks for
   * an instruction, not about the instruction's content.
   */
  class TestInstructionBuilder
  {
  public:
    std::list<TestInstruction> GenerateRouteInstructions(
      const osmscout::RouteDescription::NodeIterator& /*first*/,
      const osmscout::RouteDescription::NodeIterator& /*last*/) const
    {
      // A distance past the route node's distance, so that the agent does not drop it.
      return {TestInstruction{100.0}};
    }

    TestInstruction GenerateNextRouteInstruction(
      const osmscout::RouteDescription::NodeIterator& /*previous*/,
      const osmscout::RouteDescription::NodeIterator& /*last*/,
      const GeoCoord& /*coord*/) const
    {
      return TestInstruction{50.0};
    }
  };

  size_t CountNextInstructions(const std::list<NavigationMessageRef>& messages)
  {
    size_t count = 0;

    for (const auto& message : messages) {
      if (dynamic_cast<osmscout::NextRouteInstructionsMessage<TestInstruction>*>(message.get()) !=
          nullptr) {
        count++;
      }
    }

    return count;
  }
}

TEST_CASE("Position agent reports the abscissa of the snapped position")
{
  Fixture                 fixture = MakeFixture();
  osmscout::PositionAgent agent;

  agent.Process(std::make_shared<osmscout::RouteUpdateMessage>(fixture.t0,
                                                               fixture.route,
                                                               osmscout::vehicleCar));
  agent.Process(std::make_shared<osmscout::GPSUpdateMessage>(fixture.t0 + std::chrono::seconds(1),
                                                             OnRoute(NODE_1_LON, NODE_2_LON, 0.25),
                                                             -1.0,
                                                             osmscout::Meters(20)));

  auto messages = agent.Process(std::make_shared<osmscout::RoutableObjectsMessage>(
                                  fixture.t0 + std::chrono::seconds(2),
                                  fixture.routableObjects));

  auto position = FindPosition(messages);

  REQUIRE(position != nullptr);
  REQUIRE(position->position.state == osmscout::PositionAgent::PositionState::OnRoute);
  REQUIRE(position->position.abscissa == Catch::Approx(0.25).margin(0.01));
  REQUIRE(NodeIndex(fixture.route, position->position.routeNode) == 1);
  REQUIRE(position->position.databaseId == DATABASE_ID);
  REQUIRE(position->position.way == fixture.way);

  // Progress along the same segment increases the abscissa.
  auto later = agent.Process(std::make_shared<osmscout::GPSUpdateMessage>(
                               fixture.t0 + std::chrono::seconds(3),
                               OnRoute(NODE_1_LON, NODE_2_LON, 0.5),
                               -1.0,
                               osmscout::Meters(20)));

  auto laterPosition = FindPosition(later);

  REQUIRE(laterPosition != nullptr);
  REQUIRE(laterPosition->position.state == osmscout::PositionAgent::PositionState::OnRoute);
  REQUIRE(laterPosition->position.abscissa == Catch::Approx(0.5).margin(0.01));
  REQUIRE(laterPosition->position.abscissa > position->position.abscissa);
}

TEST_CASE("Position agent keeps its place on the route when the snap search fails")
{
  Fixture                 fixture = MakeFixture();
  osmscout::PositionAgent agent;

  agent.Process(std::make_shared<osmscout::RouteUpdateMessage>(fixture.t0,
                                                               fixture.route,
                                                               osmscout::vehicleCar));

  // On route, in the second segment: the route node of the position is node 1,
  // so a reset to the route start is visible as a change to node 0.
  const GeoCoord onRoute = OnRoute(NODE_1_LON, NODE_2_LON, 0.25);

  agent.Process(std::make_shared<osmscout::GPSUpdateMessage>(fixture.t0 + std::chrono::seconds(1),
                                                             onRoute,
                                                             -1.0,
                                                             osmscout::Meters(20)));

  auto onRouteMessages = agent.Process(std::make_shared<osmscout::RoutableObjectsMessage>(
                                         fixture.t0 + std::chrono::seconds(2),
                                         fixture.routableObjects));

  auto onRoutePosition = FindPosition(onRouteMessages);

  REQUIRE(onRoutePosition != nullptr);
  REQUIRE(onRoutePosition->position.state == osmscout::PositionAgent::PositionState::OnRoute);
  REQUIRE(NodeIndex(fixture.route, onRoutePosition->position.routeNode) == 1);

  // 45 m north of the route: beyond the snap distance the agent compares against (the
  // tolerance is expressed in longitude degrees, which is about 32 m of latitude at this
  // latitude), and still within reach of the way, which extends to the north.
  auto messages = agent.Process(std::make_shared<osmscout::GPSUpdateMessage>(
                                  fixture.t0 + std::chrono::seconds(3),
                                  Move(onRoute, 0.0, 45.0),
                                  -1.0,
                                  osmscout::Meters(20)));

  auto position = FindPosition(messages);

  REQUIRE(position != nullptr);
  REQUIRE(position->position.state == osmscout::PositionAgent::PositionState::OffRoute);

  // The position stays where it was on the route instead of jumping back to the start,
  // and it carries no abscissa for the segment it is no longer on.
  REQUIRE(NodeIndex(fixture.route, position->position.routeNode) == 1);
  REQUIRE(position->position.routeNode != fixture.route->Nodes().begin());
  REQUIRE(position->position.abscissa == Catch::Approx(0.0));

  // The off-route position is still resolved to the way it is closest to.
  REQUIRE(position->position.way == fixture.way);
}

TEST_CASE("Route instruction agent publishes the next instruction without an on-route position")
{
  Fixture                                                                  fixture = MakeFixture();
  osmscout::RouteInstructionAgent<TestInstruction, TestInstructionBuilder> agent;

  const osmscout::RouteDescription::NodeIterator                           routeStart = fixture.route->Nodes().begin();
  const GeoCoord                                                           coord = OnRoute(NODE_0_LON, NODE_1_LON, 0.5);

  // Cold start on route: the route change publishes the instruction list and the next
  // instruction, the on-route state publishes the next instruction again.
  auto onRoute = agent.Process(MakePositionMessage(fixture.t0,
                                                   fixture.route,
                                                   osmscout::PositionAgent::PositionState::OnRoute,
                                                   coord,
                                                   routeStart));

  REQUIRE(CountNextInstructions(onRoute) == 2);

  // Off route: the UI must not freeze on the last instruction of the route.
  auto offRoute = agent.Process(MakePositionMessage(
                                  fixture.t0 + std::chrono::seconds(1),
                                  fixture.route,
                                  osmscout::PositionAgent::PositionState::OffRoute,
                                  coord,
                                  routeStart));

  REQUIRE(CountNextInstructions(offRoute) == 1);

  // No fresh GPS fix: the position is estimated, the next instruction stays computable.
  auto noGpsSignal = agent.Process(MakePositionMessage(
                                     fixture.t0 + std::chrono::seconds(2),
                                     fixture.route,
                                     osmscout::PositionAgent::PositionState::NoGpsSignal,
                                     coord,
                                     routeStart));

  REQUIRE(CountNextInstructions(noGpsSignal) == 1);

  // An uninitialised position has nothing to say about the route.
  auto uninitialised = agent.Process(MakePositionMessage(
                                       fixture.t0 + std::chrono::seconds(3),
                                       fixture.route,
                                       osmscout::PositionAgent::PositionState::Uninitialised,
                                       coord,
                                       routeStart));

  REQUIRE(uninitialised.empty());
}
