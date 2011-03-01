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

#include <osmscout/TypeConfig.h>

#include <cassert>

namespace osmscout {

  TagInfo::TagInfo()
   : id(0)
  {
  }

  TagInfo::TagInfo(const std::string& name)
   : id(0),
     name(name)
  {
    // no code
  }

  TagInfo& TagInfo::SetId(TagId id)
  {
    this->id=id;

    return *this;
  }

  TypeInfo::TypeInfo()
   : id(0),
     canBeNode(false),
     canBeWay(false),
     canBeArea(false),
     canBeRelation(false),
     canBeRoute(false),
     canBeIndexed(false)
  {
    // no code
  }

  TypeInfo& TypeInfo::SetId(TypeId id)
  {
    this->id=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetType(const std::string& name,
                              TagId tag,
                              const std::string tagValue)
  {
    this->name=name;
    this->tag=tag;
    this->tagValue=tagValue;

    return *this;
  }

  TypeConfig::TypeConfig()
   : nextTagId(0),
     nextTypeId(0)
  {
    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for tagIgnore
    AddTagInfo(TagInfo(""));

    AddTagInfo(TagInfo("admin_level"));
    AddTagInfo(TagInfo("boundary"));
    AddTagInfo(TagInfo("building"));
    AddTagInfo(TagInfo("bridge"));
    AddTagInfo(TagInfo("highway"));
    AddTagInfo(TagInfo("layer"));
    AddTagInfo(TagInfo("name"));
    AddTagInfo(TagInfo("natural"));
    AddTagInfo(TagInfo("oneway"));
    AddTagInfo(TagInfo("place"));
    AddTagInfo(TagInfo("place_name"));
    AddTagInfo(TagInfo("ref"));
    AddTagInfo(TagInfo("restriction"));
    AddTagInfo(TagInfo("tunnel"));
    AddTagInfo(TagInfo("type"));
    AddTagInfo(TagInfo("width"));

    TypeInfo ignore;
    TypeInfo route;

    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for typeIgnore
    ignore.SetType("",
                   0,"");

    AddTypeInfo(ignore);

    route.SetType("_route",
                  0,"")
         .CanBeWay(true);

    AddTypeInfo(route);

    tagAdminLevel=GetTagId("admin_level");
    tagBoundary=GetTagId("boundary");
    tagBuilding=GetTagId("building");
    tagBridge=GetTagId("bridge");
    tagLayer=GetTagId("layer");
    tagName=GetTagId("name");
    tagOneway=GetTagId("oneway");
    tagPlace=GetTagId("place");
    tagPlaceName=GetTagId("place_name");
    tagRef=GetTagId("ref");
    tagTunnel=GetTagId("tunnel");
    tagType=GetTagId("type");
    tagWidth=GetTagId("width");

    assert(tagAdminLevel!=tagIgnore);
    assert(tagBoundary!=tagIgnore);
    assert(tagBuilding!=tagIgnore);
    assert(tagBridge!=tagIgnore);
    assert(tagLayer!=tagIgnore);
    assert(tagName!=tagIgnore);
    assert(tagOneway!=tagIgnore);
    assert(tagPlace!=tagIgnore);
    assert(tagPlaceName!=tagIgnore);
    assert(tagRef!=tagIgnore);
    assert(tagTunnel!=tagIgnore);
    assert(tagType!=tagIgnore);
    assert(tagWidth!=tagIgnore);
  }

  TypeConfig::~TypeConfig()
  {
    // no code
  }

  const std::vector<TagInfo>& TypeConfig::GetTags() const
  {
    return tags;
  }

  const std::vector<TypeInfo>& TypeConfig::GetTypes() const
  {
    return types;
  }

  TypeConfig& TypeConfig::AddTagInfo(const TagInfo& tagInfo)
  {
    TagInfo ti(tagInfo);

    if (stringToTagMap.find(ti.GetName())!=stringToTagMap.end()) {
      // Tag was already (internally?) defined, we ignore the second definition
      return *this;
    }

    if (ti.GetId()==0) {
      ti.SetId(nextTagId);

      nextTagId++;
    }
    else {
      nextTagId=std::max(nextTagId,(TagId)(ti.GetId()+1));
    }

    tags.push_back(ti);
    stringToTagMap[ti.GetName()]=ti;

    return *this;
  }

  TypeConfig& TypeConfig::AddTypeInfo(TypeInfo& typeInfo)
  {
    if (typeInfo.GetId()==0) {
      typeInfo.SetId(nextTypeId);

      nextTypeId++;
    }
    else {
      nextTypeId=std::max(nextTypeId,(TypeId)(typeInfo.GetId()+1));
    }

    if (nameToTypeMap.find(typeInfo.GetName())==nameToTypeMap.end()) {
      types.push_back(typeInfo);
      nameToTypeMap[typeInfo.GetName()]=typeInfo;
    }

    if (idToTypeMap.find(typeInfo.GetId())==idToTypeMap.end()) {
      idToTypeMap[typeInfo.GetId()]=typeInfo;
    }

    if (typeInfo.GetTag()>=tagToTypeMaps.size()) {
      tagToTypeMaps.resize(typeInfo.GetTag()+1);
    }

    tagToTypeMaps[typeInfo.GetTag()][typeInfo.GetTagValue()]=typeInfo;

    return *this;
  }

  TypeId TypeConfig::GetMaxTypeId() const
  {
    if (nextTypeId==0) {
      return 0;
    }
    else {
      return nextTypeId-1;
    }
  }

  TagId TypeConfig::GetTagId(const char* name) const
  {
    std::map<std::string,TagInfo>::const_iterator iter=stringToTagMap.find(name);

    if (iter!=stringToTagMap.end()) {
      return iter->second.GetId();
    }
    else {
      return tagIgnore;
    }
  }

  const TypeInfo& TypeConfig::GetTypeInfo(TypeId id) const
  {
    std::map<TypeId,TypeInfo>::const_iterator iter=idToTypeMap.find(id);

    assert(iter!=idToTypeMap.end());

    return iter->second;
  }

  bool TypeConfig::GetNodeTypeId(std::vector<Tag> &tags,
                                 std::vector<Tag>::iterator& tag,
                                 TypeId &type) const
  {
    type=typeIgnore;

    for (tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      type=GetNodeTypeId(tag->key,tag->value.c_str());

      if (type!=typeIgnore) {
        return true;
      }
    }

    return false;
  }

  bool TypeConfig::GetWayAreaTypeId(std::vector<Tag>& tags,
                                    std::vector<Tag>::iterator& wayTag,
                                    TypeId &wayType,
                                    std::vector<Tag>::iterator& areaTag,
                                    TypeId &areaType) const
  {
    wayType=typeIgnore;
    areaType=typeIgnore;
    wayTag=tags.end();
    areaTag=tags.end();

    for (std::vector<Tag>::iterator tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      if (wayType==typeIgnore) {
        wayType=GetWayTypeId(tag->key,tag->value.c_str());
        if (wayType!=typeIgnore) {
          wayTag=tag;
        }
      }

      if (areaType==typeIgnore) {
        areaType=GetAreaTypeId(tag->key,tag->value.c_str());
        if (areaType!=typeIgnore) {
          areaTag=tag;
        }
      }

      if (wayType!=typeIgnore && areaType!=typeIgnore) {
        return true;
      }
    }

    return wayType!=typeIgnore || areaType!=typeIgnore;
  }

  bool TypeConfig::GetRelationTypeId(std::vector<Tag> &tags,
                                     std::vector<Tag>::iterator& tag,
                                     TypeId &type) const
  {
    std::string relType;

    for (tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      if (tag->key==tagType) {
        relType=tag->value;
        break;
      }
    }

    type=typeIgnore;

    type=typeIgnore;

    for (tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      type=GetRelationTypeId(tag->key,tag->value.c_str());

      if (type!=typeIgnore) {
        return true;
      }
    }

    return false;
  }

  TypeId TypeConfig::GetNodeTypeId(TagId tagKey, const char* tagValue) const
  {
    if (tagKey>=tagToTypeMaps.size()) {
      return typeIgnore;
    }

    std::map<std::string,TypeInfo>::const_iterator iter=tagToTypeMaps[tagKey].find(tagValue);

    if (iter!=tagToTypeMaps[tagKey].end() &&
        iter->second.CanBeNode()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetWayTypeId(TagId tagKey, const char* tagValue) const
  {
    if (tagKey>=tagToTypeMaps.size()) {
      return typeIgnore;
    }

    std::map<std::string,TypeInfo>::const_iterator iter=tagToTypeMaps[tagKey].find(tagValue);

    if (iter!=tagToTypeMaps[tagKey].end() &&
        iter->second.CanBeWay()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetAreaTypeId(TagId tagKey, const char* tagValue) const
  {
    if (tagKey>=tagToTypeMaps.size()) {
      return typeIgnore;
    }

    std::map<std::string,TypeInfo>::const_iterator iter=tagToTypeMaps[tagKey].find(tagValue);

    if (iter!=tagToTypeMaps[tagKey].end() &&
        iter->second.CanBeArea()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetRelationTypeId(TagId tagKey, const char* tagValue) const
  {
    if (tagKey>=tagToTypeMaps.size()) {
      return typeIgnore;
    }

    std::map<std::string,TypeInfo>::const_iterator iter=tagToTypeMaps[tagKey].find(tagValue);

    if (iter!=tagToTypeMaps[tagKey].end() &&
        iter->second.CanBeRelation()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetNodeTypeId(const std::string& name) const
  {
    std::map<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end() &&
        iter->second.CanBeNode()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetWayTypeId(const std::string& name) const
  {
    std::map<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end() &&
        iter->second.CanBeWay()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetAreaTypeId(const std::string& name) const
  {
    std::map<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end() &&
        iter->second.CanBeArea()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetRelationTypeId(const std::string& name) const
  {
    std::map<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end() &&
        iter->second.CanBeRelation()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  void TypeConfig::GetWaysWithKey(TagId tagKey, std::set<TypeId>& types) const
  {
    for (std::map<std::string,TypeInfo>::const_iterator iter=tagToTypeMaps[tagKey].begin();
         iter!=tagToTypeMaps[tagKey].end();
         ++iter) {
      if (iter->second.CanBeWay()) {
        types.insert(iter->second.GetId());
      }
    }
  }

  void TypeConfig::GetRoutables(std::set<TypeId>& types) const
  {
    types.clear();

    for (std::vector<TypeInfo>::const_iterator type=this->types.begin();
         type!=this->types.end();
         ++type) {
      if (type->CanBeRoute()) {
        types.insert(type->GetId());
      }
    }
  }

  void TypeConfig::GetIndexables(std::set<TypeId>& types) const
  {
    types.clear();

    for (std::vector<TypeInfo>::const_iterator type=this->types.begin();
         type!=this->types.end();
         ++type) {
      if (type->CanBeIndexed()) {
        types.insert(type->GetId());
      }
    }
  }
}
