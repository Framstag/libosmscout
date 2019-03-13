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
#include <osmscout/navigation/BearingAgent.h>

#include <chrono>

namespace osmscout {

  BearingChangedMessage::BearingChangedMessage(const Timestamp& timestamp,
                                               const Bearing &bearing)
      : NavigationMessage(timestamp),
        bearing(bearing)
  {
  }

  BearingAgent::BearingAgent()
  {}

  std::list<NavigationMessageRef> BearingAgent::Process(const NavigationMessageRef& message)
  {
    std::list<NavigationMessageRef> result;

    auto now=message->timestamp;

    if (dynamic_cast<osmscout::PositionAgent::PositionMessage*>(message.get())!=nullptr) {
      auto msg=dynamic_cast<osmscout::PositionAgent::PositionMessage*>(message.get());
      auto coord=msg->position.coord;

      if (!previousPointValid){
        previousPointValid=true;
        previousPoing=coord;
        lastUpdate=now;
      }else if (GetSphericalDistance(previousPoing,coord) > Meters(2)){
        result.push_back(std::make_shared<BearingChangedMessage>(now, GetSphericalBearingInitial(previousPoing, coord)));
        previousPointValid=true;
        previousPoing=coord;
        lastUpdate=now;
      }
    } else if (previousPointValid &&
               (now-lastUpdate) > std::chrono::seconds(10)) {
      previousPointValid=false;
    }

    return result;
  }
}
