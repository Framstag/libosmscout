/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/RoutingProfile.h>

#include <limits>
#include <iostream>

#include <osmscout/system/Assert.h>

namespace osmscout {

  RoutingProfile::~RoutingProfile()
  {
    // no code
  }

  AbstractRoutingProfile::AbstractRoutingProfile(const TypeConfigRef& typeConfig)
   : typeConfig(typeConfig),
     accessReader(typeConfig),
     maxSpeedReader(typeConfig),
     vehicle(vehicleCar),
     vehicleRouteNodeBit(RouteNode::usableByCar),
     minSpeed(0),
     maxSpeed(0),
     vehicleMaxSpeed(std::numeric_limits<double>::max())
  {
    // no code
  }

  void AbstractRoutingProfile::SetVehicle(Vehicle vehicle)
  {
    this->vehicle=vehicle;

    switch (vehicle) {
    case vehicleFoot:
      vehicleRouteNodeBit=RouteNode::usableByFoot;
      break;
    case vehicleBicycle:
      vehicleRouteNodeBit=RouteNode::usableByBicycle;
      break;
    case vehicleCar:
      vehicleRouteNodeBit=RouteNode::usableByCar;
      break;
    }
  }

  void AbstractRoutingProfile::SetVehicleMaxSpeed(double maxSpeed)
  {
    vehicleMaxSpeed=maxSpeed;
  }

  void AbstractRoutingProfile::ParametrizeForFoot(const TypeConfig& typeConfig,
                                                  double maxSpeed)
  {
    speeds.clear();

    SetVehicle(vehicleFoot);
    SetVehicleMaxSpeed(maxSpeed);

    for (auto type : typeConfig.GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanRouteFoot()) {
        AddType(type->GetId(),maxSpeed);
      }
    }
  }

  void AbstractRoutingProfile::ParametrizeForBicycle(const TypeConfig& typeConfig,
                                                     double maxSpeed)
  {
    speeds.clear();

    SetVehicle(vehicleBicycle);
    SetVehicleMaxSpeed(maxSpeed);

    for (auto type : typeConfig.GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanRouteBicycle()) {
        AddType(type->GetId(),maxSpeed);
      }

    }
  }

  bool AbstractRoutingProfile::ParametrizeForCar(const osmscout::TypeConfig& typeConfig,
                                                 const std::map<std::string,double>& speedMap,
                                                 double maxSpeed)
  {
    bool everythingResolved=true;

    speeds.clear();

    SetVehicle(vehicleCar);
    SetVehicleMaxSpeed(maxSpeed);

    for (auto type : typeConfig.GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanRouteCar()) {
        std::map<std::string,double>::const_iterator speed=speedMap.find(type->GetName());

        if (speed==speedMap.end()) {
          std::cerr << "No speed for type '" << type->GetName() << "' defined!" << std::endl;
          everythingResolved=false;

          continue;
        }

        AddType(type->GetId(),speed->second);
      }
    }

    return everythingResolved;
  }

  void AbstractRoutingProfile::AddType(TypeId type, double speed)
  {
    if (speeds.empty()) {
      minSpeed=speed;
      maxSpeed=speed;
    }
    else {
      minSpeed=std::min(minSpeed,speed);
      maxSpeed=std::max(maxSpeed,speed);
    }

    if (type>=speeds.size()) {
      speeds.resize(type+1,0.0);
    }

    speeds[type]=speed;
  }

  bool AbstractRoutingProfile::CanUse(const RouteNode& currentNode,
                                      size_t pathIndex) const
  {
    if (!(currentNode.paths[pathIndex].flags & vehicleRouteNodeBit)) {
      return false;
    }

    TypeId type=currentNode.paths[pathIndex].type;

    return type<speeds.size() && speeds[type]>0.0;
  }

  bool AbstractRoutingProfile::CanUse(const Area& area) const
  {
    if (area.rings.size()!=1) {
      return false;
    }

    TypeId type=area.rings[0].GetType()->GetId();

    return type<speeds.size() && speeds[type]>0.0;
  }

  bool AbstractRoutingProfile::CanUse(const Way& way) const
  {
    TypeId type=way.GetType()->GetId();

    if (type>=speeds.size() || speeds[type]<=0.0) {
      return false;
    }

    AccessFeatureValue *accessValue=accessReader.GetValue(way.GetFeatureValueBuffer());

    if (accessValue!=NULL) {
      switch (vehicle) {
      case vehicleFoot:
        return accessValue->CanRouteFoot();
        break;
      case vehicleBicycle:
        return accessValue->CanRouteBicycle();
        break;
      case vehicleCar:
        return accessValue->CanRouteCar();
        break;
      }
    }
    else {
      switch (vehicle) {
      case vehicleFoot:
        return way.GetType()->CanRouteFoot();
        break;
      case vehicleBicycle:
        return way.GetType()->CanRouteBicycle();
        break;
      case vehicleCar:
        return way.GetType()->CanRouteCar();
        break;
      }
    }

    return false;
  }

  bool AbstractRoutingProfile::CanUseForward(const Way& way) const
  {
    TypeId type=way.GetType()->GetId();

    if (type>=speeds.size() || speeds[type]<=0.0) {
      return false;
    }

    AccessFeatureValue *accessValue=accessReader.GetValue(way.GetFeatureValueBuffer());

    if (accessValue!=NULL) {
      switch (vehicle) {
      case vehicleFoot:
        return accessValue->CanRouteFootForward();
        break;
      case vehicleBicycle:
        return accessValue->CanRouteBicycleForward();
        break;
      case vehicleCar:
        return accessValue->CanRouteCarForward();
        break;
      }
    }
    else {
      switch (vehicle) {
      case vehicleFoot:
        return way.GetType()->CanRouteFoot();
        break;
      case vehicleBicycle:
        return way.GetType()->CanRouteBicycle();
        break;
      case vehicleCar:
        return way.GetType()->CanRouteCar();
        break;
      }
    }

    return false;
  }

  bool AbstractRoutingProfile::CanUseBackward(const Way& way) const
  {
    TypeId type=way.GetType()->GetId();

    if (type>=speeds.size() || speeds[type]<=0.0) {
      return false;
    }

    AccessFeatureValue *accessValue=accessReader.GetValue(way.GetFeatureValueBuffer());

    if (accessValue!=NULL) {
      switch (vehicle) {
      case vehicleFoot:
        return accessValue->CanRouteFootBackward();
        break;
      case vehicleBicycle:
        return accessValue->CanRouteBicycleBackward();
        break;
      case vehicleCar:
        return accessValue->CanRouteCarBackward();
        break;
      }
    }
    else {
      switch (vehicle) {
      case vehicleFoot:
        return way.GetType()->CanRouteFoot();
        break;
      case vehicleBicycle:
        return way.GetType()->CanRouteBicycle();
        break;
      case vehicleCar:
        return way.GetType()->CanRouteCar();
        break;
      }
    }

    return false;
  }

  ShortestPathRoutingProfile::ShortestPathRoutingProfile(const TypeConfigRef& typeConfig)
  : AbstractRoutingProfile(typeConfig)
  {
    // no code
  }

  FastestPathRoutingProfile::FastestPathRoutingProfile(const TypeConfigRef& typeConfig)
  : AbstractRoutingProfile(typeConfig)
  {
    // no code
  }
}

