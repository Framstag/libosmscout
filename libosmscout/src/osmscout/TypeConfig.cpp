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

#include <osmscout/TypeConfig.h>

TagInfo::TagInfo()
 : name("ignore"),
   id(tagIgnore)
{
}

TagInfo::TagInfo(const std::string& name,
                 TagId id)
 : name(name),
   id(id)
{
  // no code
}

TypeInfo::TypeInfo()
 : id(typeIgnore)
{
}

TypeInfo::TypeInfo(TypeId id,
                   TagId tag,
                   const std::string tagValue)
 : id(id),
   tag(tag),
   tagValue(tagValue)
{
  // no code
}

TypeConfig::TypeConfig()
{
  AddTagInfo(TagInfo("ignore",tagIgnore));
  AddTagInfo(TagInfo("name",tagName));
  AddTagInfo(TagInfo("ref",tagRef));
  AddTagInfo(TagInfo("oneway",tagOneway));
  AddTagInfo(TagInfo("bridge",tagBridge));
  AddTagInfo(TagInfo("tunnel",tagTunnel));
  AddTagInfo(TagInfo("layer",tagLayer));
  AddTagInfo(TagInfo("building",tagBuilding));
  AddTagInfo(TagInfo("place",tagPlace));
  AddTagInfo(TagInfo("place_name",tagPlaceName));
  AddTagInfo(TagInfo("boundary",tagBoundary));
  AddTagInfo(TagInfo("admin_level",tagAdminLevel));
  AddTagInfo(TagInfo("highway",tagHighway));
  AddTagInfo(TagInfo("restriction",tagRestriction));
  AddTagInfo(TagInfo("internal",tagInternal));

  AddTypeInfo(TypeInfo(typeRoute,tagInternal,"route").CanBeWay(true));
}

TypeConfig& TypeConfig::AddTagInfo(const TagInfo& tagInfo)
{
  tags.push_back(tagInfo);
  stringToTagMap[tagInfo.GetName()]=tagInfo;

  return *this;
}

TypeConfig& TypeConfig::AddTypeInfo(const TypeInfo& typeInfo)
{
  types.push_back(typeInfo);

  tagToTypeMap[typeInfo.GetTag()][typeInfo.GetTagValue()]=typeInfo;

  return *this;
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

bool TypeConfig::GetNodeTypeId(std::vector<Tag> &tags,
                               std::vector<Tag>::iterator& tag,
                               TypeId &type) const
{
  type=typeIgnore;

  for (std::list<TypeInfo>::const_iterator t=types.begin();
       t!=types.end();
       ++t) {
    for (tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      if (tag->key==t->GetTag() &&
          tag->value==t->GetTagValue() &&
          t->CanBeNode()) {
        type=t->GetId();
        return true;
      }
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

  for (std::list<TypeInfo>::const_iterator type=types.begin();
       type!=types.end();
       ++type) {
    for (std::vector<Tag>::iterator tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      if (wayType==typeIgnore &&
          tag->key==type->GetTag() &&
          tag->value==type->GetTagValue() &&
          type->CanBeWay()) {
        wayTag=tag;
        wayType=type->GetId();
      }

      if (areaType==typeIgnore &&
          tag->key==type->GetTag() &&
          tag->value==type->GetTagValue() &&
          type->CanBeArea()) {
        areaTag=tag;
        areaType=type->GetId();
      }

      if (wayType!=typeIgnore && areaType!=typeIgnore) {
        return true;
      }
    }
  }

  return wayType!=typeIgnore || areaType!=typeIgnore;
}

bool TypeConfig::GetRelationTypeId(std::vector<Tag> &tags,
                                   std::vector<Tag>::iterator& tag,
                                   TypeId &type) const
{
  type=typeIgnore;

  for (std::list<TypeInfo>::const_iterator t=types.begin();
       t!=types.end();
       ++t) {
    for (tag=tags.begin();
         tag!=tags.end();
         ++tag) {
      if (tag->key==t->GetTag() &&
          tag->value==t->GetTagValue() &&
          t->CanBeRelation()) {
        type=t->GetId();
        return true;
      }
    }
  }

  return false;
}

TypeId TypeConfig::GetNodeTypeId(TagId tagKey, const char* tagValue) const
{
  std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

  if (iter!=tagToTypeMap.end()) {
    std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.find(tagValue);

    if (iter2!=iter->second.end() && iter2->second.CanBeNode()) {
      return iter2->second.GetId();
    }
  }

  return typeIgnore;
}

TypeId TypeConfig::GetWayTypeId(TagId tagKey, const char* tagValue) const
{
  std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

  if (iter!=tagToTypeMap.end()) {
    std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.find(tagValue);

    if (iter2!=iter->second.end() && iter2->second.CanBeWay()) {
      return iter2->second.GetId();
    }
  }

  return typeIgnore;
}

TypeId TypeConfig::GetAreaTypeId(TagId tagKey, const char* tagValue) const
{
  std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

  if (iter!=tagToTypeMap.end()) {
    std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.find(tagValue);

    if (iter2!=iter->second.end() && iter2->second.CanBeArea()) {
      return iter2->second.GetId();
    }
  }

  return typeIgnore;
}

TypeId TypeConfig::GetRelationTypeId(TagId tagKey, const char* tagValue) const
{
  std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

  if (iter!=tagToTypeMap.end()) {
    std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.find(tagValue);

    if (iter2!=iter->second.end() && iter2->second.CanBeRelation()) {
      return iter2->second.GetId();
    }
  }

  return typeIgnore;
}

void TypeConfig::GetWaysWithKey(TagId tagKey, std::set<TypeId>& types) const
{
  std::map<TagId,std::map<std::string,TypeInfo> >::const_iterator iter=tagToTypeMap.find(tagKey);

  if (iter!=tagToTypeMap.end()) {
    for (std::map<std::string,TypeInfo>::const_iterator iter2=iter->second.begin();
         iter2!=iter->second.end();
         ++iter2) {
      if (iter2!=iter->second.end() && iter2->second.CanBeWay()) {
        types.insert(iter2->second.GetId());
      }
    }
  }
}

void TypeConfig::GetRoutables(std::set<TypeId>& types) const
{
  types.clear();

  for (std::list<TypeInfo>::const_iterator type=this->types.begin();
       type!=this->types.end();
       ++type) {
    if (type->CanBeRoute()) {
      types.insert(type->GetId());
    }
  }
}

