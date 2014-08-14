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

    for (TypeId typeId=0; typeId<=typeConfig.GetMaxTypeId(); typeId++) {
      if (!typeConfig.GetTypeInfo(typeId)->CanRouteFoot()) {
        continue;
      }

      AddType(typeId,maxSpeed);
    }
  }

  void AbstractRoutingProfile::ParametrizeForBicycle(const TypeConfig& typeConfig,
                                                     double maxSpeed)
  {
    speeds.clear();

    SetVehicle(vehicleBicycle);
    SetVehicleMaxSpeed(maxSpeed);

    for (TypeId typeId=0; typeId<=typeConfig.GetMaxTypeId(); typeId++) {
      if (!typeConfig.GetTypeInfo(typeId)->CanRouteBicycle()) {
        continue;
      }

      AddType(typeId,maxSpeed);
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

    for (TypeId typeId=0; typeId<=typeConfig.GetMaxTypeId(); typeId++) {
      TypeInfoRef type=typeConfig.GetTypeInfo(typeId);
      if (!type->CanRouteCar()) {
        continue;
      }

      std::map<std::string,double>::const_iterator speed=speedMap.find(type->GetName());

      if (speed==speedMap.end()) {
        std::cerr << "No speed for type '" << type->GetName() << "' defined!" << std::endl;
        everythingResolved=false;

        continue;
      }

      AddType(typeId,speed->second);
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

