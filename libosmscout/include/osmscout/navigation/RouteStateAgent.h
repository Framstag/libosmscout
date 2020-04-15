#ifndef OSMSCOUT_NAVIGATION_ROUTE_STATE_AGENT_H
#define OSMSCOUT_NAVIGATION_ROUTE_STATE_AGENT_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2018  Tim Teulings
 Copyright (C) 2019  Lukas Karas

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

#include <osmscout/navigation/Engine.h>
#include <osmscout/navigation/Agents.h>
#include <osmscout/navigation/PositionAgent.h>

#include <optional>

namespace osmscout {

  /**
   * Message created when we should compute new route.
   */
  struct OSMSCOUT_API RerouteRequestMessage CLASS_FINAL : public NavigationMessage
  {
    const GeoCoord from;
    const std::optional<Bearing> initialBearing;
    const GeoCoord to;

    RerouteRequestMessage(const Timestamp& timestamp,
                          const GeoCoord &from,
                          const std::optional<Bearing> &initialBearing,
                          const GeoCoord &to);
  };

  struct OSMSCOUT_API TargetReachedMessage CLASS_FINAL : public NavigationMessage
  {
    const GeoCoord coord;
    const GeoCoord target;
    const Bearing  targetBearing;
    const Distance targetDistance;

    TargetReachedMessage(const Timestamp& timestamp,
                         const GeoCoord &coord,
                         const GeoCoord &target,
                         const Bearing &targetBearing,
                         const Distance &targetDistance);
  };

  class OSMSCOUT_API RouteStateAgent CLASS_FINAL : public NavigationAgent
  {
  private:
    Timestamp lastUpdate;
    PositionAgent::PositionState state;
    std::optional<Bearing> bearing;
    GeoCoord target;
    bool targetSetup{false};

  public:
    explicit RouteStateAgent();
    std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;
  };
}
#endif
