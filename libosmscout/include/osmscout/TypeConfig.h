#ifndef OSMSCOUT_TYPECONFIG_H
#define OSMSCOUT_TYPECONFIG_H

/*
  Import/TravelJinni - Openstreetmap offline viewer
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

typedef uint32_t Id;
typedef uint32_t Page;
typedef uint16_t TypeId;
typedef uint16_t NodeCount;

enum Mag {
 magWorld     =              1,
 magState     =             32,
 magStateOver =             64,
 magCounty    =            128,
 magRegion    =            256,
 magProximity =            512,
 magCityOver  =           1024,
 magCity      =         2*1024,
 magDetail    =     2*2*2*1024,
 magClose     =   2*2*2*2*1024,
 magVeryClose = 2*2*2*2*2*1024
};

const static TagId tagIgnore     = 32000;
const static TagId tagName       = 32001;
const static TagId tagRef        = 32002;
const static TagId tagOneway     = 32003;
const static TagId tagBridge     = 32004;
const static TagId tagTunnel     = 32005;
const static TagId tagLayer      = 32006;
const static TagId tagBuilding   = 32007;
const static TagId tagPlace      = 32008;
const static TagId tagPlaceName  = 32009;
const static TagId tagBoundary   = 32010;
const static TagId tagAdminLevel = 32011;
const static TagId tagHighway    = 32012;
const static TagId tagInternal   = 32013;

const static TypeId typeIgnore   = 32000;
const static TypeId typeRoute    = 32001;

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
  bool        canBeRoute;

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

  inline TypeInfo& CanBeRoute(bool canBeRoute)
  {
    this->canBeRoute=canBeRoute;

    return *this;
  }

  inline bool CanBeRoute() const
  {
    return canBeRoute;
  }
};

class TypeConfig
{
private:
  std::list<TagInfo>                              tags;
  std::list<TypeInfo>                             types;

  std::map<std::string,TagInfo>                   stringToTagMap;
  std::map<TagId,std::map<std::string,TypeInfo> > tagToTypeMap;

public:
  TypeConfig();
  TypeConfig& AddTagInfo(const TagInfo& tagInfo);
  TypeConfig& AddTypeInfo(const TypeInfo& typeInfo);

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
};

#endif
