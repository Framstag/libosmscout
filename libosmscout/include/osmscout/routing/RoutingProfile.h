#ifndef OSMSCOUT_ROUTINGPROFILE_H
#define OSMSCOUT_ROUTINGPROFILE_H

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

#include <memory>
#include <vector>

#include <osmscout/Types.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeatures.h>

#include <osmscout/Way.h>
#include <osmscout/Area.h>

#include <osmscout/routing/RouteNode.h>

namespace osmscout {

  /**
   * \ingroup Routing
   * Abstract interface for a routing profile. A routing profile decides about the costs
   * of taking a certain way. It thus may hold information about how fast ways can be used,
   * maximum speed of the traveling device etc...
   */
  class OSMSCOUT_API RoutingProfile
  {
  public:
    virtual ~RoutingProfile();

    virtual Vehicle GetVehicle() const = 0;
    virtual double GetCostLimitDistance() const = 0;
    virtual double GetCostLimitFactor() const = 0;

    virtual bool CanUse(const RouteNode& currentNode,
                        const std::vector<ObjectVariantData>& objectVariantData,
                        size_t pathIndex) const = 0;
    virtual bool CanUse(const Area& area) const = 0;
    virtual bool CanUse(const Way& way) const = 0;
    virtual bool CanUseForward(const Way& way) const = 0;
    virtual bool CanUseBackward(const Way& way) const = 0;

    virtual double GetCosts(const RouteNode& currentNode,
                            const std::vector<ObjectVariantData>& objectVariantData,
                            size_t pathIndex) const = 0;
    virtual double GetCosts(const Area& area,
                            double distance) const = 0;
    virtual double GetCosts(const Way& way,
                            double distance) const = 0;
    virtual double GetCosts(double distance) const = 0;

    virtual double GetTime(const Area& area,
                           double distance) const = 0;
    virtual double GetTime(const Way& way,
                           double distance) const = 0;
  };

  typedef std::shared_ptr<RoutingProfile> RoutingProfileRef;

  /**
   * \ingroup Routing
   * Common base class for our concrete profile instantiations. Offers a number of profile
   * type independent interface implementations and helper methods.
   */
  class OSMSCOUT_API AbstractRoutingProfile : public RoutingProfile
  {
  protected:
    TypeConfigRef              typeConfig;
    AccessFeatureValueReader   accessReader;
    MaxSpeedFeatureValueReader maxSpeedReader;
    Vehicle                    vehicle;
    uint8_t                    vehicleRouteNodeBit;
    double                     costLimitDistance;
    double                     costLimitFactor;
    std::vector<double>        speeds;
    double                     minSpeed;
    double                     maxSpeed;
    double                     vehicleMaxSpeed;

  public:
    AbstractRoutingProfile(const TypeConfigRef& typeConfig);

    void SetVehicle(Vehicle vehicle);
    void SetVehicleMaxSpeed(double maxSpeed);

    void ParametrizeForFoot(const TypeConfig& typeConfig,
                            double maxSpeed);
    void ParametrizeForBicycle(const TypeConfig& typeConfig,
                               double maxSpeed);
    bool ParametrizeForCar(const TypeConfig& typeConfig,
                           const std::map<std::string,double>& speedMap,
                           double maxSpeed);

    inline Vehicle GetVehicle() const
    {
      return vehicle;
    }

    void SetCostLimitDistance(double costLimitDistance);

    inline double GetCostLimitDistance() const
    {
      return costLimitDistance;
    }

    void SetCostLimitFactor(double costLimitFactor);

    inline double GetCostLimitFactor() const
    {
      return costLimitFactor;
    }

    void AddType(const TypeInfoRef& type, double speed);

    bool CanUse(const RouteNode& currentNode,
                const std::vector<ObjectVariantData>& objectVariantData,
                size_t pathIndex) const;
    bool CanUse(const Area& area) const;
    bool CanUse(const Way& way) const;
    bool CanUseForward(const Way& way) const;
    bool CanUseBackward(const Way& way) const;

    inline double GetTime(const Area& area,
                          double distance) const
    {
      double speed=speeds[area.GetType()->GetIndex()];

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }

    inline double GetTime(const Way& way,
                          double distance) const
    {
      double speed;

      MaxSpeedFeatureValue *maxSpeedValue=maxSpeedReader.GetValue(way.GetFeatureValueBuffer());

      if (maxSpeedValue!=NULL &&
          maxSpeedValue->GetMaxSpeed()>0) {
        speed=maxSpeedValue->GetMaxSpeed();
      }
      else {
        speed=speeds[way.GetType()->GetIndex()];
      }

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }
  };

  /**
   * \ingroup Routing
   * Profile that defines costs in a way that the shortest way is chosen (cost==distance).
   */
  class OSMSCOUT_API ShortestPathRoutingProfile : public AbstractRoutingProfile
  {
  public:
    ShortestPathRoutingProfile(const TypeConfigRef& typeConfig);

    inline double GetCosts(const RouteNode& currentNode,
                           const std::vector<ObjectVariantData>& /*objectVariantData*/,
                           size_t pathIndex) const
    {
      return currentNode.paths[pathIndex].distance;
    }

    inline double GetCosts(const Area& /*area*/,
                           double distance) const
    {
      return distance;
    }

    inline double GetCosts(const Way& /*way*/,
                           double distance) const
    {
      return distance;
    }

    inline double GetCosts(double distance) const
    {
      return distance;
    }
  };

  typedef std::shared_ptr<ShortestPathRoutingProfile> ShortestPathRoutingProfileRef;

  /**
   * \ingroup Routing
   * Profile that defines costs base of the time the traveling device needs
   * for a certain way resulting in the fastest path chosen (cost=distance/speedForWayType).
   */
  class OSMSCOUT_API FastestPathRoutingProfile : public AbstractRoutingProfile
  {
  public:
    FastestPathRoutingProfile(const TypeConfigRef& typeConfig);

    inline double GetCosts(const RouteNode& currentNode,
                           const std::vector<ObjectVariantData>& objectVariantData,
                           size_t pathIndex) const
    {
      double speed;
      size_t index=currentNode.paths[pathIndex].objectIndex;

      if (objectVariantData[currentNode.objects[index].objectVariantIndex].maxSpeed>0) {
        speed=objectVariantData[currentNode.objects[index].objectVariantIndex].maxSpeed;
      }
      else {
        TypeInfoRef type=objectVariantData[currentNode.objects[index].objectVariantIndex].type;

        speed=speeds[type->GetIndex()];
      }

      speed=std::min(vehicleMaxSpeed,speed);

      return currentNode.paths[pathIndex].distance/speed;
    }

    inline double GetCosts(const Area& area,
                           double distance) const
    {
      double speed=speeds[area.GetType()->GetIndex()];

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }

    inline double GetCosts(const Way& way,
                           double distance) const
    {
      double speed;

      MaxSpeedFeatureValue *maxSpeedValue=maxSpeedReader.GetValue(way.GetFeatureValueBuffer());

      if (maxSpeedValue!=NULL &&
          maxSpeedValue->GetMaxSpeed()>0) {
        speed=maxSpeedValue->GetMaxSpeed();
      }
      else {
        speed=speeds[way.GetType()->GetIndex()];
      }

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }

    inline double GetCosts(double distance) const
    {
      double speed=maxSpeed;

      speed=std::min(vehicleMaxSpeed,speed);

      return distance/speed;
    }
  };

  typedef std::shared_ptr<FastestPathRoutingProfile> FastestPathRoutingProfileRef;
}

#endif
