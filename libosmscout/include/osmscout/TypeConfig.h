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

#include <osmscout/util/Reference.h>

namespace osmscout {

  const static TagId tagIgnore        = 0;

  const static TypeId typeIgnore      = 0;

  class OSMSCOUT_API Condition : public Referencable
  {
  public:
    virtual ~Condition();

    virtual bool Evaluate(const std::map<TagId,std::string>& tagMap) const = 0;
  };

  typedef Ref<Condition> ConditionRef;

  class OSMSCOUT_API NotCondition : public Condition
  {
  private:
    ConditionRef condition;

  public:
    NotCondition(Condition* condition);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  class OSMSCOUT_API AndCondition : public Condition
  {
  private:
    std::list<ConditionRef> conditions;

  public:
    AndCondition();

    void AddCondition(Condition* condition);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  class OSMSCOUT_API OrCondition : public Condition
  {
  private:
    std::list<ConditionRef> conditions;

  public:
    OrCondition();

    void AddCondition(Condition* condition);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  class OSMSCOUT_API ExistsCondition : public Condition
  {
  private:
    TagId tag;

  public:
    ExistsCondition(TagId tag);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  class OSMSCOUT_API EqualsCondition : public Condition
  {
  private:
    TagId       tag;
    std::string tagValue;

  public:
    EqualsCondition(TagId tag,
                    const std::string& tagValue);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  class OSMSCOUT_API NotEqualsCondition : public Condition
  {
  private:
    TagId       tag;
    std::string tagValue;

  public:
    NotEqualsCondition(TagId tag,
                       const std::string& tagValue);

    bool Evaluate(const std::map<TagId,std::string>& tagMap) const;
  };

  class OSMSCOUT_API IsInCondition : public Condition
  {
  private:
    TagId                 tag;
    std::set<std::string> tagValues;

  public:
    IsInCondition(TagId tag);

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
      unsigned char types;
      ConditionRef  condition;
    };

  private:
    TypeId       id;
    std::string  name;

    std::list<TypeCondition> conditions;

    bool         canBeNode;
    bool         canBeWay;
    bool         canBeArea;
    bool         canBeRelation;
    bool         canBeRoute;
    bool         canBeIndexed;
    bool         consumeChildren;
    bool         optimizeLowZoom;
    bool         multipolygon;
    bool         pinWay;
    bool         ignore;

  public:
    TypeInfo();
    virtual ~TypeInfo();

    TypeInfo& SetId(TypeId id);

    TypeInfo& SetType(const std::string& name);

    TypeInfo& AddCondition(unsigned char types,
                           Condition* condition);

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

    inline TypeInfo& CanBeRoute(bool canBeRoute)
    {
      this->canBeRoute=canBeRoute;

      return *this;
    }

    inline bool CanBeRoute() const
    {
      return canBeRoute;
    }

    inline TypeInfo& CanBeIndexed(bool canBeIndexed)
    {
      this->canBeIndexed=canBeIndexed;

      return *this;
    }

    inline bool CanBeIndexed() const
    {
      return canBeIndexed;
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

  class OSMSCOUT_API TypeConfig
  {
  private:
    std::vector<TagInfo>                            tags;
    std::vector<TypeInfo>                           types;

    TagId                                           nextTagId;
    TypeId                                          nextTypeId;

    std::map<std::string,TagId>                     stringToTagMap;
    std::map<std::string,TypeInfo>                  nameToTypeMap;
    std::map<TypeId,TypeInfo>                       idToTypeMap;

  public:
    TagId                                           tagAdminLevel;
    TagId                                           tagBoundary;
    TagId                                           tagBridge;
    TagId                                           tagLayer;
    TagId                                           tagName;
    TagId                                           tagOneway;
    TagId                                           tagPlace;
    TagId                                           tagPlaceName;
    TagId                                           tagRef;
    TagId                                           tagTunnel;
    TagId                                           tagType;
    TagId                                           tagWidth;
    TagId                                           tagArea;
    TagId                                           tagHouseNr;
    TagId                                           tagJunction;
    TagId                                           tagMaxSpeed;
    TagId                                           tagAccess;

  public:
    TypeConfig();
    virtual ~TypeConfig();

    void RestoreTagInfo(const TagInfo& tagInfo);

    TagId RegisterTagForInternalUse(const std::string& tagName);
    TagId RegisterTagForExternalUse(const std::string& tagName);

    TypeConfig& AddTypeInfo(TypeInfo& typeInfo);

    const std::vector<TagInfo>& GetTags() const;
    const std::vector<TypeInfo>& GetTypes() const;

    TypeId GetMaxTypeId() const;

    TagId GetTagId(const char* name) const;

    const TagInfo& GetTagInfo(TagId id) const;
    const TypeInfo& GetTypeInfo(TypeId id) const;

    void ResolveTags(const std::map<TagId,std::string>& map,
                     std::vector<Tag>& tags) const;

    bool GetNodeTypeId(const std::map<TagId,std::string>& tagMap,
                       TypeId &typeId) const;
    bool GetWayAreaTypeId(const std::map<TagId,std::string>& tagMap,
                          TypeId &wayType,
                          TypeId &areaType) const;
    bool GetRelationTypeId(const std::map<TagId,std::string>& tagMap,
                           TypeId &typeId) const;

    TypeId GetNodeTypeId(const std::string& name) const;
    TypeId GetWayTypeId(const std::string& name) const;
    TypeId GetAreaTypeId(const std::string& name) const;
    TypeId GetRelationTypeId(const std::string& name) const;

    void GetRoutables(std::set<TypeId>& types) const;
    void GetIndexables(std::set<TypeId>& types) const;
  };
}

#endif
