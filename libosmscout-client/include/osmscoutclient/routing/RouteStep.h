#ifndef OSMSCOUT_CLIENT_ROUTING_ROUTESTEP_H
#define OSMSCOUT_CLIENT_ROUTING_ROUTESTEP_H

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

#include <osmscoutclient/ClientImportExport.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/util/Distance.h>
#include <osmscout/util/LaneTurn.h>
#include <osmscout/util/Time.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace osmscout::client {

  /**
   * \ingroup Routing
   *
   * A language-agnostic record of one manoeuvre of a route.
   *
   * A step carries every description the route description attaches to its route node, the
   * identity of that node and its position along the route. It carries values as they exist in
   * the map data: no translated words, no units and no number formatting. Every label a user
   * reads is produced by the client that displays the step.
   *
   * Live guidance extends the same record with the progress facts of the current position
   * (`remainingDistance`, `remainingTime` and `nextNext`); a published step list entry leaves
   * them absent.
   */
  class OSMSCOUT_CLIENT_API RouteStep CLASS_FINAL
  {
  public:
    /**
     * The manoeuvre a step describes, derived by the producer for icons and row selection.
     * It is a convenience value, never a substitute for the description records below.
     */
    enum class Kind : uint8_t {
      unknown = 0,
      start,
      target,
      turn,
      roundaboutEnter,
      roundaboutLeave,
      motorwayEnter,
      motorwayChange,
      motorwayLeave,
      nameChange,
      informative
    };

    /**
     * A name with its optional alphanumeric reference (like B1 or A40).
     */
    struct Name
    {
      std::string name;
      std::string ref;
    };

    /**
     * The way's name changes from `from` to `to` at this node.
     */
    struct NameChanged
    {
      Name from;
      Name to;
    };

    /**
     * The ways that cross the route at this node.
     */
    struct CrossingWays
    {
      size_t              exitCount=0;
      std::optional<Name> origin;
      std::optional<Name> target;
      std::vector<Name>   others;
    };

    /**
     * The turn and the curve while getting from the previous node to the next node.
     */
    struct Direction
    {
      double                                       turnAngle=0.0;
      double                                       curveAngle=0.0;
      RouteDescription::DirectionDescription::Move turn=RouteDescription::DirectionDescription::straightOn;
      RouteDescription::DirectionDescription::Move curve=RouteDescription::DirectionDescription::straightOn;
    };

    struct RoundaboutEnter
    {
      bool clockwise=false;
    };

    struct RoundaboutLeave
    {
      size_t exitCountTaken=0;   //!< 1-based ordinal, in circulation order, of the exit taken
      bool   clockwise=false;
    };

    struct MotorwayEnter
    {
      Name to;
    };

    struct MotorwayChange
    {
      Name                                         from;
      Name                                         to;
      RouteDescription::DirectionDescription::Move move=RouteDescription::DirectionDescription::straightOn;
    };

    struct MotorwayLeave
    {
      Name                                         from;
      RouteDescription::DirectionDescription::Move move=RouteDescription::DirectionDescription::straightOn;
    };

    struct Lane
    {
      bool                  oneway=false;
      uint8_t               count=0;   //!< lanes in the direction of travel, not the sum of both
      std::vector<LaneTurn> turns;     //!< turns of the lanes from the left, own direction
    };

    struct SuggestedLane
    {
      uint8_t  from=0;   //!< left-most suggested lane, inclusive, counted from the left
      uint8_t  to=0;     //!< right-most suggested lane, inclusive
      LaneTurn turn=LaneTurn::None;
    };

    struct Poi
    {
      DatabaseId          databaseId=0;
      ObjectFileRef       object;
      std::optional<Name> name;
      Distance            distance;   //!< distance of the POI from the route start
    };

    struct Via
    {
      int sectionNumber=0;
      int nodeCount=0;
    };

  public:
    Kind                                                        kind=Kind::unknown;   //!< derived, not a source of facts
    DatabaseId                                                  databaseId=0;   //!< database of the node and its path object
    size_t                                                      currentNodeIndex=0;   //!< node index of pathObject
    ObjectFileRef                                               pathObject;   //!< object travelled along from this node
    size_t                                                      targetNodeIndex=0;   //!< target node index of pathObject
    GeoCoord                                                    coord;   //!< position of the node
    Distance                                                    distanceFromStart;   //!< distance from the route start
    Duration                                                    timeFromStart=Duration::zero();   //!< time from the route start
    std::vector<ObjectFileRef>                                  objects;   //!< objects intersecting the node
    Distance                                                    legDistance;   //!< distance from the preceding step
    Duration                                                    legTime=Duration::zero();   //!< time from the preceding step

    std::optional<Name>                                         name;   //!< NameDescription
    std::optional<NameChanged>                                  nameChanged;   //!< NameChangedDescription
    std::optional<std::string>                                  typeName;   //!< TypeNameDescription
    std::optional<CrossingWays>                                 crossingWays;   //!< CrossingWaysDescription
    std::optional<Direction>                                    direction;   //!< DirectionDescription
    std::optional<RouteDescription::DirectionDescription::Move> turn;   //!< TurnDescription
    std::optional<RoundaboutEnter>                              roundaboutEnter;
    std::optional<RoundaboutLeave>                              roundaboutLeave;
    std::optional<MotorwayEnter>                                motorwayEnter;
    std::optional<MotorwayChange>                               motorwayChange;
    std::optional<MotorwayLeave>                                motorwayLeave;
    std::optional<Name>                                         junction;   //!< MotorwayJunctionDescription, name and ref
    std::optional<std::string>                                  destination;   //!< DestinationDescription, raw semicolon separated text
    std::optional<uint8_t>                                      maxSpeed;   //!< MaxSpeedDescription, km/h
    std::optional<Lane>                                         lanes;   //!< LaneDescription
    std::optional<SuggestedLane>                                suggestedLanes;
    std::optional<Poi>                                          poi;   //!< POIAtRouteDescription
    std::optional<Via>                                          via;   //!< ViaDescription
    std::optional<std::string>                                  start;   //!< StartDescription, the start label
    std::optional<std::string>                                  target;   //!< TargetDescription, the destination label

    // Live guidance only: absent on a published step list entry.
    std::optional<Distance>    remainingDistance;   //!< distance to the manoeuvre
    std::optional<Duration>    remainingTime;       //!< time to the manoeuvre
    std::shared_ptr<RouteStep> nextNext;            //!< the manoeuvre after this one, when close

    RouteStep() = default;
    RouteStep(const RouteStep& other) = default;
    RouteStep(RouteStep && other) noexcept = default;
    ~RouteStep() = default;

    RouteStep& operator=(const RouteStep& other) = default;
    RouteStep& operator=(RouteStep&& other) noexcept = default;

    /**
     * Distance of the described node from the route start.
     *
     * This is the value the navigation engine orders its instruction list by, so a producer
     * satisfies `RouteInstructionAgent` with it.
     */
    Distance GetDistance() const
    {
      return distanceFromStart;
    }
  };

  /**
   * \ingroup Routing
   *
   * The published steps of a route, together with the mapping from database identity to map
   * directory that their node references point into.
   */
  class OSMSCOUT_CLIENT_API RouteSteps CLASS_FINAL
  {
  public:
    std::vector<RouteStep>            steps;
    std::map<DatabaseId, std::string> databaseMapping;

    RouteSteps() = default;
    RouteSteps(const RouteSteps& other) = default;
    RouteSteps(RouteSteps && other) noexcept = default;
    ~RouteSteps() = default;

    RouteSteps& operator=(const RouteSteps& other) = default;
    RouteSteps& operator=(RouteSteps&& other) noexcept = default;
  };
} // namespace osmscout::client

#endif
