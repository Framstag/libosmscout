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

namespace osmscout {

  class OSMSCOUT_API PositionAgent CLASS_FINAL : public NavigationAgent
  {
  private:
    enum class State {
      noCoord,
      oneCoord,
      twoCoords
    };

  private:
    GeoCoord  previousPosition;
    Timestamp previousCheckTime;

    GeoCoord  currentPosition;
    Timestamp currentCheckTime;
    double    currentSpeed;

    bool      hasBearing;
    double    currentBearing;

    State     currentState;

  private:
    void UpdateHistory(const GPSUpdateMessage* message);
    std::list<NavigationMessageRef> UpdateBearing();

  public:
    PositionAgent();
    std::list<NavigationMessageRef> Process(const NavigationMessageRef& message) override;
  };

}
#endif 
