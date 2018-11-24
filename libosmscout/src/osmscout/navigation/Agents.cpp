/*
  This source is part of the libosmscout library
  Copyright (C) 2018  Tim Teulings

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

#include <osmscout/navigation/Agents.h>

namespace osmscout {

  GPSUpdateMessage::GPSUpdateMessage(const Timestamp& timestamp,
                                     bool hasPositionLock,
                                     const GeoCoord& currentPosition,
                                     double currentSpeed)
  : NavigationMessage(timestamp),
    hasPositionLock(hasPositionLock),
    currentPosition(currentPosition),
    currentSpeed(currentSpeed)
  {
  }

  PositionChangedMessage::PositionChangedMessage(const Timestamp& timestamp,
                                                 bool hasPositionLock,
                                                 const GeoCoord& currentPosition,
                                                 double currentSpeed)
    : NavigationMessage(timestamp),
      hasPositionLock(hasPositionLock),
      currentPosition(currentPosition),
      currentSpeed(currentSpeed)
  {
  }

  std::list<NavigationMessageRef> PositionAgent::Process(const NavigationMessageRef& message)
  {
    std::list<NavigationMessageRef> result;

    if (dynamic_cast<GPSUpdateMessage*>(message.get())!=nullptr) {
      auto gpsUpdateMessage=dynamic_cast<GPSUpdateMessage*>(message.get());

      // TODO: Keep track of the last relevant position events to interpolate
      // bearing and current position from them.

      result.push_back(std::make_shared<PositionChangedMessage>(gpsUpdateMessage->timestamp,
                                                                gpsUpdateMessage->hasPositionLock,
                                                                gpsUpdateMessage->currentPosition,
                                                                gpsUpdateMessage->currentSpeed));
    }
    else if (dynamic_cast<TimeTickMessage*>(message.get())!=nullptr) {
      //auto timeTickMessage=dynamic_cast<TimeTickMessage*>(message.get());

      // TODO: If gps signal is lost for some time start generating fake PositionChangedMessages
      // from position history
    }

    return result;
  }

  StreetChangedMessage::StreetChangedMessage(const Timestamp& timestamp,
                                             const std::string& name)
  : NavigationMessage(timestamp),
    name(name)
  {
  }

  CurrentStreetAgent::CurrentStreetAgent(const LocationDescriptionServiceRef& locationDescriptionService)
  : locationDescriptionService(locationDescriptionService)
  {
  }

  std::string CurrentStreetAgent::GetStreetName() const
  {
    osmscout::LocationDescription description;

    if (locationDescriptionService->DescribeLocationByWay(currentPosition,description)) {
      auto wayDescription=description.GetWayDescription();

      if (wayDescription && wayDescription->GetWay().GetLocation()) {
        return wayDescription->GetWay().GetLocation()->name;
      }
    }

    return "";
  }

  std::list<NavigationMessageRef> CurrentStreetAgent::Process(const NavigationMessageRef& message)
  {
    std::list<NavigationMessageRef> result;

    // We assume that we get regular position change events, either directly from the gps
    // or fake ones.
    if (dynamic_cast<PositionChangedMessage*>(message.get())!=nullptr) {
      auto positionChangedMessage=dynamic_cast<PositionChangedMessage*>(message.get());

      currentPosition=positionChangedMessage->currentPosition;
    }
    else if (dynamic_cast<TimeTickMessage*>(message.get())!=nullptr) {
      auto now=message->timestamp;

      if (now-lastCheckTime>std::chrono::seconds(5)) {

        auto currentStreetName=GetStreetName();

        if (lastStreetName!=currentStreetName) {
          result.push_back(std::make_shared<StreetChangedMessage>(message->timestamp,
                                                                    currentStreetName));

          lastStreetName=currentStreetName;
        }

        lastCheckTime=now;
      }

      // TODO: Do we have to handle position timeouts? What does that means? No change of position
      // or general signal loss and thus unknown position?
    }

    return result;
  }
}
