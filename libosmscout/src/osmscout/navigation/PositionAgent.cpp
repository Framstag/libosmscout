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

#include <osmscout/navigation/PositionAgent.h>

namespace osmscout {

  PositionChangedMessage::PositionChangedMessage(const Timestamp& timestamp,
                                                 const GeoCoord& currentPosition,
                                                 double currentSpeed)
      : NavigationMessage(timestamp),
        currentPosition(currentPosition),
        currentSpeed(currentSpeed)
  {
  }

  BearingChangedMessage::BearingChangedMessage(const Timestamp& timestamp)
      : NavigationMessage(timestamp),
        hasBearing(false),
        bearing(0.0)
  {
  }

  BearingChangedMessage::BearingChangedMessage(const Timestamp& timestamp,
                                               double bearing)
      : NavigationMessage(timestamp),
        hasBearing(true),
        bearing(bearing)
  {
  }

  PositionAgent::PositionAgent()
      : hasBearing(false),
        currentState(State::noCoord)
  {
  }

  void PositionAgent::UpdateHistory(const GPSUpdateMessage* message)
  {
    previousPosition=currentPosition;
    previousCheckTime=currentCheckTime;

    currentPosition=message->currentPosition;
    currentCheckTime=message->timestamp;
    currentSpeed=message->currentSpeed;

    switch (currentState) {
      case State::noCoord:
        currentState=State::oneCoord;
        break;
      case State::oneCoord:
        currentState=State::twoCoords;
        break;
      case State::twoCoords:
        break;
    }
  }

  std::list<NavigationMessageRef> PositionAgent::UpdateBearing()
  {
    std::list<NavigationMessageRef> result;

    if (currentState==State::twoCoords) {
      hasBearing=true;

      double newBearing=GetSphericalBearingInitial(previousPosition,currentPosition);

      if (currentBearing!=newBearing) {
        currentBearing=newBearing;

        result.push_back(std::make_shared<BearingChangedMessage>(currentCheckTime,
                                                                 currentBearing));
      }
    }

    return result;
  }

  std::list<NavigationMessageRef> PositionAgent::Process(const NavigationMessageRef& message)
  {
    std::list<NavigationMessageRef> result;

    if (dynamic_cast<GPSUpdateMessage*>(message.get())!=nullptr) {
      auto gpsUpdateMessage=dynamic_cast<GPSUpdateMessage*>(message.get());

      UpdateHistory(gpsUpdateMessage);

      auto bearingMessages=UpdateBearing();

      result.insert(result.end(),bearingMessages.begin(),bearingMessages.end());

      result.push_back(std::make_shared<PositionChangedMessage>(gpsUpdateMessage->timestamp,
                                                                gpsUpdateMessage->currentPosition,
                                                                gpsUpdateMessage->currentSpeed));
    }
    else if (dynamic_cast<TimeTickMessage*>(message.get())!=nullptr) {
      auto now=message->timestamp;

      if (currentState==State::twoCoords &&
          hasBearing &&
          now-currentCheckTime>std::chrono::seconds(5)) {
        previousPosition=currentPosition;
        previousCheckTime=currentCheckTime;

        auto timeSpanInHours=std::chrono::duration_cast<std::chrono::hours>(now-currentCheckTime).count();
        auto distance=osmscout::Distance::Of<osmscout::Kilometer>(currentSpeed*timeSpanInHours);

        currentPosition=previousPosition.Add(currentBearing*180/M_PI,
                                             distance);
        currentCheckTime=message->timestamp;


        result.push_back(std::make_shared<PositionChangedMessage>(message->timestamp,
                                                                  currentPosition,
                                                                  currentSpeed));
      }
    }

    return result;
  }

}