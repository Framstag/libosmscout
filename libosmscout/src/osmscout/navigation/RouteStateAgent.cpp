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

#include <osmscout/navigation/PositionAgent.h>
#include <osmscout/navigation/RouteStateAgent.h>
#include <osmscout/navigation/BearingAgent.h>

#include <chrono>
#include <memory>

namespace osmscout {

  RerouteRequestMessage::RerouteRequestMessage(const Timestamp& timestamp,
                                               const GeoCoord &from,
                                               const std::shared_ptr<Bearing> &initialBearing,
                                               const GeoCoord &to)
      : NavigationMessage(timestamp),
        from(from),
        initialBearing(initialBearing),
        to(to)
  {
  }

  TargetReachedMessage::TargetReachedMessage(const Timestamp& timestamp,
                                             const GeoCoord &coord,
                                             const GeoCoord &target,
                                             const Bearing &targetBearing,
                                             const Distance &targetDistance)
      : NavigationMessage(timestamp),
        coord(coord),
        target(target),
        targetBearing(targetBearing),
        targetDistance(targetDistance)
  {
  }

  RouteStateAgent::RouteStateAgent()
      : state(PositionAgent::PositionState::NoGpsSignal)
  {
  }

  std::list<NavigationMessageRef> RouteStateAgent::Process(const NavigationMessageRef& message)
  {
    std::list<NavigationMessageRef> result;

    auto now = message->timestamp;

    if (dynamic_cast<RouteUpdateMessage*>(message.get())!=nullptr) {
      auto routeUpdateMessage = dynamic_cast<RouteUpdateMessage *>(message.get());

      if (!routeUpdateMessage->routeDescription->Nodes().empty()) {
        target = routeUpdateMessage->routeDescription->Nodes().rbegin()->GetLocation();
        targetSetup = true;
      }
    } else if (dynamic_cast<osmscout::BearingChangedMessage*>(message.get())!=nullptr) {
      auto bearingChangedMessage=dynamic_cast<osmscout::BearingChangedMessage*>(message.get());

      bearing = std::make_shared<Bearing>(bearingChangedMessage->bearing);

    } else if (dynamic_cast<PositionAgent::PositionMessage*>(message.get())!=nullptr) {
      auto positionMessage=dynamic_cast<PositionAgent::PositionMessage*>(message.get());
      auto &position = positionMessage->position;

      if (position.state==state){
        if (state==PositionAgent::PositionState::OffRoute &&
            (now-lastUpdate) > std::chrono::seconds(5) &&
            targetSetup){
          // when we are off-route more than five seconds, trigger rerouting
          result.push_back(std::make_shared<RerouteRequestMessage>(now,
              position.coord,
              bearing,
              target));
        }
      }else{
        state=position.state;
        lastUpdate=now;
      }

      if (targetSetup) {
        auto distanceToTarget = GetEllipsoidalDistance(position.coord, target);
        //log.Debug() << "Distance to target: " << distanceToTarget.AsMeter() << " m";
        if (position.state == PositionAgent::PositionState::OnRoute &&
            distanceToTarget < Meters(30)) {

          result.push_back(std::make_shared<TargetReachedMessage>(now,
                                                                  position.coord,
                                                                  target,
                                                                  GetSphericalBearingInitial(position.coord, target),
                                                                  distanceToTarget));
        }
      }
    }

    return result;
  }  
}