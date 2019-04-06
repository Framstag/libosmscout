/*
 This source is part of the libosmscout library
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
#include <osmscout/navigation/DataAgent.h>

#include <chrono>

namespace osmscout {

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

  std::string PositionAgent::GpsPosition::GetStateStr(const Timestamp &now) const
  {
    switch (GetState(now)) {
      case Outdated:
        return "Outdated";
      case LowAccuracy:
        return "LowAccuracy";
      case Good:
        return "Good";
    }
    assert(false);
    return "";
  }

  std::string PositionAgent::Position::StateStr() const
  {
    switch (state) {
      case NoGpsSignal:
        return "NoGpsSignal";
      case OnRoute:
        return "OnRoute";
      case OffRoute:
        return "OffRoute";
      case EstimateInTunnel:
        return "EstimateInTunnel";
    }
    assert(false);
    return "";
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

  PositionAgent::PositionMessage::PositionMessage(const Timestamp& timestamp,
                                                  const RouteDescriptionRef &route,
                                                  const Position position):
                                                  NavigationMessage(timestamp),
                                                  route(route),
                                                  position(position)
  {
  }

  namespace {
    bool Includes(const RoutableObjectsRef &routableObjects,
                  const GeoBox &box){

      return routableObjects &&
          routableObjects->bbox.IsValid() &&
          routableObjects->bbox.Includes(box.GetMinCoord(), false) &&
          routableObjects->bbox.Includes(box.GetMaxCoord(), false);
    }

    PositionAgent::Position findNearest(const GeoCoord &coord,
                                        const RoutableObjectsRef &routableObjects)
    {
      assert(routableObjects);

      auto lookupArea=GeoBox::BoxByCenterAndRadius(coord,Meters(30));
      double nearest=std::numeric_limits<double>::max();

      PositionAgent::Position position;

      for (auto &e:routableObjects->dbMap){
        DatabaseId dbId=e.first;
        RoutableDBObjects &objs=e.second;

        for (auto &w:objs.ways){
          assert(w.second);
          if (w.second->GetBoundingBox().Intersects(lookupArea)){
            double distance=std::numeric_limits<double>::max();
            GeoCoord p{0,0};
            GeoCoord nearestPoint{0,0};
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
              position.databaseId=dbId;
              position.typeConfig=objs.typeConfig;
              position.way=w.second;
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

    static double one_degree_at_equator=111320.0;

    double distanceInDegrees(const Distance &d, double latitude)
    {
      return d.AsMeter()/(one_degree_at_equator*cos(M_PI*latitude/180));
    }

    double GetVehicleSpeed(osmscout::Vehicle vehicle,
                           const TypeConfig &/*typeConfig*/,
                           const TypeInfo &/*typeInfo*/)
    {
      switch (vehicle){
        case vehicleFoot:
          return 6;
        case vehicleBicycle:
          return 22;
        case vehicleCar:
          return 160;
      }
      assert(false);
      return 6;
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
      if (gps.GetState(now)!=Good ||
          gpsUpdateMessage->horizontalAccuracy < Meters(100)){
        gps.Update(now,
                   gpsUpdateMessage->currentPosition,
                   gpsUpdateMessage->horizontalAccuracy);

        if (!Includes(routableObjects,gps.GetGeoBox())){
          // we don't have routable data for current position, request data
          GeoBox requestBox = gps.GetGeoBox();
          requestBox.Include(GeoBox::BoxByCenterAndRadius(gps.position, Meters(200)));
          result.push_back(std::make_shared<RoutableObjectsRequestMessage>(now, requestBox));
        }
      }
    } else if (dynamic_cast<RoutableObjectsMessage*>(message.get())!=nullptr){
      auto msg=dynamic_cast<RoutableObjectsMessage*>(message.get());
      if (gps.GetState(now)==Good && Includes(msg->data,gps.GetGeoBox())){
        this->routableObjects = msg->data;
      }
    } else if (dynamic_cast<RouteUpdateMessage*>(message.get())!=nullptr) {
      auto routeUpdateMessage = dynamic_cast<RouteUpdateMessage *>(message.get());
      route=routeUpdateMessage->routeDescription;
      vehicle=routeUpdateMessage->vehicle;
      position.routeNode=route->Nodes().begin();
    }

    if (!route){
      // we don't have route yet
      return result;
    }

    if (!Includes(routableObjects,gps.GetGeoBox())){
      // we don't have data for gpx position yet
      return result;
    }

    using namespace std::chrono;
    if (now-lastUpdate < milliseconds(100)){
      // don't update state too often
      return result;
    }

    if (gps.GetState(now)==Good){
      auto foundNode = position.routeNode;
      double foundAbscissa = 0.0;
      double minDistance = 0.0;
      GeoCoord coord{0,0};
      bool found=SearchClosestSegment(gps.position,
                                      position.routeNode,
                                      coord,
                                      foundNode,
                                      foundAbscissa,
                                      minDistance);
      if (found && foundNode!=route->Nodes().end()) {
        position.state=OnRoute;
        position.routeNode=foundNode;
        position.coord=coord;
        position.databaseId=foundNode->GetDatabaseId();
        position.typeConfig=routableObjects->GetTypeConfig(foundNode->GetDatabaseId());
        position.way=routableObjects->GetWay(foundNode->GetDatabaseId(), foundNode->GetPathObject());
        position.area=routableObjects->GetArea(foundNode->GetDatabaseId(), foundNode->GetPathObject());
      }else{
        position=findNearest(gps.position, routableObjects);
        position.state=OffRoute;
        position.routeNode=route->Nodes().begin(); // reset route position
      }
    } else {
      // gps signal with LowAccuracy or Outdated
      Distance moveEstimate;
      do {
        if (position.way && // TODO: handle area
            position.routeNode != route->Nodes().end() &&
            (position.state == OnRoute || position.state == EstimateInTunnel)) {

          auto nextNode = position.routeNode;
          nextNode++;
          if (nextNode==route->Nodes().end()){
            break;
          }

          assert(routableObjects->dbMap.find(position.databaseId) != routableObjects->dbMap.end());
          TypeConfigRef typeConfig = routableObjects->dbMap[position.databaseId].typeConfig;
          assert(typeConfig);
          TunnelFeatureReader tunnelReader(*typeConfig);
          bool tunnel = tunnelReader.IsSet(position.way->GetFeatureValueBuffer());
          if (tunnel) {
            position.state = EstimateInTunnel;
            MaxSpeedFeatureValueReader maxSpeedReader(*typeConfig);
            auto maxSpeed = maxSpeedReader.GetValue(position.way->GetFeatureValueBuffer());
            double vehicleSpeed = GetVehicleSpeed(vehicle, *typeConfig, *position.way->GetType());
            double speed = maxSpeed ? std::min(static_cast<double>(maxSpeed->GetMaxSpeed()), vehicleSpeed)
                                    : vehicleSpeed;
            double lastUpdateInHours = duration_cast<duration<double, std::ratio<3600>>>(now - lastUpdate).count();
            if (lastUpdateInHours<=0 || speed <= 0){
              break;
            }
            moveEstimate = Kilometers(speed * lastUpdateInHours);
            auto distanceToNextNode = GetEllipsoidalDistance(position.coord, nextNode->GetLocation());
            if (distanceToNextNode < moveEstimate){
              auto bearing=GetSphericalBearingInitial(position.coord, nextNode->GetLocation());
              position.coord=GetEllipsoidalDistance(position.coord,bearing,moveEstimate);
              log.Debug() << "Estimate position in tunnel. "
                          << "Move " << moveEstimate.AsMeter() << " m "
                          << "to " << position.coord.GetDisplayText();
              moveEstimate=Meters(0);
            }else{
              moveEstimate=moveEstimate - distanceToNextNode;
              position.coord=nextNode->GetLocation();
              position.routeNode=nextNode;
              position.databaseId=nextNode->GetDatabaseId();
              position.typeConfig=typeConfig;
              position.way=routableObjects->GetWay(nextNode->GetDatabaseId(), nextNode->GetPathObject());
              position.area=routableObjects->GetArea(nextNode->GetDatabaseId(), nextNode->GetPathObject());
              if (distanceToNextNode.As<Kilometer>() > 0) {
                // move time
                auto timeToNextNode = duration<double, std::ratio<3600>>(distanceToNextNode.As<Kilometer>() / speed);
                lastUpdate=lastUpdate+duration_cast<Timestamp::duration>(timeToNextNode);
                if (lastUpdate > now){
                  // should not happen
                  log.Warn() << "Correction of time " << TimestampToISO8601TimeString(lastUpdate) << " > " << TimestampToISO8601TimeString(now);
                  lastUpdate=now;
                }
              }
            }
          }
        }
      } while (moveEstimate.AsMeter() > 0);

      if (position.state!=EstimateInTunnel){
        position.state=NoGpsSignal;
      }
    }

    log.Debug() << "GPS signal state: " << gps.GetStateStr(now) << ", "
                << "position state: " << position.StateStr() << ", "
                << "position " << position.coord.GetDisplayText();

    result.push_back(std::make_shared<PositionMessage>(now, route, position));

    lastUpdate = now;
    return result;
  }

  /**
     * return true and set foundNode with the start node of the closest route segment from the location and foundAbscissa with the abscissa
     * of the projected point on the line, return false if there is no such point that is closer than snapDistanceInMeters
     * from the route.
     * The search start a the locationOnRoute node toward the end.
     */
  bool PositionAgent::SearchClosestSegment(const GeoCoord& location,
                                           const std::list<RouteDescription::Node>::const_iterator locationOnRoute,
                                           GeoCoord &closestPosition,
                                           std::list<RouteDescription::Node>::const_iterator& foundNode,
                                           double& foundAbscissa,
                                           double& minDistance)
  {
    auto   nextNode=locationOnRoute;
    double abscissa=0.0;
    bool   found=false;
    double qLon,qLat;
    double snapDistanceInDegrees = distanceInDegrees(snapDistanceInMeters,
                                                     location.GetLat());

    minDistance=std::numeric_limits<double>::max();
    for (auto node=nextNode++; node!=route->Nodes().end(); node++) {
      if (nextNode==route->Nodes().end()) {
        break;
      }
      double d=DistanceToSegment(location.GetLon(),
                                 location.GetLat(),
                                 node->GetLocation().GetLon(),
                                 node->GetLocation().GetLat(),
                                 nextNode->GetLocation().GetLon(),
                                 nextNode->GetLocation().GetLat(),
                                 abscissa,
                                 qLon,
                                 qLat);
      if (minDistance>=d) {
        minDistance=d;
        if (d<=snapDistanceInDegrees) {
          foundNode    =node;
          foundAbscissa=abscissa;
          found        =true;
          closestPosition.Set(qLat,qLon);
        }
      }
      else if (found && d>minDistance*2) {
        // Stop the search we have a good candidate
        break;
      }
      nextNode++;
    }

    return found;
  }

}