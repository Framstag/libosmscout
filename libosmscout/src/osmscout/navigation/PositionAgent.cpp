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
#include <osmscout/navigation/DataAgent.h>

#include <chrono>

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

  PositionAgent::GpsPositionState PositionAgent::GpsPosition::GetState(const Timestamp &now) const
  {
    using namespace std::chrono;
    if (now-lastUpdate > seconds(2)){
      return Outdated;
    }
    if (horizontalAccuracy > Meters(100)){
      return LowAccuracy;
    }
    return Good;
  }

  GeoBox PositionAgent::GpsPosition::GetGeoBox() const
  {
    return GeoBox::BoxByCenterAndRadius(position,Meters(std::max(0.0,horizontalAccuracy.AsMeter())));
  }

  void PositionAgent::GpsPosition::Update(const Timestamp &time,
                                          const GeoCoord &position,
                                          const Distance &horizontalAccuracy)
  {
    this->lastUpdate=time;
    this->position=position;
    this->horizontalAccuracy=horizontalAccuracy;
  }

  namespace {
    bool Includes(const RoutableObjectsRef &routableObjects,
                  const GeoBox &box){

      return routableObjects &&
          routableObjects->bbox.IsValid() &&
          routableObjects->bbox.Includes(box.GetMinCoord(), false) &&
          routableObjects->bbox.Includes(box.GetMaxCoord(), false);
    }

    PositionAgent::PositionOnRoutableObject findNearest(const GeoCoord &coord,
        const RoutableObjectsRef &routableObjects)
    {
      assert(routableObjects);

      auto lookupArea=GeoBox::BoxByCenterAndRadius(coord,Meters(30));
      double nearest=std::numeric_limits<double>::max();

      PositionAgent::PositionOnRoutableObject position;

      for (auto &e:routableObjects->dbMap){
        DatabaseId dbId=e.first;
        RoutableDBObjects &objs=e.second;

        for (auto &w:objs.ways){
          assert(w.second);
          if (w.second->GetBoundingBox().Intersects(lookupArea)){
            double distance=std::numeric_limits<double>::max();
            GeoCoord p;
            GeoCoord nearestPoint;
            for (size_t i=1; i<w.second->nodes.size(); i++){
              double d = CalculateDistancePointToLineSegment(coord,
                                    w.second->nodes[i-1].GetCoord(),
                                    w.second->nodes[i].GetCoord(),
                                    p);
              if (d<distance){
                distance=d;
                nearestPoint=p;
              }
            }
            if (distance<nearest){
              position.coord=nearestPoint;
              position.way=w.second;
              position.wayDb=dbId;
              position.area.reset();
            }
          }
        }
        for (auto &a:objs.areas){
          assert(a.second);
          if (a.second->GetBoundingBox().Includes(coord, false)){
            // TODO
          }
        }
      }
      return position;
    }
  }

  PositionAgent::PositionAgent()
  {
  }

  std::list<NavigationMessageRef> PositionAgent::Process(const NavigationMessageRef& message)
  {
    std::list<NavigationMessageRef> result;
    auto now=message->timestamp;

    if (dynamic_cast<GPSUpdateMessage*>(message.get())!=nullptr) {
      auto gpsUpdateMessage=dynamic_cast<GPSUpdateMessage*>(message.get());
      // ignore gps update when we accuracy is too low and we have good data already
      if (position.GetState(now)!=Good ||
          gpsUpdateMessage->horizontalAccuracy < Meters(100)){
        position.Update(now,
                        gpsUpdateMessage->currentPosition,
                        gpsUpdateMessage->horizontalAccuracy);

        if (!Includes(routableObjects,position.GetGeoBox())){
          // we don't have routable data for current position, request data
          GeoBox requestBox = position.GetGeoBox();
          requestBox.Include(GeoBox::BoxByCenterAndRadius(position.position, Meters(200)));
          result.push_back(std::make_shared<RoutableObjectsRequestMessage>(now, requestBox));
        }
      }
    } else if (dynamic_cast<RoutableObjectsMessage*>(message.get())!=nullptr){
      auto msg=dynamic_cast<RoutableObjectsMessage*>(message.get());
      if (position.GetState(now)==Good && Includes(msg->data,position.GetGeoBox())){
        this->routableObjects = msg->data;
      }
    }

    if (!Includes(routableObjects,position.GetGeoBox())){
      return result;
    }

    using namespace std::chrono;
    if (now-lastUpdate < milliseconds(500)){
      return result;
    }

    // TODO
    PositionAgent::PositionOnRoutableObject objectPosition=findNearest(position.position, routableObjects);
    if (objectPosition.way) {
      assert(routableObjects->dbMap.find(objectPosition.wayDb)!=routableObjects->dbMap.end());
      TypeConfigRef typeConfig = routableObjects->dbMap[objectPosition.wayDb].typeConfig;
      assert(typeConfig);
      MaxSpeedFeatureValueReader maxSpeedReader(*typeConfig);
      AccessFeatureValueReader accessReader(*typeConfig);
      AccessRestrictedFeatureValueReader restrictionsReader(*typeConfig);
      TunnelFeatureReader tunnelReader(*typeConfig);

      bool tunnel = tunnelReader.IsSet(objectPosition.way->GetFeatureValueBuffer());
      log.Info() << "Here: " << objectPosition.coord.GetDisplayText();
      log.Info() << "Tunnel: " << tunnel;

      auto maxSpeed = maxSpeedReader.GetValue(objectPosition.way->GetFeatureValueBuffer());
      if (maxSpeed){
        log.Info() << "Max speed: " << maxSpeed->GetMaxSpeed();
      }

    }

    lastUpdate = now;
    return result;
  }

}