#ifndef OSMSCOUT_TYPECONFIG_H
#define OSMSCOUT_TYPECONFIG_H

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

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <osmscout/ObjectRef.h>
#include <osmscout/Tag.h>
#include <osmscout/Types.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/Parser.h>
#include <osmscout/util/Progress.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
   * \ingroup type
   *
   * Magic constant for an unresolved and to be ignored tag
   */
  static const TagId tagIgnore        = 0;

  /**
   * \ingroup type
   *
   * Magic constant for an unresolved and to be ignored object type.
   * Object having typeIgnore as type should be handled like
   * they do not have a type at all.
   */
  static const TypeId typeIgnore      = 0;

  // Forward declaration of classes TypeConfig and TypeInfo because
  // of circular dependency between them and Feature
  class TypeConfig;
  class TypeInfo;

  /**
   * \ingroup type
   *
   * Abstract base class for all tag based conditions
   */
  class OSMSCOUT_API TagCondition : public Referencable
  {
  public:
    virtual ~TagCondition();

    virtual bool Evaluate(const std::map<TagId,std::string>& tagMap) const = 0;
  };

  /**
   * \ingroup type
   *
   * Reference counted reference to a tag condition
   */
  typedef Ref<TagCondition> TagConditionRef;

  /**
   * \ingroup type
   *
   * Negates the result of the given child condition
   */
  class OSMSCOUT_API TagNotCondition : public TagCondition
  {
  private:
    TagConditionRef condition;

  public:
    TagNotCondition(TagCondition* condition);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  /**
   * \ingroup type
   *
   * Allows a boolean and/or condition between a number of
   * child conditions.
   */
  class OSMSCOUT_API TagBoolCondition : public TagCondition
  {
  public:
    enum Type {
      boolAnd,
      boolOr
    };

  private:
    std::list<TagConditionRef> conditions;
    Type                       type;

  public:
    TagBoolCondition(Type type);

    void AddCondition(TagCondition* condition);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  /**
   * \ingroup type
   *
   * Returns true, if the given tag exists for an object
   */
  class OSMSCOUT_API TagExistsCondition : public TagCondition
  {
  private:
    TagId tag;

  public:
    TagExistsCondition(TagId tag);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  /**
   * \ingroup type
   *
   * Returns true, if the value of the given tag fulfills the given
   * boolean condition in regard to the comparison value.
   */
  class OSMSCOUT_API TagBinaryCondition : public TagCondition
  {
  private:
    enum ValueType {
      string,
      sizet
    };

  private:
    TagId          tag;
    BinaryOperator binaryOperator;
    ValueType      valueType;
    std::string    tagStringValue;
    size_t         tagSizeValue;

  public:
    TagBinaryCondition(TagId tag,
                       BinaryOperator binaryOperator,
                       const std::string& tagValue);
    TagBinaryCondition(TagId tag,
                       BinaryOperator binaryOperator,
                       const size_t& tagValue);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  /**
   * \ingroup type
   *
   * Returns true, if the tag value of the given is one of the
   * given values.
   */
  class OSMSCOUT_API TagIsInCondition : public TagCondition
  {
  private:
    TagId                 tag;
    std::set<std::string> tagValues;

  public:
    TagIsInCondition(TagId tag);

    void AddTagValue(const std::string& tagValue);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  /**
   * \ingroup type
   *
   * Information about a tag definition
   */
  class OSMSCOUT_API TagInfo
  {
  private:
    TagId       id;
    std::string name;
    bool        internalOnly;

  public:
    TagInfo();
    TagInfo(const std::string& name,
            bool internalOnly);

    TagInfo& SetId(TagId id);
    TagInfo& SetToExternal();

    inline std::string GetName() const
    {
      return name;
    }

    inline TagId GetId() const
    {
      return id;
    }

    inline bool IsInternalOnly() const
    {
      return internalOnly;
    }
  };

  class OSMSCOUT_API FeatureValue
  {
  public:
    FeatureValue();
    virtual ~FeatureValue();
  };

  /**
   * A feature combines one or multiple tags  to build information attribute for a type.
   *
   * The class "Feature" is the abstract base class for a concrete feature implementation
   * like "NameFeature" or "AccessFeature".
   *
   * A feature could just be an alias for one tag (like "name") but it could also combine
   * a number of attributes (e.g. access and all its variations).
   */
  class OSMSCOUT_API Feature : public Referencable
  {
  public:
    Feature();
    virtual ~Feature();

    /**
     * Does further initialization based on the current TypeConfig. For example
     * it registers Tags (and stores their TagId) for further processing.
     */
    virtual void Initialize(TypeConfig& typeConfig) = 0;

    /**
     * Returns the name of the feature
     */
    virtual std::string GetName() const = 0;

    virtual void Parse(Progress& progress,
                       const TypeConfig& typeConfig,
                       const ObjectOSMRef& object,
                       const TypeInfo& type,
                       const std::map<TagId,std::string>& tags,
                       FeatureValue*& value,
                       bool& set) const = 0;
  };

  typedef Ref<Feature> FeatureRef;

  class OSMSCOUT_API NameFeatureValue : public FeatureValue
  {
  private:
    std::string name;
    std::string nameAlt;

  public:
    inline NameFeatureValue(const std::string& name,
                            const std::string& nameAlt)
    : name(name),
      nameAlt(nameAlt)
    {
      // no code
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline std::string GetNameAlt() const
    {
      return nameAlt;
    }
  };

  class OSMSCOUT_API NameFeature : public Feature
  {
  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
  };

  class OSMSCOUT_API RefFeatureValue : public FeatureValue
  {
  private:
    std::string ref;

  public:
    inline RefFeatureValue(const std::string& ref)
    : ref(ref)
    {
      // no code
    }

    inline std::string GetRef() const
    {
      return ref;
    }
  };

  class OSMSCOUT_API RefFeature : public Feature
  {
  private:
    TagId tagRef;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
  };

  class OSMSCOUT_API AddressFeatureValue : public FeatureValue
  {
  private:
    std::string location;
    std::string address;

  public:
    inline AddressFeatureValue(const std::string& location,
                               const std::string& address)
    : location(location),
      address(address)
    {
      // no code
    }

    inline std::string GetLocation() const
    {
      return location;
    }

    inline std::string GetAddress() const
    {
      return address;
    }
  };

  class OSMSCOUT_API AddressFeature : public Feature
  {
  private:
    TagId tagAddrHouseNr;
    TagId tagAddrStreet;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TypeConfig& typeConfig);

    std::string GetName() const;

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
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
    inline AccessFeatureValue(uint8_t access)
    : access(access)
    {
      // no code
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
  };

  class OSMSCOUT_API AccessFeature : public Feature
  {
  private:
    TagId tagOneway;
    TagId tagJunction;

    TagId tagAccess;
    TagId tagAccessForward;
    TagId tagAccessBackward;

    TagId tagAccessFoot;
    TagId tagAccessFootForward;
    TagId tagAccessFootBackward;

    TagId tagAccessBicycle;
    TagId tagAccessBicycleForward;
    TagId tagAccessBicycleBackward;

    TagId tagAccessMotorVehicle;
    TagId tagAccessMotorVehicleForward;
    TagId tagAccessMotorVehicleBackward;

    TagId tagAccessMotorcar;
    TagId tagAccessMotorcarForward;
    TagId tagAccessMotorcarBackward;

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

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
  };

  class OSMSCOUT_API LayerFeatureValue : public FeatureValue
  {
  private:
    int8_t layer;

  public:
    inline LayerFeatureValue(int8_t layer)
    : layer(layer)
    {
      // no code
    }

    inline int8_t GetLayer() const
    {
      return layer;
    }
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

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
  };

  class OSMSCOUT_API WidthFeatureValue : public FeatureValue
  {
  private:
    uint8_t width;

  public:
    inline WidthFeatureValue(uint8_t width)
    : width(width)
    {
      // no code
    }

    inline uint8_t GetWidth() const
    {
      return width;
    }
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

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
  };

  class OSMSCOUT_API MaxSpeedFeatureValue : public FeatureValue
  {
  private:
    uint8_t maxSpeed;

  public:
    inline MaxSpeedFeatureValue(uint8_t maxSpeed)
    : maxSpeed(maxSpeed)
    {
      // no code
    }

    inline uint8_t GetMaxSpeed() const
    {
      return maxSpeed;
    }
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

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
  };

  class OSMSCOUT_API GradeFeatureValue : public FeatureValue
  {
  private:
    uint8_t grade;

  public:
    inline GradeFeatureValue(uint8_t grade)
    : grade(grade)
    {
      // no code
    }

    inline uint8_t GetGrade() const
    {
      return grade;
    }
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

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
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

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
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

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
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

    void Parse(Progress& progress,
               const TypeConfig& typeConfig,
               const ObjectOSMRef& object,
               const TypeInfo& type,
               const std::map<TagId,std::string>& tags,
               FeatureValue*& value,
               bool& set) const;
  };

  /**
   * \ingroup type
   *
   *  Detailed information about one object type
   *
   *  \see TypeConfig
   */
  class OSMSCOUT_API TypeInfo : public Referencable
  {
  public:
    static const unsigned char typeNode     = 1 << 0;
    static const unsigned char typeWay      = 1 << 1;
    static const unsigned char typeArea     = 1 << 2;
    static const unsigned char typeRelation = 1 << 3;

  public:
    /**
     * \ingroup type
     *
     * A type can have a number of conditions that allow
     * to identify the type of an object based on its
     * tag values.
     */
    struct TypeCondition
    {
      unsigned char    types;     //<! Bitset of types the condition can be applied to
      TagConditionRef  condition; //<! The root condition
    };

  private:
    TypeId                        id;
    std::string                   name;

    std::list<TypeCondition>      conditions;
    OSMSCOUT_HASHSET<std::string> featureSet;
    std::list<FeatureRef>         features;

    bool                          canBeNode;
    bool                          canBeWay;
    bool                          canBeArea;
    bool                          canBeRelation;
    bool                          canRouteFoot;
    bool                          canRouteBicycle;
    bool                          canRouteCar;
    bool                          indexAsLocation;
    bool                          indexAsRegion;
    bool                          indexAsPOI;
    bool                          optimizeLowZoom;
    bool                          multipolygon;
    bool                          pinWay;
    bool                          ignoreSeaLand;
    bool                          ignore;

  private:
    TypeInfo(const TypeInfo& other);

  public:
    TypeInfo();
    virtual ~TypeInfo();

    /**
     * Set the id of this type
     */
    TypeInfo& SetId(TypeId id);

    /**
     * The the name of this type
     */
    TypeInfo& SetType(const std::string& name);

    TypeInfo& AddCondition(unsigned char types,
                           TagCondition* condition);

    /**
     * Add a feature to this type
     */
    TypeInfo& AddFeature(const FeatureRef& feature);

    /**
     * Returns true, if the feature with the given name has already been
     * assigned to this type.
     */
    bool HasFeature(const std::string& featureName) const;

    /**
     * Return the list of features assigned to this type
     */
    inline const std::list<FeatureRef>& GetFeatures() const
    {
      return features;
    }

    /**
     * Returns the number of bits required for storing the feature mask
     */
    inline size_t GetFeatureBits() const
    {
      return features.size();
    }

    /**
     * Returns the (rounded) number of bytes required for storing the feature mask
     */
    inline size_t GetFeatureBytes() const
    {
      size_t size=features.size();

      if (size%8==0) {
        return size/8;
      }
      else {
        return size/8+1;
      }
    }

    /**
     * The Type Id of the given type
     */
    inline TypeId GetId() const
    {
      return id;
    }

    /**
     * The name of the given type
     */
    inline std::string GetName() const
    {
      return name;
    }

    /**
     * Returns true, if there are any conditions bound to the type. If the conditions
     * are met for a given object, the object is in turn of the given type.
     * to
     */
    inline bool HasConditions() const
    {
      return conditions.size()>0;
    }

    /**
     * Returns the list of conditions for the given type.
     */
    inline const std::list<TypeCondition>& GetConditions() const
    {
      return conditions;
    }

    /**
     * If set to 'true', a node can be of this type.
     */
    inline TypeInfo& CanBeNode(bool canBeNode)
    {
      this->canBeNode=canBeNode;

      return *this;
    }

    inline bool CanBeNode() const
    {
      return canBeNode;
    }

    /**
     * If set to 'true', a way can be of this type.
     */
    inline TypeInfo& CanBeWay(bool canBeWay)
    {
      this->canBeWay=canBeWay;

      return *this;
    }

    inline bool CanBeWay() const
    {
      return canBeWay;
    }

    /**
     * If set to 'true', an area can be of this type.
     */
    inline TypeInfo& CanBeArea(bool canBeArea)
    {
      this->canBeArea=canBeArea;

      return *this;
    }

    inline bool CanBeArea() const
    {
      return canBeArea;
    }

    /**
     * If set to 'true', a relation can be of this type.
     */
    inline TypeInfo& CanBeRelation(bool canBeRelation)
    {
      this->canBeRelation=canBeRelation;

      return *this;
    }

    inline bool CanBeRelation() const
    {
      return canBeRelation;
    }

    /**
     * If set to 'true', an object of this type can be traveled by feet by default.
     */
    inline TypeInfo& CanRouteFoot(bool canBeRoute)
    {
      this->canRouteFoot=canBeRoute;

      return *this;
    }

    inline TypeInfo& CanRouteBicycle(bool canBeRoute)
    {
      this->canRouteBicycle=canBeRoute;

      return *this;
    }

    /**
     * If set to 'true', an object of this type can be traveled by car by default.
     */
    inline TypeInfo& CanRouteCar(bool canBeRoute)
    {
      this->canRouteCar=canBeRoute;

      return *this;
    }

    inline bool CanRoute() const
    {
      return canRouteFoot || canRouteBicycle || canRouteCar;
    }

    /**
     * If set to 'true', an object of this type can be traveled by the given vehicle by default.
     */
    inline bool CanRoute(Vehicle vehicle) const
    {
      switch (vehicle)
      {
      case vehicleFoot:
        return canRouteFoot;
      case vehicleBicycle:
        return canRouteBicycle;
      case vehicleCar:
        return canRouteCar;
      }

      return false;
    }

    inline bool CanRouteFoot() const
    {
      return canRouteFoot;
    }

    inline bool CanRouteBicycle() const
    {
      return canRouteBicycle;
    }

    inline bool CanRouteCar() const
    {
      return canRouteCar;
    }

    /**
     * Sets, if an object of this type should be indexed as a location.
     */
    inline TypeInfo& SetIndexAsLocation(bool indexAsLocation)
    {
      this->indexAsLocation=indexAsLocation;

      return *this;
    }

    inline bool GetIndexAsLocation() const
    {
      return indexAsLocation;
    }

    /**
     * Sets, if an object of this type should be indexed as a region.
     */
    inline TypeInfo& SetIndexAsRegion(bool indexAsRegion)
    {
      this->indexAsRegion=indexAsRegion;

      return *this;
    }

    inline bool GetIndexAsRegion() const
    {
      return indexAsRegion;
    }

    /**
     * Sets, if an object of this type should be indexed as a POI.
     */
    inline TypeInfo& SetIndexAsPOI(bool indexAsPOI)
    {
      this->indexAsPOI=indexAsPOI;

      return *this;
    }

    inline bool GetIndexAsPOI() const
    {
      return indexAsPOI;
    }

    /**
     * Sets, if an object of this type should be optimized for low zoom.
     */
    inline TypeInfo& SetOptimizeLowZoom(bool optimize)
    {
      this->optimizeLowZoom=optimize;

      return *this;
    }

    inline bool GetOptimizeLowZoom() const
    {
      return optimizeLowZoom;
    }

    /**
     * If set to 'true', an object is handled as multipolygon even though it may not have
     * type=multipolygon set explicitly.
     */
    inline TypeInfo& SetMultipolygon(bool multipolygon)
    {
      this->multipolygon=multipolygon;

      return *this;
    }

    inline bool GetMultipolygon() const
    {
      return multipolygon;
    }

    inline TypeInfo& SetPinWay(bool pinWay)
    {
      this->pinWay=pinWay;

      return *this;
    }

    inline bool GetPinWay() const
    {
      return pinWay;
    }

    /**
     * Sets, if an object of this type should be ignored for land/sea calculation.
     */
    inline TypeInfo& SetIgnoreSeaLand(bool ignoreSeaLand)
    {
      this->ignoreSeaLand=ignoreSeaLand;

      return *this;
    }

    inline bool GetIgnoreSeaLand() const
    {
      return ignoreSeaLand;
    }

    /**
     * If set to true, an object of this typoe should be ignored (not exported for renderng, routing,
     * location indexing or other services).
     */
    inline TypeInfo& SetIgnore(bool ignore)
    {
      this->ignore=ignore;

      return *this;
    }

    inline bool GetIgnore() const
    {
      return ignore;
    }
  };

  typedef Ref<TypeInfo> TypeInfoRef;

  /**
   * \ingroup type
   *
   * The TypeConfig class holds information about object types
   * defined by a database instance.
   */
  class OSMSCOUT_API TypeConfig : public Referencable
  {
  private:
    std::vector<TagInfo>                      tags;
    std::vector<TypeInfoRef>                  types;

    TagId                                     nextTagId;
    TypeId                                    nextTypeId;

    OSMSCOUT_HASHMAP<std::string,TagId>       stringToTagMap;
    OSMSCOUT_HASHMAP<std::string,TypeInfoRef> nameToTypeMap;
    OSMSCOUT_HASHMAP<TypeId,TypeInfoRef>      idToTypeMap;
    OSMSCOUT_HASHMAP<TagId,uint32_t>          nameTagIdToPrioMap;
    OSMSCOUT_HASHMAP<TagId,uint32_t>          nameAltTagIdToPrioMap;

    OSMSCOUT_HASHMAP<std::string,size_t>      surfaceToGradeMap;

    OSMSCOUT_HASHMAP<std::string,FeatureRef>  nameToFeatureMap;

  public:
    TypeInfoRef                               typeInfoIgnore;

    TypeId                                    typeTileLand;
    TypeId                                    typeTileSea;
    TypeId                                    typeTileCoast;
    TypeId                                    typeTileUnknown;
    TypeId                                    typeTileCoastline;

    // External use (also available in "normal" types, if not explicitly deleted)
    TagId                                     tagRef;
    TagId                                     tagBridge;
    TagId                                     tagTunnel;
    TagId                                     tagLayer;
    TagId                                     tagWidth;
    TagId                                     tagOneway;
    TagId                                     tagHouseNr;
    TagId                                     tagJunction;
    TagId                                     tagMaxSpeed;
    TagId                                     tagSurface;
    TagId                                     tagTracktype;

    TagId                                     tagAdminLevel;

    TagId                                     tagAccess;
    TagId                                     tagAccessForward;
    TagId                                     tagAccessBackward;

    TagId                                     tagAccessFoot;
    TagId                                     tagAccessFootForward;
    TagId                                     tagAccessFootBackward;

    TagId                                     tagAccessBicycle;
    TagId                                     tagAccessBicycleForward;
    TagId                                     tagAccessBicycleBackward;

    TagId                                     tagAccessMotorVehicle;
    TagId                                     tagAccessMotorVehicleForward;
    TagId                                     tagAccessMotorVehicleBackward;

    TagId                                     tagAccessMotorcar;
    TagId                                     tagAccessMotorcarForward;
    TagId                                     tagAccessMotorcarBackward;

    TagId                                     tagAddrStreet;

    // Internal use (only available during preprocessing)
    TagId                                     tagArea;
    TagId                                     tagNatural;
    TagId                                     tagType;
    TagId                                     tagRestriction;

  public:
    TypeConfig();
    virtual ~TypeConfig();

    TagId RegisterTagForInternalUse(const std::string& tagName);
    TagId RegisterTagForExternalUse(const std::string& tagName);

    TagId RegisterNameTag(const std::string& tagName, uint32_t priority);
    TagId RegisterNameAltTag(const std::string& tagName, uint32_t priority);

    void RegisterFeature(const FeatureRef& feature);
    FeatureRef GetFeature(const std::string& name) const;

    TypeConfig& AddTypeInfo(const TypeInfoRef& typeInfo);

    const std::vector<TagInfo>& GetTags() const;
    const std::vector<TypeInfoRef>& GetTypes() const;

    TypeId GetMaxTypeId() const;

    TagId GetTagId(const char* name) const;

    const TagInfo& GetTagInfo(TagId id) const;
    const TypeInfoRef& GetTypeInfo(TypeId id) const;

    void ResolveTags(const std::map<TagId,std::string>& map,
                     std::vector<Tag>& tags) const;

    bool IsNameTag(TagId tag, uint32_t& priority) const;
    bool IsNameAltTag(TagId tag, uint32_t& priority) const;

    TypeInfoRef GetNodeType(const std::map<TagId,std::string>& tagMap) const;

    bool GetWayAreaTypeId(const std::map<TagId,std::string>& tagMap,
                          TypeId &wayType,
                          TypeId &areaType) const;
    bool GetRelationTypeId(const std::map<TagId,std::string>& tagMap,
                           TypeId &typeId) const;

    TypeId GetTypeId(const std::string& name) const;
    TypeId GetNodeTypeId(const std::string& name) const;
    TypeId GetWayTypeId(const std::string& name) const;
    TypeId GetAreaTypeId(const std::string& name) const;
    TypeId GetRelationTypeId(const std::string& name) const;

    void GetAreaTypes(std::set<TypeId>& types) const;
    void GetWayTypes(std::set<TypeId>& types) const;

    void GetRoutables(std::set<TypeId>& types) const;
    void GetIndexAsLocationTypes(OSMSCOUT_HASHSET<TypeId>& types) const;
    void GetIndexAsRegionTypes(OSMSCOUT_HASHSET<TypeId>& types) const;
    void GetIndexAsPOITypes(OSMSCOUT_HASHSET<TypeId>& types) const;

    void RegisterSurfaceToGradeMapping(const std::string& surface,
                                       size_t grade);
    bool GetGradeForSurface(const std::string& surface,
                            size_t& grade) const;

    bool LoadFromOSTFile(const std::string& filename);
    bool LoadFromDataFile(const std::string& directory);
    bool StoreToDataFile(const std::string& directory) const;
  };


  //! \ingroup type
  //! Reference counted reference to a TypeConfig instance
  typedef Ref<TypeConfig> TypeConfigRef;

  /**
   * \defgroup type Object type related data structures and services
   */
}

#endif
