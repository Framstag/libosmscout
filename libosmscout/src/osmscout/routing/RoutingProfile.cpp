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

#include <osmscout/routing/RoutingProfile.h>

#include <limits>

#include <osmscout/util/Logger.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  RoutingProfile::~RoutingProfile()
  {
    // no code
  }

  AbstractRoutingProfile::AbstractRoutingProfile(const TypeConfigRef& typeConfig)
   : typeConfig(typeConfig),
     accessReader(*typeConfig),
     maxSpeedReader(*typeConfig),
     vehicle(vehicleCar),
     vehicleRouteNodeBit(RouteNode::usableByCar),
     costLimitDistance(10.0),
     costLimitFactor(5.0),
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

  /**
   * seet SetCostLimitFactor()
   *
   * @param costLimitDistance
   *    static distance value added to the maximum cost
   */
  void AbstractRoutingProfile::SetCostLimitDistance(double costLimitDistance)
  {
    this->costLimitDistance=costLimitDistance;
  }

  /**
   * The router tries to minimize the actual costs of the route. There is a lower limit
   * defined by GetCosts(double distance). Applying the given factor to the minimal cost
   * results in a upper limit for the costs.
   *
   * Increasing the factor results in the router trying harder to find a route by looking for
   * bigger and even bigger detours, decreasing the factor result in the router either finding a rather direct
   * route or none. Setting the factor below 1.0 should result in the router not finding any route at all.
   *
   * If there is a router the current router will find it and the router will look for the optimal route first.
   * So, if there is a route the limit could be set to std::limits<double>::max(). If there is no route though
   * the limit will stop the router to search for all possible detours, walking the whole graph in the end.
   * Since this might take for ever the limit should be reasonable high.
   *
   * The actual maximum cost limit is calculated based on a constant limit distance (default 10.0 Km)
   * and a cost factor applied to the minimum costs 8default 5.0).
   *
   * So the resulting maxium cost are profile.GetCosts(profile.GetCostLimitDistance())+
   * profile.GetCosts(distance)*profile.GetCostLimitFactor().
   *
   * @param costLimitFactor
   *    The new limit
   */
  void AbstractRoutingProfile::SetCostLimitFactor(double costLimitFactor)
  {
    this->costLimitFactor=costLimitFactor;
  }

  void AbstractRoutingProfile::ParametrizeForFoot(const TypeConfig& typeConfig,
                                                  double maxSpeed)
  {
    speeds.clear();

    SetVehicle(vehicleFoot);
    SetVehicleMaxSpeed(maxSpeed);

    for (const auto &type : typeConfig.GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanRouteFoot()) {
        AddType(type,maxSpeed);
      }
    }
  }

  void AbstractRoutingProfile::ParametrizeForBicycle(const TypeConfig& typeConfig,
                                                     double maxSpeed)
  {
    speeds.clear();

    SetVehicle(vehicleBicycle);
    SetVehicleMaxSpeed(maxSpeed);

    for (const auto &type : typeConfig.GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanRouteBicycle()) {
        AddType(type,maxSpeed);
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

    for (const auto &type : typeConfig.GetTypes()) {
      if (!type->GetIgnore() &&
          type->CanRouteCar()) {
        std::map<std::string,double>::const_iterator speed=speedMap.find(type->GetName());

        if (speed==speedMap.end()) {
          log.Error() << "No speed for type '" << type->GetName() << "' defined!";
          everythingResolved=false;

          continue;
        }

        AddType(type,speed->second);
      }
    }

    return everythingResolved;
  }

  void AbstractRoutingProfile::AddType(const TypeInfoRef& type,
                                       double speed)
  {
    if (speeds.empty()) {
      minSpeed=speed;
      maxSpeed=speed;
    }
    else {
      minSpeed=std::min(minSpeed,speed);
      maxSpeed=std::max(maxSpeed,speed);
    }

    if (type->GetIndex()>=speeds.size()) {
      speeds.resize(type->GetIndex()+1,0.0);
    }

    speeds[type->GetIndex()]=speed;
  }

  bool AbstractRoutingProfile::CanUse(const RouteNode& currentNode,
                                      const std::vector<ObjectVariantData>& objectVariantData,
                                      size_t pathIndex) const
  {
    if (!(currentNode.paths[pathIndex].flags & vehicleRouteNodeBit)) {
      return false;
    }


    size_t      index=currentNode.paths[pathIndex].objectIndex;
    TypeInfoRef type=objectVariantData[currentNode.objects[index].objectVariantIndex].type;

    size_t typeIndex=type->GetIndex();

    return typeIndex<speeds.size() && speeds[typeIndex]>0.0;
  }

  bool AbstractRoutingProfile::CanUse(const Area& area) const
  {
    if (area.rings.size()!=1) {
      return false;
    }

    size_t index=area.rings[0].GetType()->GetIndex();

    return index<speeds.size() && speeds[index]>0.0;
  }

  bool AbstractRoutingProfile::CanUse(const Way& way) const
  {
    size_t index=way.GetType()->GetIndex();

    if (index>=speeds.size() || speeds[index]<=0.0) {
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
    size_t index=way.GetType()->GetIndex();

    if (index>=speeds.size() || speeds[index]<=0.0) {
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
    size_t index=way.GetType()->GetIndex();

    if (index>=speeds.size() || speeds[index]<=0.0) {
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
