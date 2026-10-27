#ifndef OSMSCOUT_CLIENT_ROUTING_ROUTESTEPCOLLECTOR_H
#define OSMSCOUT_CLIENT_ROUTING_ROUTESTEPCOLLECTOR_H

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

#include <osmscoutclient/routing/RouteStep.h>

#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace osmscout::client {

  /**
   * \ingroup Routing
   *
   * Converts a route description into route steps: one step per route node that carries at
   * least one description, in route order, and no step for a node that carries none.
   *
   * The collector is the `RouteDescriptionPostprocessor::Callback` over every hook the
   * postprocessor offers. The hooks decide which manoeuvre a step describes; the facts of the
   * step are read from the node itself, so a description that has no hook of its own (lanes,
   * suggested lanes, max speed, a POI, a via section) still reaches the model.
   *
   * A description kind that no step member represents is reported through
   * `GetUnhandledDescriptions()` instead of being dropped silently.
   */
  class OSMSCOUT_CLIENT_API RouteStepCollector CLASS_FINAL : public RouteDescriptionPostprocessor::Callback
  {
  private:
    RouteSteps               result;
    std::vector<std::string> unhandledDescriptions;

    bool                     pendingKindSet=false;
    RouteStep::Kind          pendingKind=RouteStep::Kind::unknown;

    bool                     firstStep=true;
    Distance                 previousDistance;
    Duration                 previousTime=Duration::zero();
    Distance                 stopAfter=Distance::Lowest();   //!< < 0: unlimited

  private:
    void Reset();

    void NoteKind(RouteStep::Kind kind);

  public:
    RouteStepCollector() = default;
    ~RouteStepCollector() override = default;

    /**
     * Convert the whole description. The collector's report is reset first.
     */
    RouteSteps Collect(const RouteDescription& description);

    /**
     * Convert the nodes of the given range.
     */
    RouteSteps Collect(const RouteDescription::NodeIterator& first,
                       const RouteDescription::NodeIterator& last);

    /**
     * Convert the nodes of the given range, stopping behind `stopAfter`.
     *
     * @param stopAfter distance from the route start behind which no step is collected;
     *                  a negative value collects the whole range
     */
    RouteSteps Collect(const RouteDescription::NodeIterator& first,
                       const RouteDescription::NodeIterator& last,
                       const Distance& stopAfter);

    /**
     * The descriptions the collection could not represent, by their debug string.
     */
    const std::vector<std::string>& GetUnhandledDescriptions() const
    {
      return unhandledDescriptions;
    }

    /**
     * The set of description keys this collector handles. A key of `RouteDescription.h` that is
     * missing here is a description that would be dropped.
     */
    static const std::set<std::string>& HandledDescriptionKeys();

    /**
     * Build the step of a single node from the descriptions it carries, without the hooks.
     * Returns no step for a node that carries no description.
     */
    static std::optional<RouteStep> BuildNodeStep(const RouteDescription::Node& node);

    // RouteDescriptionPostprocessor::Callback
    void BeforeRoute() override;

    void OnStart(const RouteDescription::StartDescriptionRef& startDescription,
                 const RouteDescription::TypeNameDescriptionRef& typeNameDescription,
                 const RouteDescription::NameDescriptionRef& nameDescription) override;

    void OnTargetReached(const RouteDescription::TargetDescriptionRef& targetDescription) override;

    void OnTurn(const RouteDescription::TurnDescriptionRef& turnDescription,
                const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                const RouteDescription::DirectionDescriptionRef& directionDescription,
                const RouteDescription::TypeNameDescriptionRef& typeNameDescription,
                const RouteDescription::NameDescriptionRef& nameDescription) override;

    void OnRoundaboutEnter(const RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                           const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription) override;

    void OnRoundaboutLeave(const RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                           const RouteDescription::NameDescriptionRef& nameDescription) override;

    void OnMotorwayEnter(const RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                         const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription) override;

    void OnMotorwayChange(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                          const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                          const RouteDescription::DirectionDescriptionRef& directionDescription,
                          const RouteDescription::DestinationDescriptionRef& crossingDestinationDescription) override;

    void OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                         const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                         const RouteDescription::DirectionDescriptionRef& directionDescription,
                         const RouteDescription::NameDescriptionRef& nameDescription,
                         const RouteDescription::DestinationDescriptionRef& destinationDescription) override;

    void OnPathNameChange(const RouteDescription::NameChangedDescriptionRef& nameChangedDescription) override;

    void OnMaxSpeed(const RouteDescription::MaxSpeedDescriptionRef& maxSpeedDescription) override;

    void OnPOIAtRoute(const RouteDescription::POIAtRouteDescriptionRef& poiAtRouteDescription) override;

    void OnViaAtRoute(const RouteDescription::ViaDescriptionRef& viaDescription) override;

    void BeforeNode(const RouteDescription::Node& node) override;

    void AfterNode(const RouteDescription::Node& node) override;

    bool Continue() const override;
  };
} // namespace osmscout::client

#endif
