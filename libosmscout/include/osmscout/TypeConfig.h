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

  const static TagId tagIgnore        = 0;

  const static TypeId typeIgnore      = 0;

  class OSMSCOUT_API TagCondition : public Referencable
  {
  public:
    virtual ~TagCondition();

    virtual bool Evaluate(const std::map<TagId,std::string>& tagMap) const = 0;
  };

  typedef Ref<TagCondition> TagConditionRef;

  class OSMSCOUT_API TagNotCondition : public TagCondition
  {
  private:
    TagConditionRef condition;

  public:
    TagNotCondition(TagCondition* condition);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

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

  class OSMSCOUT_API TagExistsCondition : public TagCondition
  {
  private:
    TagId tag;

  public:
    TagExistsCondition(TagId tag);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  class OSMSCOUT_API TagBinaryCondition : public TagCondition
  {
  private:
    TagId          tag;
    BinaryOperator binaryOperator;
    std::string    tagValue;

  public:
    TagBinaryCondition(TagId tag,
                       BinaryOperator binaryOperator,
                       const std::string& tagValue);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

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

  class OSMSCOUT_API TypeInfo
  {
  public:
    const static unsigned char typeNode     = 1 << 0;
    const static unsigned char typeWay      = 1 << 1;
    const static unsigned char typeArea     = 1 << 2;
    const static unsigned char typeRelation = 1 << 3;

  public:
    struct TypeCondition
    {
      unsigned char    types;
      TagConditionRef  condition;
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
    bool         consumeChildren;
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

    inline TypeId GetId() const
    {
      return id;
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline bool HasConditions() const
    {
      return conditions.size()>0;
    }

    inline const std::list<TypeCondition>& GetConditions() const
    {
      return conditions;
    }

    inline TypeInfo& CanBeNode(bool canBeNode)
    {
      this->canBeNode=canBeNode;

      return *this;
    }

    inline bool CanBeNode() const
    {
      return canBeNode;
    }

    inline TypeInfo& CanBeWay(bool canBeWay)
    {
      this->canBeWay=canBeWay;

      return *this;
    }

    inline bool CanBeWay() const
    {
      return canBeWay;
    }

    inline TypeInfo& CanBeArea(bool canBeArea)
    {
      this->canBeArea=canBeArea;

      return *this;
    }

    inline bool CanBeArea() const
    {
      return canBeArea;
    }

    inline TypeInfo& CanBeRelation(bool canBeRelation)
    {
      this->canBeRelation=canBeRelation;

      return *this;
    }

    inline bool CanBeRelation() const
    {
      return canBeRelation;
    }

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

    inline TypeInfo& CanRouteCar(bool canBeRoute)
    {
      this->canRouteCar=canBeRoute;

      return *this;
    }

    inline bool CanRoute() const
    {
      return canRouteFoot || canRouteBicycle || canRouteCar;
    }

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

    inline TypeInfo& SetIndexAsLocation(bool indexAsLocation)
    {
      this->indexAsLocation=indexAsLocation;

      return *this;
    }

    inline bool GetIndexAsLocation() const
    {
      return indexAsLocation;
    }

    inline TypeInfo& SetIndexAsRegion(bool indexAsRegion)
    {
      this->indexAsRegion=indexAsRegion;

      return *this;
    }

    inline bool GetIndexAsRegion() const
    {
      return indexAsRegion;
    }

    inline TypeInfo& SetIndexAsPOI(bool indexAsPOI)
    {
      this->indexAsPOI=indexAsPOI;

      return *this;
    }

    inline bool GetIndexAsPOI() const
    {
      return indexAsPOI;
    }

    inline TypeInfo& SetConsumeChildren(bool consumeChildren)
    {
      this->consumeChildren=consumeChildren;

      return *this;
    }

    inline bool GetConsumeChildren() const
    {
      return consumeChildren;
    }

    inline TypeInfo& SetOptimizeLowZoom(bool optimize)
    {
      this->optimizeLowZoom=optimize;

      return *this;
    }

    inline bool GetOptimizeLowZoom() const
    {
      return optimizeLowZoom;
    }

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

    inline TypeInfo& SetIgnoreSeaLand(bool ignoreSeaLand)
    {
      this->ignoreSeaLand=ignoreSeaLand;

      return *this;
    }

    inline bool GetIgnoreSeaLand() const
    {
      return ignoreSeaLand;
    }

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
    TagId                                  tagType;
    TagId                                  tagWidth;
    TagId                                  tagOneway;
    TagId                                  tagHouseNr;
    TagId                                  tagStreet;
    TagId                                  tagJunction;
    TagId                                  tagMaxSpeed;
    TagId                                  tagRestriction;
    TagId                                  tagSurface;
    TagId                                  tagTracktype;
    TagId                                  tagPlace;
    TagId                                  tagBoundary;
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

    // Internal use (only available during preprocessing)
    TagId                                  tagArea;
    TagId                                  tagNatural;

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

    void GetRoutables(std::set<TypeId>& types) const;
    void GetIndexables(OSMSCOUT_HASHSET<TypeId>& types) const;
    void GetIndexAsRegionTypes(OSMSCOUT_HASHSET<TypeId>& types) const;
    void GetIndexAsPOITypes(OSMSCOUT_HASHSET<TypeId>& types) const;

    void RegisterSurfaceToGradeMapping(const std::string& surface,
                                       size_t grade);
    bool GetGradeForSurface(const std::string& surface,
                            size_t& grade) const;
  };

  typedef Ref<TypeConfig> TypeConfigRef;
}

#endif
