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

#include <unordered_map>

#include <osmscout/TypeConfig.h>

#include <osmscout/util/String.h>

namespace osmscout {

  class OSMSCOUT_API NameFeatureValue : public FeatureValue
  {
  private:
    std::string name;

  public:
    inline NameFeatureValue()
    {
      // no code
    }

    inline NameFeatureValue(const std::string& name)
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

    inline std::string GetLabel() const
    {
      return name;
    }

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
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
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API NameAltFeatureValue : public FeatureValue
  {
  private:
    std::string nameAlt;

  public:
    inline NameAltFeatureValue()
    {
      // no code
    }

    inline NameAltFeatureValue(const std::string& nameAlt)
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

    inline std::string GetLabel() const
    {
      return nameAlt;
    }

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
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
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API RefFeatureValue : public FeatureValue
  {
  private:
    std::string ref;

  public:
    inline RefFeatureValue()
    {
      // no code
    }

    inline RefFeatureValue(const std::string& ref)
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

    inline std::string GetLabel() const
    {
      return ref;
    }

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
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
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API LocationFeatureValue : public FeatureValue
  {
  private:
    std::string location;

  public:
    inline LocationFeatureValue()
    {
      // no code
    }

    inline LocationFeatureValue(const std::string& location)
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

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
  };

  class OSMSCOUT_API LocationFeature : public Feature
  {
  private:
    TagId tagAddrStreet;
    TagId tagAddrHouseNr;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API AddressFeatureValue : public FeatureValue
  {
  private:
    std::string address;

  public:
    inline AddressFeatureValue()
    {
      // no code
    }

    inline AddressFeatureValue(const std::string& address)
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

    inline std::string GetLabel() const
    {
      return address;
    }

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
  };

  class OSMSCOUT_API AddressFeature : public Feature
  {
  private:
    TagId tagAddrHouseNr;
    TagId tagAddrStreet;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    AddressFeature();
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API AccessFeatureValue : public FeatureValue
  {
  public:
    enum Access {
      footForward     = 1 << 0,
      footBackward    = 1 << 1,
      bicycleForward  = 1 << 2,
      bicycleBackward = 1 << 3,
      carForward      = 1 << 4,
      carBackward     = 1 << 5,
      onewayForward   = 1 << 6,
      onewayBackward  = 1 << 7
    };

  private:
    uint8_t access;

  public:
    inline AccessFeatureValue()
    : access(0)
    {

    }

    inline AccessFeatureValue(uint8_t access)
    : access(access)
    {
      // no code
    }

    inline void SetAccess(uint8_t access)
    {
      this->access=access;
    }

    inline uint8_t GetAccess()
    {
      return access;
    }

    inline bool CanRoute() const
    {
      return access & (footForward|footBackward|bicycleForward|bicycleBackward|carForward|carBackward);
    }

    inline bool CanRoute(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return access & (footForward|footBackward);
      case vehicleBicycle:
        return access & (bicycleForward|bicycleBackward);
      case vehicleCar:
        return access & (carForward|carBackward);
      }

      return false;
    }

    inline bool CanRouteForward() const
    {
      return access & (footForward|bicycleForward|carForward);
    }

    inline bool CanRouteForward(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return access & footForward;
      case vehicleBicycle:
        return access & bicycleForward;
      case vehicleCar:
        return access & carForward;
      }

      return false;
    }

    inline bool CanRouteBackward() const
    {
      return access & (footBackward|bicycleBackward|carBackward);
    }

    inline bool CanRouteBackward(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return access & footBackward;
      case vehicleBicycle:
        return access & bicycleBackward;
      case vehicleCar:
        return access & carBackward;
      }

      return false;
    }

    inline bool CanRouteFoot() const
    {
      return (access & footForward) &&
             (access & footBackward);
    }

    inline bool CanRouteFootForward() const
    {
      return access & footForward;
    }

    inline bool CanRouteFootBackward() const
    {
      return access & footBackward;
    }

    inline bool CanRouteBicycle() const
    {
      return (access & bicycleForward) &&
             (access & bicycleBackward);
    }

    inline bool CanRouteBicycleForward() const
    {
      return access & bicycleForward;
    }

    inline bool CanRouteBicycleBackward() const
    {
      return access & bicycleBackward;
    }

    inline bool CanRouteCar() const
    {
      return (access & carForward) &&
             (access & carBackward);
    }

    inline bool CanRouteCarForward() const
    {
      return access & carForward;
    }

    inline bool CanRouteCarBackward() const
    {
      return access & carBackward;
    }

    inline bool IsOneway() const
    {
      return access & (onewayForward|onewayBackward);
    }

    inline bool IsOnewayForward() const
    {
      return access & onewayForward;
    }

    inline bool IsOnewayBackward() const
    {
      return access & onewayBackward;
    }

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
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
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API AccessRestrictedFeature : public Feature
  {
  private:
    TagId tagAccess;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API LayerFeatureValue : public FeatureValue
  {
  private:
    int8_t layer;

  public:
    inline LayerFeatureValue()
    : layer(0)
    {

    }

    inline LayerFeatureValue(int8_t layer)
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

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
  };

  class OSMSCOUT_API LayerFeature : public Feature
  {
  private:
    TagId tagLayer;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API WidthFeatureValue : public FeatureValue
  {
  private:
    uint8_t width;

  public:
    inline WidthFeatureValue()
    : width(0)
    {

    }

    inline WidthFeatureValue(uint8_t width)
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

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
  };

  class OSMSCOUT_API WidthFeature : public Feature
  {
  private:
    TagId tagWidth;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API MaxSpeedFeatureValue : public FeatureValue
  {
  private:
    uint8_t maxSpeed;

  public:
    inline MaxSpeedFeatureValue()
    : maxSpeed(0)
    {

    }

    inline MaxSpeedFeatureValue(uint8_t maxSpeed)
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

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
  };

  class OSMSCOUT_API MaxSpeedFeature : public Feature
  {
  private:
    TagId tagMaxSpeed;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API GradeFeatureValue : public FeatureValue
  {
  private:
    uint8_t grade;

  public:
    inline GradeFeatureValue()
    : grade(0)
    {

    }

    inline GradeFeatureValue(uint8_t grade)
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

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
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
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API AdminLevelFeatureValue : public FeatureValue
  {
  private:
    uint8_t adminLevel;

  public:
    inline AdminLevelFeatureValue()
    : adminLevel(0)
    {

    }

    inline AdminLevelFeatureValue(uint8_t adminLevel)
    : adminLevel(adminLevel)
    {
      // no code
    }

    inline void SetAdminLevel(uint8_t adminLevel)
    {
      this->adminLevel=adminLevel;
    }

    inline uint8_t GetAdminLevel() const
    {
      return adminLevel;
    }

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
  };

  class OSMSCOUT_API AdminLevelFeature : public Feature
  {
  private:
    TagId tagAdminLevel;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API BridgeFeature : public Feature
  {
  private:
    TagId tagBridge;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API TunnelFeature : public Feature
  {
  private:
    TagId tagTunnel;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API RoundaboutFeature : public Feature
  {
  private:
    TagId tagJunction;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  class OSMSCOUT_API EleFeatureValue : public FeatureValue
  {
  private:
    uint32_t ele;

  public:
    inline EleFeatureValue()
    : ele(0)
    {

    }

    inline EleFeatureValue(uint32_t ele)
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

    inline std::string GetLabel() const
    {
      return NumberToString(ele)+"m";
    }

    bool Read(FileScanner& scanner);
    bool Write(FileWriter& writer);

    FeatureValue& operator=(const FeatureValue& other);
    bool operator==(const FeatureValue& other) const;
  };

  class OSMSCOUT_API EleFeature : public Feature
  {
  private:
    TagId tagEle;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "name" label */
    static const char* const NAME_LABEL;

    /** Index of the 'name' label */
    static const size_t      NAME_LABEL_INDEX;

  public:
    EleFeature();
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    size_t GetValueSize() const;
    FeatureValue* AllocateValue(void* buffer);

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const std::unordered_map<TagId,std::string>& tags,
               FeatureValueBuffer& buffer) const;
  };

  /**
   * Helper template class for easy access to flag-like Features.
   *
   * Each type may have stored the feature in request at a different index. The FeatureReader
   * caches the index for each type once in the constructor and later on allows access to the feature
   * in O(1) - without iterating of all feature(values) of an object.
   */
  template<class F>
  class OSMSCOUT_API FeatureReader
  {
  private:
    std::vector<size_t> lookupTable;

  public:
    FeatureReader(const TypeConfig& typeConfig);

    /**
     * Returns the index of the Feature/FeatureValue within the given FeatureValueBuffer.
     *
     * @param buffer
     *    The FeatureValueBuffer instance
     * @param index
     *    The index
     * @return
     *    true, if there is a valid index 8because the type has such feature), else false
     */
    bool GetIndex(const FeatureValueBuffer& buffer,
                  size_t& index) const;

    /**
     * Returns true, if the feature is set for the given FeatureValueBuffer
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    true if set, else false
     */
    bool IsSet(const FeatureValueBuffer& buffer) const;
  };

  template<class F>
  FeatureReader<F>::FeatureReader(const TypeConfig& typeConfig)
  {
    FeatureRef feature=typeConfig.GetFeature(F::NAME);

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(F::NAME,
                          index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  template<class F>
  bool FeatureReader<F>::GetIndex(const FeatureValueBuffer& buffer,
                                  size_t& index) const
  {
    index=lookupTable[buffer.GetType()->GetIndex()];

    return index!=std::numeric_limits<size_t>::max();
  }

  template<class F>
  bool FeatureReader<F>::IsSet(const FeatureValueBuffer& buffer) const
  {
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max()) {
      return buffer.HasValue(index);
    }
    else {
      return false;
    }
  }

  typedef FeatureReader<AccessRestrictedFeature> AccessRestrictedFeatureReader;
  typedef FeatureReader<BridgeFeature>           BridgeFeatureReader;
  typedef FeatureReader<TunnelFeature>           TunnelFeatureReader;
  typedef FeatureReader<RoundaboutFeature>       RoundaboutFeatureReader;

  /**
   * Helper template class for easy access to the value of a certain feature for objects of any type.
   *
   * Each type may have stored the feature in request at a different index. The FeatureValueReader
   * caches the index for each type once in the constructor and later on allows access to the feature value
   * in O(1) - without iterating of all feature(values) of an object.
   */
  template<class F, class V>
  class OSMSCOUT_API FeatureValueReader
  {
  private:
    std::vector<size_t> lookupTable;

  public:
    FeatureValueReader(const TypeConfig& typeConfig);

    /**
     * Returns the index of the Feature/FeatureValue within the given FeatureValueBuffer.
     *
     * @param buffer
     *    The FeatureValueBuffer instance
     * @param index
     *    The index
     * @return
     *    true, if there is a valid index 8because the type has such feature), else false
     */
    bool GetIndex(const FeatureValueBuffer& buffer,
                  size_t& index) const;

    /**
     * Returns the FeatureValue for the given FeatureValueBuffer
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    A pointer to an instance if the Type and the instance do have the feature and its value is not NULL,
     *    else NULL
     */
    V* GetValue(const FeatureValueBuffer& buffer) const;
  };

  template<class F, class V>
  FeatureValueReader<F,V>::FeatureValueReader(const TypeConfig& typeConfig)
  {
    FeatureRef feature=typeConfig.GetFeature(F::NAME);

    assert(feature->HasValue());

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(F::NAME,
                          index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  template<class F, class V>
  bool FeatureValueReader<F,V>::GetIndex(const FeatureValueBuffer& buffer,
                                         size_t& index) const
  {
    index=lookupTable[buffer.GetType()->GetIndex()];

    return index!=std::numeric_limits<size_t>::max();
  }

  template<class F, class V>
  V* FeatureValueReader<F,V>::GetValue(const FeatureValueBuffer& buffer) const
  {
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max() &&
        buffer.HasValue(index)) {
      return dynamic_cast<V*>(buffer.GetValue(index));
    }
    else {
      return NULL;
    }
  }

  typedef FeatureValueReader<NameFeature,NameFeatureValue>             NameFeatureValueReader;
  typedef FeatureValueReader<NameAltFeature,NameAltFeatureValue>       NameAltFeatureValueReader;
  typedef FeatureValueReader<RefFeature,RefFeatureValue>               RefFeatureValueReader;
  typedef FeatureValueReader<LocationFeature,LocationFeatureValue>     LocationFeatureValueReader;
  typedef FeatureValueReader<AddressFeature,AddressFeatureValue>       AddressFeatureValueReader;
  typedef FeatureValueReader<AccessFeature,AccessFeatureValue>         AccessFeatureValueReader;
  typedef FeatureValueReader<LayerFeature,LayerFeatureValue>           LayerFeatureValueReader;
  typedef FeatureValueReader<WidthFeature,WidthFeatureValue>           WidthFeatureValueReader;
  typedef FeatureValueReader<MaxSpeedFeature,MaxSpeedFeatureValue>     MaxSpeedFeatureValueReader;
  typedef FeatureValueReader<GradeFeature,GradeFeatureValue>           GradeFeatureValueReader;
  typedef FeatureValueReader<AdminLevelFeature,AdminLevelFeatureValue> AdminLevelFeatureValueReader;

  template <class F, class V>
  class OSMSCOUT_API FeatureLabelReader
  {
  private:
    std::vector<size_t> lookupTable;

  public:
    FeatureLabelReader(const TypeConfig& typeConfig);

    /**
     * Returns the label of the given object
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    The label, if the given feature has a value and a label  or a empty string
     */
    std::string GetLabel(const FeatureValueBuffer& buffer) const;
  };

  template<class F, class V>
  FeatureLabelReader<F,V>::FeatureLabelReader(const TypeConfig& typeConfig)
  {
    FeatureRef feature=typeConfig.GetFeature(F::NAME);

    assert(feature->HasLabel());

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(F::NAME,
                          index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  template<class F, class V>
  std::string FeatureLabelReader<F,V>::GetLabel(const FeatureValueBuffer& buffer) const
  {
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max() &&
        buffer.HasValue(index)) {
      V* value=dynamic_cast<V*>(buffer.GetValue(index));

      if (value!=NULL) {
        return value->GetLabel();
      }
    }

    return "";
  }

  typedef FeatureLabelReader<NameFeature,NameFeatureValue>         NameFeatureLabelReader;

  /**
   * \defgroup type Object type related data structures and services
   */
}

#endif
