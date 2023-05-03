#ifndef OSMSCOUT_FEATURE_ACCESS_RESTRICTED_FEATURE_H
#define OSMSCOUT_FEATURE_ACCESS_RESTRICTED_FEATURE_H

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

  class OSMSCOUT_API AccessRestrictedFeatureValue : public FeatureValue
  {
  public:
    enum Access : uint8_t {
      foot     = 1u << 0u,
      bicycle  = 1u << 1u,
      car      = 1u << 2u,
    };

  private:
    uint8_t access=0;

  public:
    AccessRestrictedFeatureValue() = default;

    explicit AccessRestrictedFeatureValue(uint8_t access)
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

    bool CanAccess() const
    {
      return (access & (foot|bicycle|car))!=0;
    }

    bool CanAccess(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return (access &foot)!=0;
      case vehicleBicycle:
        return (access & bicycle)!=0;
      case vehicleCar:
        return (access & car)!=0;
      }

      return false;
    }

    bool CanAccess(VehicleMask vehicleMask) const
    {
      if ((vehicleMask & vehicleFoot)!=0 &&
          (access & foot)!=0) {
        return true;
      }

      if ((vehicleMask & vehicleBicycle)!=0 &&
          (access & bicycle)!=0) {
        return true;
      }

      return (vehicleMask & vehicleCar)!=0 &&
             (access & car)!=0;
    }

    bool CanAccessFoot() const
    {
      return (access & foot)!=0;
    }

    bool CanAccessBicycle() const
    {
      return (access & bicycle)!=0;
    }

    bool CanAccessCar() const
    {
      return (access & car)!=0;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    AccessRestrictedFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  /**
   * AccessRestriction signals, if there is some access restriction for a given way and a given vehicle.
   *
   * An access restriction means, that a way can be used for a vehicle, but access ist restricted.
   * Restricted access means, that you can enter a restricted region, but cannot leave it again for a given
   * route. You may only enter the restricted region if you have a certain intention.
   *
   * No access restriction, does not mean that a way can be used for a given vehicle. You must still evaluate if
   * there is access at all for the vehicle.
   */
  class OSMSCOUT_API AccessRestrictedFeature : public Feature
  {
  private:
    TagId tagAccess;
    TagId tagFoot;
    TagId tagBicycle;
    TagId tagMotorVehicle;

  public:
    /** Name of this feature */
    static const char* const NAME;

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

  using AccessRestrictedFeatureReader      = FeatureReader<AccessRestrictedFeature>;
  using AccessRestrictedFeatureValueReader = FeatureValueReader<AccessRestrictedFeature, AccessRestrictedFeatureValue>;
}

#endif
