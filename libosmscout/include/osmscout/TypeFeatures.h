#ifndef OSMSCOUT_FEATURES_H
#define OSMSCOUT_FEATURES_H

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

#include <cstdint>
#include <limits>
#include <unordered_map>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeature.h>

#include <osmscout/util/Color.h>

namespace osmscout {

  class OSMSCOUT_API NameFeatureValue : public FeatureValue
  {
  private:
    std::string name;

  public:
    NameFeatureValue() = default;
    NameFeatureValue(const NameFeatureValue& featureValue) = default;

    inline explicit NameFeatureValue(const std::string& name)
    : name(name)
    {
      // no code
    }

    inline void SetName(const std::string& name)
    {
      this->name=name;
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return name;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    NameFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API NameFeature : public Feature
  {
  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    NameFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API NameAltFeatureValue : public FeatureValue
  {
  private:
    std::string nameAlt;

  public:
    NameAltFeatureValue() = default;
    NameAltFeatureValue(const NameAltFeatureValue& featureValue) = default;

    inline explicit NameAltFeatureValue(const std::string& nameAlt)
    : nameAlt(nameAlt)
    {
      // no code
    }

    inline void SetNameAlt(const std::string& nameAlt)
    {
      this->nameAlt=nameAlt;
    }

    inline std::string GetNameAlt() const
    {
      return nameAlt;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return nameAlt;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    NameAltFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API NameAltFeature : public Feature
  {
  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    NameAltFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API NameShortFeatureValue : public FeatureValue
  {
  private:
    std::string nameShort;

  public:
    inline NameShortFeatureValue() = default;

    inline explicit NameShortFeatureValue(const std::string& nameShort)
    : nameShort(nameShort)
    {
      // no code
    }

    inline void SetNameShort(const std::string& nameShort)
    {
      this->nameShort=nameShort;
    }

    inline std::string GetNameShort() const
    {
      return nameShort;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return nameShort;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    NameShortFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API NameShortFeature : public Feature
  {
  private:
      TagId tagShortName;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    NameShortFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API RefFeatureValue : public FeatureValue
  {
  private:
    std::string ref;

  public:
    RefFeatureValue() = default;
    RefFeatureValue(const RefFeatureValue& featureValue) = default;

    inline explicit RefFeatureValue(const std::string& ref)
    : ref(ref)
    {
      // no code
    }

    inline void SetRef(const std::string& ref)
    {
      this->ref=ref;
    }

    inline std::string GetRef() const
    {
      return ref;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return ref;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    RefFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API RefFeature : public Feature
  {
  private:
    TagId tagRef;

  public:
    /** Name of this feature */
    static const char* const NAME;
    /** Name of this feature */
    static const char* const NAME_LABEL;
    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    RefFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API LocationFeatureValue : public FeatureValue
  {
  private:
    std::string location;

  public:
    LocationFeatureValue() = default;
    LocationFeatureValue(const LocationFeatureValue& featureValue) = default;

    inline explicit LocationFeatureValue(const std::string& location)
    : location(location)
    {
      // no code
    }

    inline void SetLocation(const std::string& location)
    {
      this->location=location;
    }

    inline std::string GetLocation() const
    {
      return location;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return location;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    LocationFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  /**
   * The location feature stores the location of an (normally) node or area. Even the data is not stored
   * the location feature checks that a street or place and an house number is stored on the object.
   *
   * So in effect it stores the location part of objects that have an address.
   */
  class OSMSCOUT_API LocationFeature : public Feature
  {
  private:
    TagId tagAddrStreet;
    TagId tagAddrHouseNr;
    TagId tagAddrPlace;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  /**
   * The address feature stores the house number of an (normally) node or area. Even the data is not stored
   * the address feature checks that a street or place and an house number is stored on the object.
   *
   * So in effect it stores the house number part of objects that have an address.
   */
  class OSMSCOUT_API AddressFeatureValue : public FeatureValue
  {
  private:
    std::string address;

  public:
    AddressFeatureValue() = default;
    AddressFeatureValue(const AddressFeatureValue& featureValue) = default;

    inline explicit AddressFeatureValue(const std::string& address)
    : address(address)
    {
      // no code
    }

    inline void SetAddress(const std::string& address)
    {
      this->address=address;
    }

    inline std::string GetAddress() const
    {
      return address;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return address;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    AddressFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API AddressFeature : public Feature
  {
  private:
    TagId tagAddrHouseNr;
    TagId tagAddrStreet;
    TagId tagAddrPlace;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    AddressFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

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

    inline explicit AccessFeatureValue(uint8_t access)
    : access(access)
    {
      // no code
    }

    inline void SetAccess(uint8_t access)
    {
      this->access=access;
    }

    inline uint8_t GetAccess() const
    {
      return access;
    }

    inline bool CanRoute() const
    {
      return (access & (footForward|footBackward|bicycleForward|bicycleBackward|carForward|carBackward))!=0;
    }

    inline bool CanRoute(Vehicle vehicle) const
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

    inline bool CanRoute(VehicleMask vehicleMask) const
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

    inline bool CanRouteForward() const
    {
      return (access & (footForward|bicycleForward|carForward))!=0;
    }

    inline bool CanRouteForward(Vehicle vehicle) const
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

    inline bool CanRouteBackward() const
    {
      return (access & (footBackward|bicycleBackward|carBackward))!=0;
    }

    inline bool CanRouteBackward(Vehicle vehicle) const
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

    inline bool CanRouteFoot() const
    {
      return (access & footForward)!=0 ||
             (access & footBackward)!=0;
    }

    inline bool CanRouteFootForward() const
    {
      return (access & footForward)!=0;
    }

    inline bool CanRouteFootBackward() const
    {
      return (access & footBackward)!=0;
    }

    inline bool CanRouteBicycle() const
    {
      return (access & bicycleForward)!=0 ||
             (access & bicycleBackward)!=0;
    }

    inline bool CanRouteBicycleForward() const
    {
      return (access & bicycleForward)!=0;
    }

    inline bool CanRouteBicycleBackward() const
    {
      return (access & bicycleBackward)!=0;
    }

    inline bool CanRouteCar() const
    {
      return (access & carForward)!=0 ||
             (access & carBackward)!=0;
    }

    inline bool CanRouteCarForward() const
    {
      return (access & carForward)!=0;
    }

    inline bool CanRouteCarBackward() const
    {
      return (access & carBackward)!=0;
    }

    inline bool IsOneway() const
    {
      return (access & (onewayForward|onewayBackward))!=0;
    }

    inline bool IsOnewayForward() const
    {
      return (access & onewayForward)!=0;
    }

    inline bool IsOnewayBackward() const
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
    inline void ParseAccessFlag(const std::string& value,
                                uint8_t& access,
                                uint8_t bit) const
    {
      access&=~bit;

      if (!(value=="no")) {
        access|=bit;
      }
    }

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

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

    inline explicit AccessRestrictedFeatureValue(uint8_t access)
    : access(access)
    {
      // no code
    }

    inline void SetAccess(uint8_t access)
    {
      this->access=access;
    }

    inline uint8_t GetAccess() const
    {
      return access;
    }

    inline bool CanAccess() const
    {
      return (access & (foot|bicycle|car))!=0;
    }

    inline bool CanAccess(Vehicle vehicle) const
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

    inline bool CanAccess(VehicleMask vehicleMask) const
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

    inline bool CanAccessFoot() const
    {
      return (access & foot)!=0;
    }

    inline bool CanAccessBicycle() const
    {
      return (access & bicycle)!=0;
    }

    inline bool CanAccessCar() const
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

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API LayerFeatureValue : public FeatureValue
  {
  private:
    int8_t layer=0;

  public:
    LayerFeatureValue() = default;

    inline explicit LayerFeatureValue(int8_t layer)
    : layer(layer)
    {
      // no code
    }

    inline void SetLayer(int8_t layer)
    {
      this->layer=layer;
    }

    inline int8_t GetLayer() const
    {
      return layer;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    LayerFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API LayerFeature : public Feature
  {
  private:
    TagId tagLayer;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API WidthFeatureValue : public FeatureValue
  {
  private:
    uint8_t width=0;

  public:
    WidthFeatureValue() = default;

    inline explicit WidthFeatureValue(uint8_t width)
    : width(width)
    {
      // no code
    }

    inline void SetWidth(uint8_t width)
    {
      this->width=width;
    }

    inline uint8_t GetWidth() const
    {
      return width;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    WidthFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API WidthFeature : public Feature
  {
  private:
    TagId tagWidth;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API MaxSpeedFeatureValue : public FeatureValue
  {
  private:
    uint8_t maxSpeed=0;

  public:
    MaxSpeedFeatureValue() = default;

    inline explicit MaxSpeedFeatureValue(uint8_t maxSpeed)
    : maxSpeed(maxSpeed)
    {
      // no code
    }

    inline void SetMaxSpeed(uint8_t maxSpeed)
    {
      this->maxSpeed=maxSpeed;
    }

    inline uint8_t GetMaxSpeed() const
    {
      return maxSpeed;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    MaxSpeedFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API MaxSpeedFeature : public Feature
  {
  private:
    TagId tagMaxSpeed;
    TagId tagMaxSpeedForward;
    TagId tagMaxSpeedBackward;

  public:
    /** Name of this feature */
    static const char* const NAME;

  private:
    bool GetTagValue(TagErrorReporter& errorReporter,
                     const TagRegistry& tagRegistry,
                     const ObjectOSMRef& object,
                     const TagMap& tags,
                     const std::string& input,
                     uint8_t& speed) const;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API GradeFeatureValue : public FeatureValue
  {
  private:
    uint8_t grade=0;

  public:
    GradeFeatureValue() = default;

    inline explicit GradeFeatureValue(uint8_t grade)
    : grade(grade)
    {
      // no code
    }

    inline void SetGrade(uint8_t grade)
    {
      this->grade=grade;
    }

    inline uint8_t GetGrade() const
    {
      return grade;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    GradeFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API GradeFeature : public Feature
  {
  private:
    TagId tagSurface;
    TagId tagTrackType;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API AdminLevelFeatureValue : public FeatureValue
  {
  private:
    uint8_t     adminLevel=0;
    std::string isIn;

  public:
    AdminLevelFeatureValue() = default;

    inline AdminLevelFeatureValue(uint8_t adminLevel,
                                  const std::string& isIn)
    : adminLevel(adminLevel),
      isIn(isIn)
    {
      // no code
    }

    inline void SetAdminLevel(uint8_t adminLevel)
    {
      this->adminLevel=adminLevel;
    }

    inline void SetIsIn(const std::string& isIn)
    {
      this->isIn=isIn;
    }

    inline uint8_t GetAdminLevel() const
    {
      return adminLevel;
    }

    inline std::string GetIsIn() const
    {
      return isIn;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    AdminLevelFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API AdminLevelFeature : public Feature
  {
  private:
    TagId tagAdminLevel;
    TagId tagIsIn;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API PostalCodeFeatureValue : public FeatureValue
  {
  private:
    std::string postalCode;

  public:
    PostalCodeFeatureValue() = default;

    inline explicit PostalCodeFeatureValue(const std::string& postalCode)
    : postalCode(postalCode)
    {
      // no code
    }

    inline void SetPostalCode(const std::string& postalCode)
    {
      this->postalCode=postalCode;
    }

    inline std::string GetPostalCode() const
    {
      return postalCode;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return postalCode;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    PostalCodeFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API PostalCodeFeature : public Feature
  {
  private:
    TagId tagPostalCode;
    TagId tagAddrPostCode;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    PostalCodeFeature();

    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API BridgeFeature : public Feature
  {
  private:
    TagId tagBridge;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API TunnelFeature : public Feature
  {
  private:
    TagId tagTunnel;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API EmbankmentFeature : public Feature
  {
    private:
      TagId tagEmbankment;

    public:
        /** Name of this feature */
        static const char* const NAME;

    public:
        void Initialize(TagRegistry& tagRegistry) override;

        std::string GetName() const override;

        void Parse(TagErrorReporter& reporter,
                   const TagRegistry& tagRegistry,
                   const FeatureInstance& feature,
                   const ObjectOSMRef& object,
                   const TagMap& tags,
                   FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API RoundaboutFeature : public Feature
  {
  private:
    TagId tagJunction;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API EleFeatureValue : public FeatureValue
  {
  private:
    uint32_t ele=0;

  public:
    EleFeatureValue() = default;

    inline explicit EleFeatureValue(uint32_t ele)
    : ele(ele)
    {
      // no code
    }

    inline void SetEle(uint32_t ele)
    {
      this->ele=ele;
    }

    inline uint32_t GetEle() const
    {
      return ele;
    }

    std::string GetLabel(const Locale &locale, size_t labelIndex) const override;

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    EleFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API EleFeature : public Feature
  {
  private:
    TagId tagEle;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "inMeter" label */
    static const char* const IN_METER_LABEL;

    /** Index of the 'inMeter' label */
    static const size_t      IN_METER_LABEL_INDEX;

    /** Name of the "inFeet" label */
    static const char* const IN_FEET_LABEL;

    /** Index of the 'inFeet' label */
    static const size_t      IN_FEET_LABEL_INDEX;

    /** Name of the "inLocaleUnit" label */
    static const char* const IN_LOCALE_UNIT_LABEL;

    /** Index of the 'inLocaleUnit' label */
    static const size_t      IN_LOCALE_UNIT_LABEL_INDEX;

  public:
    EleFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API DestinationFeatureValue : public FeatureValue
  {
  private:
    std::string destination;

  public:
    DestinationFeatureValue() = default;

    inline explicit DestinationFeatureValue(const std::string& destination)
    : destination(destination)
    {
      // no code
    }

    inline void SetDestination(const std::string& destination)
    {
      this->destination=destination;
    }

    inline std::string GetDestination() const
    {
      return destination;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return destination;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    DestinationFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API DestinationFeature : public Feature
  {
  private:
    TagId tagDestination;
    TagId tagDestinationRef;
    TagId tagDestinationForward;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    DestinationFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API BuildingFeature : public Feature
  {
  private:
    TagId tagBuilding;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API WebsiteFeatureValue : public FeatureValue
  {
  private:
    std::string website;

  public:
    WebsiteFeatureValue() = default;

    inline explicit WebsiteFeatureValue(const std::string& website)
    : website(website)
    {
      // no code
    }

    inline void SetWebsite(const std::string& website)
    {
      this->website=website;
    }

    inline std::string GetWebsite() const
    {
      return website;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return website;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    WebsiteFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API WebsiteFeature : public Feature
  {
  private:
    TagId tagWebsite;
    TagId tagContactWebsite;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "url" label */
    static const char* const URL_LABEL;

    /** Index of the 'url' label */
    static const size_t      URL_LABEL_INDEX;

  public:
    WebsiteFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };


  class OSMSCOUT_API PhoneFeatureValue : public FeatureValue
  {
  private:
    std::string phone;

  public:
    PhoneFeatureValue() = default;

    inline explicit PhoneFeatureValue(const std::string& phone)
    : phone(phone)
    {
      // no code
    }

    inline void SetPhone(const std::string& phone)
    {
      this->phone=phone;
    }

    inline std::string GetPhone() const
    {
      return phone;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return phone;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    PhoneFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API PhoneFeature : public Feature
  {
  private:
    TagId tagPhone;
    TagId tagContactPhone;
    TagId tagContactMobile;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "number" label */
    static const char* const NUMBER_LABEL;

    /** Index of the 'number' label */
    static const size_t      NUMBER_LABEL_INDEX;

  public:
    PhoneFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API IsInFeatureValue : public FeatureValue
  {
  private:
    std::string isIn;

  public:
    IsInFeatureValue() = default;

    inline explicit IsInFeatureValue(const std::string& isIn)
      : isIn(isIn)
    {
      // no code
    }

    inline void SetIsIn(const std::string& isIn)
    {
      this->isIn=isIn;
    }

    inline std::string GetIsIn() const
    {
      return isIn;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    IsInFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API IsInFeature : public Feature
  {
  private:
    TagId tagIsIn;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API ConstructionYearFeatureValue : public FeatureValue
  {
  private:
    int startYear=0;
    int endYear=0;

  public:
    ConstructionYearFeatureValue() = default;

    inline ConstructionYearFeatureValue(int startYear, int endYear)
      : startYear(startYear),
        endYear(endYear)
    {
      // no code
    }

    inline void SetStartYear(int year)
    {
      this->startYear=year;
    }

    inline int GetStartYear() const
    {
      return startYear;
    }

    inline void SetEndYear(int year)
    {
      this->endYear=year;
    }

    inline int GetEndYear() const
    {
      return endYear;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      if (startYear==endYear) {
        return std::to_string(startYear);
      }

      return std::to_string(startYear)+"-"+std::to_string(endYear);
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    ConstructionYearFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API ConstructionYearFeature : public Feature
  {
  private:
    TagId tagConstructionYear;
    TagId tagStartDate;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "year" label */
    static const char* const YEAR_LABEL;

    /** Index of the 'year' label */
    static const size_t      YEAR_LABEL_INDEX;

  public:
    ConstructionYearFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API SidewayFeatureValue : public FeatureValue
  {
  public:
    enum Feature : uint8_t {
      sidewalkTrackLeft  = 1u << 0u,
      sidewalkTrackRight = 1u << 1u,
      cyclewayLaneLeft   = 1u << 2u,
      cyclewayLaneRight  = 1u << 3u,
      cyclewayTrackLeft  = 1u << 4u,
      cyclewayTrackRight = 1u << 5u,
    };

  private:
    uint8_t featureSet=0;

  public:
    SidewayFeatureValue() = default;

    inline bool IsFlagSet(size_t flagIndex) const override
    {
      return (featureSet & (1<< flagIndex))!=0;
    }

    inline void SetFeatureSet(uint8_t featureSet)
    {
      this->featureSet=featureSet;
    }

    inline bool HasSidewalkTrackLeft() const
    {
      return (featureSet & sidewalkTrackLeft)!=0;
    }

    inline bool HasSidewalkTrackRight() const
    {
      return (featureSet & sidewalkTrackRight)!=0;
    }

    inline bool HasCyclewayLaneLeft() const
    {
      return (featureSet & cyclewayLaneLeft)!=0;
    }

    inline bool HasCyclewayLaneRight() const
    {
      return (featureSet & cyclewayLaneRight)!=0;
    }

    inline bool HasCyclewayTrackLeft() const
    {
      return (featureSet & cyclewayTrackLeft)!=0;
    }

    inline bool HasCyclewayTrackRight() const
    {
      return (featureSet & cyclewayTrackRight)!=0;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    SidewayFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API SidewayFeature : public Feature
  {
  private:
    enum class FeatureFlags: uint8_t {
      sidewalkTrackLeft  = 0,
      sidewalkTrackRight = 1,
      cyclewayLaneLeft   = 2,
      cyclewayLaneRight  = 3,
      cyclewayTrackLeft  = 4,
      cyclewayTrackRight = 5
    };

  private:
    TagId tagSidewalk;
    TagId tagCyclewayLeft;
    TagId tagCyclewayLeftSegregated;
    TagId tagCyclewayRight;
    TagId tagCyclewayRightSegregated;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    SidewayFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API LanesFeatureValue : public FeatureValue
  {
  private:

    uint8_t     lanes=0;              //!< First two bits reserved, 3 bit for number of lanes in each direction
    std::string turnForward;
    std::string turnBackward;
    std::string destinationForward;
    std::string destinationBackward;

  public:
    LanesFeatureValue() = default;

    inline explicit LanesFeatureValue(uint8_t lanes)
      : lanes(lanes)
    {
      // no code
    }

    inline void SetLanes(uint8_t forwardLanes, uint8_t backwardLanes)
    {
      this->lanes=((forwardLanes & (uint8_t)0x7) << 2) |
                  ((backwardLanes & (uint8_t)0x7) << 5);
    }

    inline bool HasSingleLane() const
    {
      return GetLanes()==0;
    }

    inline uint8_t GetForwardLanes() const
    {
      return (lanes >> 2) & (uint8_t)0x07;
    }

    inline uint8_t GetBackwardLanes() const
    {
      return (lanes >> 5) & (uint8_t)0x07;
    }

    uint8_t GetLanes() const;

    inline void SetTurnLanes(const std::string& turnForward,
                             const std::string& turnBawckard)
    {
      this->turnForward=turnForward;
      this->turnBackward=turnBawckard;
    }

    inline std::string GetTurnForward() const
    {
      return turnForward;
    }

    inline std::string GetTurnBackward() const
    {
      return turnBackward;
    }

    inline std::string GetDestinationForward() const
    {
      return destinationForward;
    }

    inline std::string GetDestinationBackward() const
    {
      return destinationBackward;
    }

    inline void SetDestinationLanes(const std::string& destinationForward,
                                    const std::string& destinationBawckard)
    {
      this->destinationForward=destinationForward;
      this->destinationBackward=destinationBawckard;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      if (HasSingleLane()) {
        return "1";
      }

      return std::to_string(GetForwardLanes()) + " " + std::to_string(GetBackwardLanes());
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    LanesFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API LanesFeature : public Feature
  {
  private:
    TagId tagOneway;
    TagId tagLanes;
    TagId tagLanesForward;
    TagId tagLanesBackward;
    TagId tagTurnLanes;
    TagId tagTurnLanesForward;
    TagId tagTurnLanesBackward;
    TagId tagDestinationLanes;
    TagId tagDestinationLanesForward;
    TagId tagDestinationLanesBackward;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    LanesFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API OperatorFeatureValue : public FeatureValue
  {
  private:
    std::string op;

  public:
    OperatorFeatureValue() = default;
    OperatorFeatureValue(const OperatorFeatureValue& featureValue) = default;

    inline explicit OperatorFeatureValue(const std::string& op)
      : op(op)
    {
      // no code
    }

    inline void SetOperator(const std::string& op)
    {
      this->op=op;
    }

    inline std::string GetOperator() const
    {
      return op;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return op;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    OperatorFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API OperatorFeature : public Feature
  {
  private:
    TagId tagOperator;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "number" label */
    static const char* const NUMBER_LABEL;

    /** Index of the 'number' label */
    static const size_t      NUMBER_LABEL_INDEX;

  public:
    OperatorFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API NetworkFeatureValue : public FeatureValue
  {
  private:
    std::string network;

  public:
    NetworkFeatureValue() = default;
    NetworkFeatureValue(const NetworkFeatureValue& featureValue) = default;

    inline explicit NetworkFeatureValue(const std::string& network)
      : network(network)
    {
      // no code
    }

    inline void SetNetwork(const std::string& network)
    {
      this->network=network;
    }

    inline std::string GetNetwork() const
    {
      return network;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return network;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    NetworkFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API NetworkFeature : public Feature
  {
  private:
    TagId tagNetwork;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "number" label */
    static const char* const NUMBER_LABEL;

    /** Index of the 'number' label */
    static const size_t      NUMBER_LABEL_INDEX;

  public:
    NetworkFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API FromToFeatureValue : public FeatureValue
  {
  private:
    std::string from;
    std::string to;

  public:
    FromToFeatureValue() = default;
    FromToFeatureValue(const FromToFeatureValue& featureValue) = default;

    inline explicit FromToFeatureValue(const std::string& from,
                                       const std::string& to)
      : from(from),
        to(to)
    {
      // no code
    }

    const std::string& GetFrom() const
    {
      return from;
    }

    void SetFrom(const std::string& from)
    {
      FromToFeatureValue::from=from;
    }

    const std::string& GetTo() const
    {
      return to;
    }

    void SetTo(const std::string& to)
    {
      FromToFeatureValue::to=to;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      if (!from.empty() && ! to.empty()) {
        return from + " => " + to;
      }

      if (!from.empty())  {
        return from + "=>";
      }

      if (!to.empty())  {
        return "=> " + to;
      }

      return "";
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    FromToFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API FromToFeature : public Feature
  {
  private:
    TagId tagFrom;
    TagId tagTo;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "number" label */
    static const char* const NUMBER_LABEL;

    /** Index of the 'number' label */
    static const size_t      NUMBER_LABEL_INDEX;

  public:
    FromToFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  class OSMSCOUT_API ColorFeatureValue : public FeatureValue
  {
  private:
    Color color;

  public:
    ColorFeatureValue() = default;
    ColorFeatureValue(const ColorFeatureValue& featureValue) = default;

    inline explicit ColorFeatureValue(const Color& color)
      : color(color)
    {
      // no code
    }

    Color GetColor() const
    {
      return color;
    }

    void SetColor(const Color& color)
    {
      ColorFeatureValue::color=color;
    }

    inline std::string GetLabel(const Locale &/*locale*/, size_t /*labelIndex*/) const override
    {
      return color.ToHexString();
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    ColorFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API ColorFeature : public Feature
  {
  private:
    TagId tagColor;
    TagId tagSymbol;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "number" label */
    static const char* const NUMBER_LABEL;

    /** Index of the 'number' label */
    static const size_t      NUMBER_LABEL_INDEX;

  public:
    ColorFeature();
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };
}

#endif
