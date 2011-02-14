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

namespace osmscout {

  const static TagId tagIgnore        = 0;

  const static TypeId typeIgnore      = 0;

  class OSMSCOUT_API TagInfo
  {
  private:
    TagId       id;
    std::string name;

  public:
    TagInfo();
    TagInfo(const std::string& name);

    TagInfo& SetId(TagId id);

    inline std::string GetName() const
    {
      return name;
    }

    inline TagId GetId() const
    {
      return id;
    }
  };

  class OSMSCOUT_API TypeInfo
  {
  private:
    TypeId      id;
    std::string name;
    TagId       tag;
    std::string tagValue;
    bool        canBeNode;
    bool        canBeWay;
    bool        canBeArea;
    bool        canBeRelation;
    bool        canBeOverview;
    bool        canBeRoute;
    bool        canBeIndexed;

  public:
    TypeInfo();

    TypeInfo& SetId(TypeId id);

    TypeInfo& SetType(const std::string& name,
                      TagId tag,
                      const std::string tagValue);

    inline TypeId GetId() const
    {
      return id;
    }

    inline std::string GetName() const
    {
      return name;
    }

    inline TagId GetTag() const
    {
      return tag;
    }

    inline std::string GetTagValue() const
    {
      return tagValue;
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

    inline TypeInfo& CanBeOverview(bool canBeOverview)
    {
      this->canBeOverview=canBeOverview;

      return *this;
    }

    inline bool CanBeOverview() const
    {
      return canBeOverview;
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
  };

  class OSMSCOUT_API TypeConfig
  {
  private:
    std::list<TagInfo>                              tags;
    std::list<TypeInfo>                             types;

    TagId                                           nextTagId;
    TypeId                                          nextTypeId;

    std::map<std::string,TagInfo>                   stringToTagMap;
    std::map<TagId,std::map<std::string,TypeInfo> > tagToTypeMap;
    std::map<std::string,TypeInfo>                  nameToTypeMap;
    std::map<TypeId,TypeInfo>                       idToTypeMap;

  public:
    TagId                                           tagAdminLevel;
    TagId                                           tagBoundary;
    TagId                                           tagBuilding;
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

  public:
    TypeConfig();
    virtual ~TypeConfig();

    TypeConfig& AddTagInfo(const TagInfo& tagInfo);
    TypeConfig& AddTypeInfo(TypeInfo& typeInfo);

    const std::list<TagInfo>& GetTags() const;
    const std::list<TypeInfo>& GetTypes() const;

    TypeId GetMaxTypeId() const;

    TagId GetTagId(const char* name) const;

    const TypeInfo& GetTypeInfo(TypeId id) const;

    bool GetNodeTypeId(std::vector<Tag>& tags,
                       std::vector<Tag>::iterator& tag,
                       TypeId &typeId) const;
    bool GetWayAreaTypeId(std::vector<Tag>& tags,
                          std::vector<Tag>::iterator& wayTag,
                          TypeId &wayType,
                          std::vector<Tag>::iterator& areaTag,
                          TypeId &areaType) const;
    bool GetRelationTypeId(std::vector<Tag>& tags,
                           std::vector<Tag>::iterator& tag,
                           TypeId &typeId) const;

    TypeId GetNodeTypeId(TagId tagKey, const char* tagValue) const;
    TypeId GetWayTypeId(TagId tagKey, const char* tagValue) const;
    TypeId GetAreaTypeId(TagId tagKey, const char* tagValue) const;
    TypeId GetRelationTypeId(TagId tagKey, const char* tagValue) const;

    TypeId GetNodeTypeId(const std::string& name) const;
    TypeId GetWayTypeId(const std::string& name) const;
    TypeId GetAreaTypeId(const std::string& name) const;
    TypeId GetRelationTypeId(const std::string& name) const;

    void GetWaysWithKey(TagId tagKey, std::set<TypeId>& types) const;

    void GetRoutables(std::set<TypeId>& types) const;

    void GetIndexables(std::set<TypeId>& types) const;
  };
}

#endif
