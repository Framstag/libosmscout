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

#include <map>
#include <memory>
#include <vector>
#include <iostream>

#include <osmscout/OSMScoutTypes.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/FeatureReader.h>

#include <osmscout/Way.h>
#include <osmscout/Area.h>

#include <osmscout/util/Time.h>
#include <osmscout/util/String.h>
#include <osmscout/util/Logger.h>

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
    virtual Distance GetCostLimitDistance() const = 0;
    virtual double GetCostLimitFactor() const = 0;

    virtual bool CanUse(const RouteNode& currentNode,
                        const std::vector<ObjectVariantData>& objectVariantData,
                        size_t pathIndex) const = 0;
    virtual bool CanUse(const Area& area) const = 0;
    virtual bool CanUse(const Way& way) const = 0;
    virtual bool CanUseForward(const Way& way) const = 0;
    virtual bool CanUseBackward(const Way& way) const = 0;

    /**
     * Estimated cost for outgoing path (outPathIndex) from currentNode
     * when currentNode is entered from inPathIndex
     */
    virtual double GetCosts(const RouteNode& currentNode,
                            const std::vector<ObjectVariantData>& objectVariantData,
                            size_t inPathIndex,
                            size_t outPathIndex) const = 0;

    /**
     * Estimated cost for specific area with given distance
     */
    virtual double GetCosts(const Area& area,
                            const Distance &distance) const = 0;

    /**
     * Estimated cost for specific way with given distance
     */
    virtual double GetCosts(const Way& way,
                            const Distance &distance) const = 0;

    /**
     * Estimated cost for distance when are no limitations (max. speed on the way)
     */
    virtual double GetCosts(const Distance &distance) const = 0;

    /**
     * Textual representation of cost
     */
    virtual std::string GetCostString(double cost) const = 0;

    virtual Duration GetTime(const Area& area,
                             const Distance &distance) const = 0;
    virtual Duration GetTime(const Way& way,
                             const Distance &distance) const = 0;
  };

  using RoutingProfileRef = std::shared_ptr<RoutingProfile>;

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
    Distance                   costLimitDistance;
    double                     costLimitFactor;
    std::vector<double>        speeds;
    double                     minSpeed;
    double                     maxSpeed;
    double                     vehicleMaxSpeed;

  public:
    explicit AbstractRoutingProfile(const TypeConfigRef& typeConfig);

    void SetVehicle(Vehicle vehicle);
    void SetVehicleMaxSpeed(double maxSpeed);

    virtual void ParametrizeForFoot(const TypeConfig& typeConfig,
                                    double maxSpeed);
    virtual void ParametrizeForBicycle(const TypeConfig& typeConfig,
                                    double maxSpeed);
    virtual bool ParametrizeForCar(const TypeConfig& typeConfig,
                                   const std::map<std::string,double>& speedMap,
                                   double maxSpeed);

    inline Vehicle GetVehicle() const override
    {
      return vehicle;
    }

    inline double GetVehicleMaxSpeed() const
    {
      return vehicleMaxSpeed;
    }

    void SetCostLimitDistance(const Distance &costLimitDistance);

    inline Distance GetCostLimitDistance() const override
    {
      return costLimitDistance;
    }

    void SetCostLimitFactor(double costLimitFactor);

    inline double GetCostLimitFactor() const override
    {
      return costLimitFactor;
    }

    std::string GetCostString(double cost) const override
    {
      return std::to_string(cost);
    }

    void AddType(const TypeInfoRef& type, double speed);

    bool CanUse(const RouteNode& currentNode,
                const std::vector<ObjectVariantData>& objectVariantData,
                size_t pathIndex) const override;
    bool CanUse(const Area& area) const override;
    bool CanUse(const Way& way) const override;
    bool CanUseForward(const Way& way) const override;
    bool CanUseBackward(const Way& way) const override;

    inline Duration GetTime(const Area& area,
                            const Distance &distance) const override
    {
      double speed=speeds[area.GetType()->GetIndex()];

      speed=std::min(vehicleMaxSpeed,speed);

      return DurationOfHours(distance.As<Kilometer>()/speed);
    }

    inline Duration GetTime(const Way& way,
                            const Distance &distance) const override
    {
      double speed;

      MaxSpeedFeatureValue *maxSpeedValue=maxSpeedReader.GetValue(way.GetFeatureValueBuffer());

      if (maxSpeedValue!=nullptr &&
          maxSpeedValue->GetMaxSpeed()>0) {
        speed=maxSpeedValue->GetMaxSpeed();
      }
      else {
        speed=speeds[way.GetType()->GetIndex()];
      }

      speed=std::min(vehicleMaxSpeed,speed);

      return DurationOfHours(distance.As<Kilometer>()/speed);
    }
  };

  /**
   * \ingroup Routing
   * Profile that defines costs in a way that the shortest way is chosen (cost==distance).
   */
  class OSMSCOUT_API ShortestPathRoutingProfile : public AbstractRoutingProfile
  {
  public:
    explicit ShortestPathRoutingProfile(const TypeConfigRef& typeConfig);

    inline double GetCosts(const RouteNode& currentNode,
                           const std::vector<ObjectVariantData>& /*objectVariantData*/,
                           size_t /*inPathIndex*/,
                           size_t outPathIndex) const override
    {
      return currentNode.paths[outPathIndex].distance.As<Kilometer>();
    }

    inline double GetCosts(const Area& /*area*/,
                           const Distance &distance) const override
    {
      return distance.As<Kilometer>();
    }

    inline double GetCosts(const Way& /*way*/,
                           const Distance &distance) const override
    {
      return distance.As<Kilometer>();
    }

    inline double GetCosts(const Distance &distance) const override
    {
      return distance.As<Kilometer>();
    }

    std::string GetCostString(double cost) const override
    {
      return Kilometers(cost).AsString();
    }
  };

  using ShortestPathRoutingProfileRef = std::shared_ptr<ShortestPathRoutingProfile>;

  /**
   * \ingroup Routing
   * Profile that defines costs base of the time the traveling device needs
   * for a certain way resulting in the fastest path chosen (cost=distance/speedForWayType).
   */
  class OSMSCOUT_API FastestPathRoutingProfile : public AbstractRoutingProfile
  {
  protected:
    bool applyJunctionPenalty=true;
    Distance penaltySameType=Meters(160);
    Distance penaltyDifferentType=Meters(250);
    HourDuration maxPenalty=std::chrono::seconds(10);

  public:
    explicit FastestPathRoutingProfile(const TypeConfigRef& typeConfig);

    void ParametrizeForFoot(const TypeConfig& typeConfig,
                            double maxSpeed) override
    {
      applyJunctionPenalty=false;
      AbstractRoutingProfile::ParametrizeForFoot(typeConfig, maxSpeed);
    }

    /**
     * Setup profile for bicycle, it also setup junction penalty and multiply cost limit and cost limit factor.
     */
    void ParametrizeForBicycle(const TypeConfig& typeConfig,
                               double maxSpeed) override
    {
      applyJunctionPenalty = true;
      costLimitDistance *= 2;
      costLimitFactor *= 1.5;
      AbstractRoutingProfile::ParametrizeForBicycle(typeConfig, maxSpeed);
    }

    /**
     * Setup profile for car, it also setup junction penalty and multiply cost limit and cost limit factor.
     */
    bool ParametrizeForCar(const TypeConfig& typeConfig,
                           const std::map<std::string,double>& speedMap,
                           double maxSpeed) override
    {
      applyJunctionPenalty = true;
      costLimitDistance *= 2;
      costLimitFactor *= 1.5;
      return AbstractRoutingProfile::ParametrizeForCar(typeConfig, speedMap, maxSpeed);
    }

    inline double GetCosts(const RouteNode& currentNode,
                           const std::vector<ObjectVariantData>& objectVariantData,
                           size_t inPathIndex,
                           size_t outPathIndex) const override
    {
      assert(currentNode.paths.size() > inPathIndex);
      assert(currentNode.paths.size() > outPathIndex);
      auto inObjIndex=currentNode.paths[inPathIndex].objectIndex;
      auto outObjIndex=currentNode.paths[outPathIndex].objectIndex;
      auto inVariantIndex=currentNode.objects[inObjIndex].objectVariantIndex;
      auto outVariantIndex=currentNode.objects[outObjIndex].objectVariantIndex;
      assert(objectVariantData.size() > inVariantIndex);
      assert(objectVariantData.size() > outVariantIndex);
      const ObjectVariantData &inPathVariant=objectVariantData[inVariantIndex];
      const ObjectVariantData &outPathVariant=objectVariantData[outVariantIndex];

      auto GetMaxSpeed = [&](const ObjectVariantData &variant) -> double {
        if (variant.maxSpeed > 0) {
          return variant.maxSpeed;
        }
        TypeInfoRef type=variant.type;
        double speed=speeds[type->GetIndex()];
        if (speed<=0){
          log.Warn() << "Infinite cost for type " << type->GetName();
        }
        return speed;
      };

      // price of ride to target node using outPath
      double speed=std::min(vehicleMaxSpeed,GetMaxSpeed(outPathVariant));
      double outPrice = speed <= 0 ?
          std::numeric_limits<double>::infinity() :
          currentNode.paths[outPathIndex].distance.As<Kilometer>() / speed;

      // add penalty for junction
      // it is estimate without considering real junction geometry
      double junctionPenalty{0};
      if (applyJunctionPenalty && inObjIndex!=outObjIndex){
        auto penaltyDistance = inPathVariant.type != outPathVariant.type ?
                               penaltyDifferentType :
                               penaltySameType;

        double minSpeed=std::min(GetMaxSpeed(inPathVariant),GetMaxSpeed(outPathVariant));
        junctionPenalty = minSpeed <= 0 ?
                          std::numeric_limits<double>::infinity() :
                          penaltyDistance.As<Kilometer>() / minSpeed;

        junctionPenalty = std::min(junctionPenalty, maxPenalty.count());
#if defined(DEBUG_ROUTING)
        std::cout << "  Add junction penalty " << GetCostString(junctionPenalty) << std::endl;
#endif
      }

      return outPrice + junctionPenalty;
    }

    inline double GetCosts(const Area& area,
                           const Distance &distance) const override
    {
      double speed=speeds[area.GetType()->GetIndex()];

      speed=std::min(vehicleMaxSpeed,speed);

      return distance.As<Kilometer>()/speed;
    }

    inline double GetCosts(const Way& way,
                           const Distance &distance) const override
    {
      double speed;

      MaxSpeedFeatureValue *maxSpeedValue=maxSpeedReader.GetValue(way.GetFeatureValueBuffer());

      if (maxSpeedValue!=nullptr &&
          maxSpeedValue->GetMaxSpeed()>0) {
        speed=maxSpeedValue->GetMaxSpeed();
      }
      else {
        speed=speeds[way.GetType()->GetIndex()];
      }

      speed=std::min(vehicleMaxSpeed,speed);

      return distance.As<Kilometer>()/speed;
    }

    inline double GetCosts(const Distance &distance) const override
    {
      double speed=maxSpeed;

      speed=std::min(vehicleMaxSpeed,speed);

      return distance.As<Kilometer>()/speed;
    }

    std::string GetCostString(double cost) const override
    {
      return DurationString(std::chrono::duration_cast<Duration>(HourDuration(cost)));
    }

  };

  using FastestPathRoutingProfileRef = std::shared_ptr<FastestPathRoutingProfile>;
}

#endif
