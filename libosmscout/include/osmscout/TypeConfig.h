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

#include <osmscout/Tag.h>
#include <osmscout/Types.h>

#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/Parser.h>
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

  /**
   * \ingroup type
   *
   *  Detailed information about one object type
   *
   *  \see TypeConfig
   */
  class OSMSCOUT_API TypeInfo
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
    TypeId       id;
    std::string  name;

    std::list<TypeCondition> conditions;

    bool         canBeNode;
    bool         canBeWay;
    bool         canBeArea;
    bool         canBeRelation;
    bool         canRouteFoot;
    bool         canRouteBicycle;
    bool         canRouteCar;
    bool         indexAsLocation;
    bool         indexAsRegion;
    bool         indexAsPOI;
    bool         optimizeLowZoom;
    bool         multipolygon;
    bool         pinWay;
    bool         ignoreSeaLand;
    bool         ignore;

  public:
    TypeInfo();
    virtual ~TypeInfo();

    TypeInfo& SetId(TypeId id);

    TypeInfo& SetType(const std::string& name);

    TypeInfo& AddCondition(unsigned char types,
                           TagCondition* condition);

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

  /**
   * \ingroup type
   *
   * The TypeConfig class holds information about object types
   * defined by a database instance.
   */
  class OSMSCOUT_API TypeConfig : public Referencable
  {
  private:
    std::vector<TagInfo>                   tags;
    std::vector<TypeInfo>                  types;

    TagId                                  nextTagId;
    TypeId                                 nextTypeId;

    OSMSCOUT_HASHMAP<std::string,TagId>    stringToTagMap;
    OSMSCOUT_HASHMAP<std::string,TypeInfo> nameToTypeMap;
    OSMSCOUT_HASHMAP<TypeId,TypeInfo>      idToTypeMap;
    OSMSCOUT_HASHMAP<TagId,uint32_t>       nameTagIdToPrioMap;
    OSMSCOUT_HASHMAP<TagId,uint32_t>       nameAltTagIdToPrioMap;

    OSMSCOUT_HASHMAP<std::string,size_t>   surfaceToGradeMap;

  public:
    TypeId                                 typeTileLand;
    TypeId                                 typeTileSea;
    TypeId                                 typeTileCoast;
    TypeId                                 typeTileUnknown;
    TypeId                                 typeTileCoastline;

    // External use (also available in "normal" types, if not explicitly deleted)
    TagId                                  tagRef;
    TagId                                  tagBridge;
    TagId                                  tagTunnel;
    TagId                                  tagLayer;
    TagId                                  tagWidth;
    TagId                                  tagOneway;
    TagId                                  tagHouseNr;
    TagId                                  tagJunction;
    TagId                                  tagMaxSpeed;
    TagId                                  tagSurface;
    TagId                                  tagTracktype;

    TagId                                  tagAdminLevel;

    TagId                                  tagAccess;
    TagId                                  tagAccessForward;
    TagId                                  tagAccessBackward;

    TagId                                  tagAccessFoot;
    TagId                                  tagAccessFootForward;
    TagId                                  tagAccessFootBackward;

    TagId                                  tagAccessBicycle;
    TagId                                  tagAccessBicycleForward;
    TagId                                  tagAccessBicycleBackward;

    TagId                                  tagAccessMotorVehicle;
    TagId                                  tagAccessMotorVehicleForward;
    TagId                                  tagAccessMotorVehicleBackward;

    TagId                                  tagAccessMotorcar;
    TagId                                  tagAccessMotorcarForward;
    TagId                                  tagAccessMotorcarBackward;

    TagId                                  tagAddrStreet;

    // Internal use (only available during preprocessing)
    TagId                                  tagArea;
    TagId                                  tagNatural;
    TagId                                  tagType;
    TagId                                  tagRestriction;

  public:
    TypeConfig();
    virtual ~TypeConfig();

    void RestoreTagInfo(const TagInfo& tagInfo);
    void RestoreNameTagInfo(TagId tagId, uint32_t priority);
    void RestoreNameAltTagInfo(TagId tagId, uint32_t priority);

    TagId RegisterTagForInternalUse(const std::string& tagName);
    TagId RegisterTagForExternalUse(const std::string& tagName);

    void RegisterNameTag(const std::string& tagName, uint32_t priority);
    void RegisterNameAltTag(const std::string& tagName, uint32_t priority);

    TypeConfig& AddTypeInfo(TypeInfo& typeInfo);

    const std::vector<TagInfo>& GetTags() const;
    const std::vector<TypeInfo>& GetTypes() const;

    TypeId GetMaxTypeId() const;

    TagId GetTagId(const char* name) const;

    const TagInfo& GetTagInfo(TagId id) const;
    const TypeInfo& GetTypeInfo(TypeId id) const;

    void ResolveTags(const std::map<TagId,std::string>& map,
                     std::vector<Tag>& tags) const;

    bool IsNameTag(TagId tag, uint32_t& priority) const;
    bool IsNameAltTag(TagId tag, uint32_t& priority) const;

    bool GetNodeTypeId(const std::map<TagId,std::string>& tagMap,
                       TypeId &typeId) const;
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
  };


  //! \ingroup type
  //! Reference counted reference to a TypeConfig instance
  typedef Ref<TypeConfig> TypeConfigRef;

  /**
   * \defgroup type Object type related data structures and services
   */
}

#endif
