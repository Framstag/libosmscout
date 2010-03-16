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

  const static TagId tagPrivateBase   = 5000;
  const static TagId tagIgnore        = tagPrivateBase+ 0;
  const static TagId tagName          = tagPrivateBase+ 1;
  const static TagId tagRef           = tagPrivateBase+ 2;
  const static TagId tagOneway        = tagPrivateBase+ 3;
  const static TagId tagBridge        = tagPrivateBase+ 4;
  const static TagId tagTunnel        = tagPrivateBase+ 5;
  const static TagId tagLayer         = tagPrivateBase+ 6;
  const static TagId tagBuilding      = tagPrivateBase+ 7;
  const static TagId tagPlace         = tagPrivateBase+ 8;
  const static TagId tagPlaceName     = tagPrivateBase+ 9;
  const static TagId tagBoundary      = tagPrivateBase+10;
  const static TagId tagAdminLevel    = tagPrivateBase+11;
  const static TagId tagHighway       = tagPrivateBase+12;
  const static TagId tagRestriction   = tagPrivateBase+13;
  const static TagId tagInternal      = tagPrivateBase+14;

  const static TypeId typePrivateBase = 5000;
  const static TypeId typeIgnore      = typePrivateBase+0;
  const static TypeId typeRoute       = typePrivateBase+1;

  class TagInfo
  {
  private:
    std::string name;
    TagId       id;

  public:
    TagInfo();
    TagInfo(const std::string& name,
            TagId id);

    inline std::string GetName() const
    {
      return name;
    }

    inline TagId GetId() const
    {
      return id;
    }
  };

  class TypeInfo
  {
  private:
    TypeId      id;
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
    TypeInfo(TypeId id,
             TagId tag,
             const std::string tagValue);

    inline TypeId GetId() const
    {
      return id;
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

  class TypeConfig
  {
  private:
    std::list<TagInfo>                              tags;
    std::list<TypeInfo>                             types;

    TypeId                                          maxTypeId;

    std::map<std::string,TagInfo>                   stringToTagMap;
    std::map<TagId,std::map<std::string,TypeInfo> > tagToTypeMap;

  public:
    TypeConfig();
    TypeConfig& AddTagInfo(const TagInfo& tagInfo);
    TypeConfig& AddTypeInfo(const TypeInfo& typeInfo);

    TypeId GetMaxTypeId() const;

    TagId GetTagId(const char* name) const;

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

    void GetWaysWithKey(TagId tagKey, std::set<TypeId>& types) const;

    void GetRoutables(std::set<TypeId>& types) const;

    void GetIndexables(std::set<TypeId>& types) const;

  };
}

#endif
