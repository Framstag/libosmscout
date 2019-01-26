#ifndef OSMSCOUT_NAVIGATION_ROUTE_POSITION_AGENT_H
#define OSMSCOUT_NAVIGATION_ROUTE_POSITION_AGENT_HC

/*
 This source is part of the libosmscout library
 Copyright (C) 2018  Lukas Karas

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
#include <osmscout/navigation/DataAgent.h>

namespace osmscout {

  /**
   * This agent pin position of vehicle to most probable routable object.
   * TODO: compute probable position inside tunnel where is no GPS signal
   */
  class OSMSCOUT_API PositionAgent CLASS_FINAL : public NavigationAgent
  {
  public:
    enum GpsPositionState {
      Good,
      LowAccuracy,
      Outdated
    };

    struct GpsPosition {
      Timestamp lastUpdate;
      GeoCoord position;
      Distance horizontalAccuracy{Meters(2000)};

      GpsPositionState GetState(const Timestamp &now) const;

      GeoBox GetGeoBox() const;

      void Update(const Timestamp &time,
                  const GeoCoord &position,
                  const Distance &horizontalAccuracy);
    };

    struct PositionOnRoutableObject {
      GeoCoord coord;
      WayRef way;
      DatabaseId wayDb;
      AreaRef area;
      DatabaseId areaDb;
    };

  private:
    GpsPosition position;
    Timestamp lastUpdate; // last update of agent state
    RoutableObjectsRef routableObjects; // routable objects around current position

  public:
    PositionAgent();

    std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;
  };

}
#endif 
