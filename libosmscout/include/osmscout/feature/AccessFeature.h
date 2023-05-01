#ifndef OSMSCOUT_FEATURE_ACCESS_FEATURE_H
#define OSMSCOUT_FEATURE_ACCESS_FEATURE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeature.h>

#include <osmscout/FeatureReader.h>

namespace osmscout {

  class OSMSCOUT_API AccessFeatureValue : public FeatureValue
  {
  public:
    enum Access : uint8_t {
      footForward     = 1u << 0u,
      footBackward    = 1u << 1u,
      bicycleForward  = 1u << 2u,
      bicycleBackward = 1u << 3u,
      carForward      = 1u << 4u,
      carBackward     = 1u << 5u,
      onewayForward   = 1u << 6u,
      onewayBackward  = 1u << 7u
    };

  private:
    uint8_t access=0;

  public:
    AccessFeatureValue() = default;
    AccessFeatureValue(const AccessFeatureValue& other) = default;

    explicit AccessFeatureValue(uint8_t access)
      : access(access)
    {
      // no code
    }

    void SetAccess(uint8_t access)
    {
      this->access=access;
    }

    uint8_t GetAccess() const
    {
      return access;
    }

    bool CanRoute() const
    {
      return (access & (footForward|footBackward|bicycleForward|bicycleBackward|carForward|carBackward))!=0;
    }

    bool CanRoute(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return (access & (footForward|footBackward))!=0;
      case vehicleBicycle:
        return (access & (bicycleForward|bicycleBackward))!=0;
      case vehicleCar:
        return (access & (carForward|carBackward))!=0;
      }

      return false;
    }

    bool CanRoute(VehicleMask vehicleMask) const
    {
      if ((vehicleMask & vehicleFoot)!=0 &&
          (access & (footForward|footBackward))!=0) {
        return true;
      }

      if ((vehicleMask & vehicleBicycle)!=0 &&
          (access & (bicycleForward|bicycleBackward))!=0) {
        return true;
      }

      if ((vehicleMask & vehicleCar)!=0 &&
          (access & (carForward|carBackward))!=0) {
        return true;
      }

      return false;
    }

    bool CanRouteForward() const
    {
      return (access & (footForward|bicycleForward|carForward))!=0;
    }

    bool CanRouteForward(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return (access & footForward)!=0;
      case vehicleBicycle:
        return (access & bicycleForward)!=0;
      case vehicleCar:
        return (access & carForward)!=0;
      }

      return false;
    }

    bool CanRouteBackward() const
    {
      return (access & (footBackward|bicycleBackward|carBackward))!=0;
    }

    bool CanRouteBackward(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return (access & footBackward)!=0;
      case vehicleBicycle:
        return (access & bicycleBackward)!=0;
      case vehicleCar:
        return (access & carBackward)!=0;
      }

      return false;
    }

    bool CanRouteFoot() const
    {
      return (access & footForward)!=0 ||
             (access & footBackward)!=0;
    }

    bool CanRouteFootForward() const
    {
      return (access & footForward)!=0;
    }

    bool CanRouteFootBackward() const
    {
      return (access & footBackward)!=0;
    }

    bool CanRouteBicycle() const
    {
      return (access & bicycleForward)!=0 ||
             (access & bicycleBackward)!=0;
    }

    bool CanRouteBicycleForward() const
    {
      return (access & bicycleForward)!=0;
    }

    bool CanRouteBicycleBackward() const
    {
      return (access & bicycleBackward)!=0;
    }

    bool CanRouteCar() const
    {
      return (access & carForward)!=0 ||
             (access & carBackward)!=0;
    }

    bool CanRouteCarForward() const
    {
      return (access & carForward)!=0;
    }

    bool CanRouteCarBackward() const
    {
      return (access & carBackward)!=0;
    }

    bool IsOneway() const
    {
      return (access & (onewayForward|onewayBackward))!=0;
    }

    bool IsOnewayForward() const
    {
      return (access & onewayForward)!=0;
    }

    bool IsOnewayBackward() const
    {
      return (access & onewayBackward)!=0;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    AccessFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API AccessFeature : public Feature
  {
  private:
    TagId tagOneway;
    TagId tagJunction;

    TagId tagAccess;
    TagId tagAccessForward;
    TagId tagAccessBackward;

    TagId tagFoot;
    TagId tagFootForward;
    TagId tagFootBackward;

    TagId tagBicycle;
    TagId tagBicycleForward;
    TagId tagBicycleBackward;

    TagId tagMotorVehicle;
    TagId tagMotorVehicleForward;
    TagId tagMotorVehicleBackward;

    TagId tagMotorcar;
    TagId tagMotorcarForward;
    TagId tagMotorcarBackward;

  public:
    /** Name of this feature */
    static const char* const NAME;

  private:
    void ParseAccessFlag(const std::string_view& value,
                         uint8_t& access,
                         uint8_t bit) const
    {
      access&=uint8_t(~bit);

      if (!(value=="no")) {
        access|=bit;
      }
    }

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueAlignment() const override;
    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  using AccessFeatureValueReader = FeatureValueReader<AccessFeature, AccessFeatureValue>;
}

#endif
