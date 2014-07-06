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

#include <osmscout/system/Assert.h>

#include <osmscout/ost/Parser.h>
#include <osmscout/ost/Scanner.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/String.h>

#include <iostream>
namespace osmscout {

  TagCondition::~TagCondition()
  {
    // no code
  }

  TagNotCondition::TagNotCondition(TagCondition* condition)
  : condition(condition)
  {
    // no code
  }

  bool TagNotCondition::Evaluate(const std::map<TagId,std::string>& tagMap) const
  {
    return !condition->Evaluate(tagMap);
  }

  TagBoolCondition::TagBoolCondition(Type type)
  : type(type)
  {
    // no code
  }

  void TagBoolCondition::AddCondition(TagCondition* condition)
  {
    conditions.push_back(condition);
  }

  bool TagBoolCondition::Evaluate(const std::map<TagId,std::string>& tagMap) const
  {
    switch (type) {
    case boolAnd:
      for (std::list<TagConditionRef>::const_iterator condition=conditions.begin();
           condition!=conditions.end();
           ++condition) {
        if (!(*condition)->Evaluate(tagMap)) {
          return false;
        }
      }

      return true;
    case boolOr:
      for (std::list<TagConditionRef>::const_iterator condition=conditions.begin();
           condition!=conditions.end();
           ++condition) {
        if ((*condition)->Evaluate(tagMap)) {
          return true;
        }
      }

      return false;
    default:
      assert(false);

      return false;
    }
  }

  TagExistsCondition::TagExistsCondition(TagId tag)
  : tag(tag)
  {
    // no code
  }

  bool TagExistsCondition::Evaluate(const std::map<TagId,std::string>& tagMap) const
  {
    return tagMap.find(tag)!=tagMap.end();
  }

  TagBinaryCondition::TagBinaryCondition(TagId tag,
                                         BinaryOperator binaryOperator,
                                         const std::string& tagValue)
  : tag(tag),
    binaryOperator(binaryOperator),
    valueType(string),
    tagStringValue(tagValue),
    tagSizeValue(0)
  {
    // no code
  }

  TagBinaryCondition::TagBinaryCondition(TagId tag,
                                         BinaryOperator binaryOperator,
                                         const size_t& tagValue)
  : tag(tag),
    binaryOperator(binaryOperator),
    valueType(sizet),
    tagSizeValue(tagValue)
  {
    // no code
  }

  bool TagBinaryCondition::Evaluate(const std::map<TagId,std::string>& tagMap) const
  {
    std::map<TagId,std::string>::const_iterator t;

    t=tagMap.find(tag);

    if (t==tagMap.end()) {
      return false;
    }

    if (valueType==string) {
      switch (binaryOperator) {
      case  operatorLess:
        return t->second<tagStringValue;
      case  operatorLessEqual:
        return t->second<=tagStringValue;
      case  operatorEqual:
        return t->second==tagStringValue;
      case operatorNotEqual:
        return t->second!=tagStringValue;
      case operatorGreaterEqual:
        return t->second>=tagStringValue;
      case  operatorGreater:
        return t->second>tagStringValue;
      default:
        assert(false);

        return false;
      }
    }
    else if (valueType==sizet) {
      size_t value;

      if (!StringToNumber(t->second,
                          value)) {
        return false;
      }

      switch (binaryOperator) {
      case  operatorLess:
        return value<tagSizeValue;
      case  operatorLessEqual:
        return value<=tagSizeValue;
      case  operatorEqual:
        return value==tagSizeValue;
      case operatorNotEqual:
        return value!=tagSizeValue;
      case operatorGreaterEqual:
        return value>=tagSizeValue;
      case  operatorGreater:
        return value>tagSizeValue;
      default:
        assert(false);

        return false;
      }
    }
    else {
      assert(false);

      return false;
    }
  }

  TagIsInCondition::TagIsInCondition(TagId tag)
  : tag(tag)
  {
    // no code
  }

  void TagIsInCondition::AddTagValue(const std::string& tagValue)
  {
    tagValues.insert(tagValue);
  }

  bool TagIsInCondition::Evaluate(const std::map<TagId,std::string>& tagMap) const
  {
    std::map<TagId,std::string>::const_iterator t;

    t=tagMap.find(tag);

    if (t==tagMap.end()) {
      return false;
    }

    return tagValues.find(t->second)!=tagValues.end();
  }

  TagInfo::TagInfo()
   : id(0),
     internalOnly(true)
  {
  }

  TagInfo::TagInfo(const std::string& name,
                   bool internalOnly)
   : id(0),
     name(name),
     internalOnly(internalOnly)
  {
    // no code
  }

  TagInfo& TagInfo::SetId(TagId id)
  {
    this->id=id;

    return *this;
  }

  TagInfo& TagInfo::SetToExternal()
  {
    internalOnly=false;

    return *this;
  }

  Feature::Feature()
  {
    // no code
  }

  Feature::~Feature()
  {
    // no code
  }

  const char* const NameFeature::NAME = "Name";

  std::string NameFeature::GetName() const
  {
    return NAME;
  }

  const char* const RefFeature::NAME = "Ref";

  std::string RefFeature::GetName() const
  {
    return NAME;
  }

  TypeInfo::TypeInfo()
   : id(0),
     canBeNode(false),
     canBeWay(false),
     canBeArea(false),
     canBeRelation(false),
     canRouteFoot(false),
     canRouteBicycle(false),
     canRouteCar(false),
     indexAsLocation(false),
     indexAsRegion(false),
     indexAsPOI(false),
     optimizeLowZoom(false),
     multipolygon(false),
     pinWay(false),
     ignoreSeaLand(false),
     ignore(false)
  {
    // no code
  }

  TypeInfo::~TypeInfo()
  {
    // no code
  }

  TypeInfo& TypeInfo::SetId(TypeId id)
  {
    this->id=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetType(const std::string& name)
  {
    this->name=name;

    return *this;
  }

  TypeInfo& TypeInfo::AddCondition(unsigned char types,
                                   TagCondition* condition)
  {
    TypeCondition typeCondition;

    if (types & typeNode) {
      canBeNode=true;
    }

    if (types & typeWay) {
      canBeWay=true;
    }

    if (types & typeArea) {
      canBeArea=true;
    }

    if (types & typeRelation) {
      canBeRelation=true;
    }

    typeCondition.types=types;
    typeCondition.condition=condition;

    conditions.push_back(typeCondition);

    return *this;
  }

  TypeInfo& TypeInfo::AddFeature(const FeatureRef& feature)
  {
    assert(feature.Valid());
    assert(featureSet.find(feature->GetName())==featureSet.end());

    features.push_back(feature);
    featureSet.insert(feature->GetName());

    return *this;
  }

  bool TypeInfo::HasFeature(const std::string& featureName) const
  {
    return featureSet.find(featureName)!=featureSet.end();
  }

  TypeConfig::TypeConfig()
   : nextTagId(0),
     nextTypeId(0)
  {
    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for tagIgnore
    RegisterTagForInternalUse("");

    RegisterTagForExternalUse("name");
    RegisterTagForExternalUse("ref");
    RegisterTagForExternalUse("bridge");
    RegisterTagForExternalUse("tunnel");
    RegisterTagForExternalUse("layer");
    RegisterTagForExternalUse("width");
    RegisterTagForExternalUse("oneway");
    RegisterTagForExternalUse("addr:housenumber");
    RegisterTagForExternalUse("addr:street");
    RegisterTagForExternalUse("junction");
    RegisterTagForExternalUse("maxspeed");
    RegisterTagForExternalUse("surface");
    RegisterTagForExternalUse("tracktype");
    RegisterTagForExternalUse("admin_level");

    RegisterTagForExternalUse("access");
    RegisterTagForExternalUse("access:foward");
    RegisterTagForExternalUse("access:backward");

    RegisterTagForExternalUse("access:foot");
    RegisterTagForExternalUse("access:foot:foward");
    RegisterTagForExternalUse("access:foot:backward");

    RegisterTagForExternalUse("access:bicycle");
    RegisterTagForExternalUse("access:bicycle:foward");
    RegisterTagForExternalUse("access:bicycle:backward");

    RegisterTagForExternalUse("access:motor_vehicle");
    RegisterTagForExternalUse("access:motor_vehicle:foward");
    RegisterTagForExternalUse("access:motor_vehicle:backward");

    RegisterTagForExternalUse("access:motorcar");
    RegisterTagForExternalUse("access:motorcar:foward");
    RegisterTagForExternalUse("access:motorcar:backward");

    RegisterTagForInternalUse("area");
    RegisterTagForInternalUse("natural");
    RegisterTagForInternalUse("type");
    RegisterTagForInternalUse("restriction");

    RegisterFeature(new NameFeature());
    RegisterFeature(new RefFeature());

    TypeInfo ignore;
    TypeInfo route;
    TypeInfo tileLand;
    TypeInfo tileSea;
    TypeInfo tileCoast;
    TypeInfo tileUnknown;
    TypeInfo tileCoastline;

    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for typeIgnore
    ignore.SetType("");

    AddTypeInfo(ignore);

    // Internal type for showing routes
    route.SetType("_route")
         .CanBeWay(true);

    // Internal types for the land/sea/coast tiles building the base layer for map drawing
    tileLand.SetType("_tile_land")
            .CanBeArea(true);
    tileSea.SetType("_tile_sea")
           .CanBeArea(true);
    tileCoast.SetType("_tile_coast")
             .CanBeArea(true);
    tileUnknown.SetType("_tile_unknown")
               .CanBeArea(true);
    tileCoastline.SetType("_tile_coastline")
               .CanBeWay(true);

    AddTypeInfo(route);
    AddTypeInfo(tileLand);
    AddTypeInfo(tileSea);
    AddTypeInfo(tileCoast);
    AddTypeInfo(tileUnknown);
    AddTypeInfo(tileCoastline);

    typeTileLand=GetTypeId("_tile_land");
    typeTileSea=GetTypeId("_tile_sea");
    typeTileCoast=GetTypeId("_tile_coast");
    typeTileUnknown=GetTypeId("_tile_unknown");
    typeTileCoastline=GetTypeId("_tile_coastline");

    tagRef=GetTagId("ref");
    tagBridge=GetTagId("bridge");
    tagTunnel=GetTagId("tunnel");
    tagLayer=GetTagId("layer");
    tagWidth=GetTagId("width");
    tagOneway=GetTagId("oneway");
    tagHouseNr=GetTagId("addr:housenumber");
    tagJunction=GetTagId("junction");
    tagMaxSpeed=GetTagId("maxspeed");
    tagSurface=GetTagId("surface");
    tagTracktype=GetTagId("tracktype");
    tagAdminLevel=GetTagId("admin_level");

    tagAccess=GetTagId("access");
    tagAccessForward=GetTagId("access:foward");
    tagAccessBackward=GetTagId("access:backward");

    tagAccessFoot=GetTagId("access:foot");
    tagAccessFootForward=GetTagId("access:foot:foward");
    tagAccessFootBackward=GetTagId("access:foot:backward");

    tagAccessBicycle=GetTagId("access:bicycle");
    tagAccessBicycleForward=GetTagId("access:bicycle:foward");
    tagAccessBicycleBackward=GetTagId("access:bicycle:backward");

    tagAccessMotorVehicle=GetTagId("access:motor_vehicle");
    tagAccessMotorVehicleForward=GetTagId("access:motor_vehicle:foward");
    tagAccessMotorVehicleBackward=GetTagId("access:motor_vehicle:backward");

    tagAccessMotorcar=GetTagId("access:motorcar");
    tagAccessMotorcarForward=GetTagId("access:motorcar:foward");
    tagAccessMotorcarBackward=GetTagId("access:motorcar:backward");

    tagAddrStreet=GetTagId("addr:street");

    tagArea=GetTagId("area");
    tagNatural=GetTagId("natural");
    tagType=GetTagId("type");
    tagRestriction=GetTagId("restriction");

    assert(tagRef!=tagIgnore);
    assert(tagBridge!=tagIgnore);
    assert(tagTunnel!=tagIgnore);
    assert(tagLayer!=tagIgnore);
    assert(tagWidth!=tagIgnore);
    assert(tagOneway!=tagIgnore);
    assert(tagHouseNr!=tagIgnore);
    assert(tagJunction!=tagIgnore);
    assert(tagMaxSpeed!=tagIgnore);
    assert(tagSurface!=tagIgnore);
    assert(tagTracktype!=tagIgnore);
    assert(tagAdminLevel!=tagIgnore);

    assert(tagAccess!=tagIgnore);
    assert(tagAccessForward!=tagIgnore);
    assert(tagAccessBackward!=tagIgnore);

    assert(tagAccessFoot!=tagIgnore);
    assert(tagAccessFootForward!=tagIgnore);
    assert(tagAccessFootBackward!=tagIgnore);

    assert(tagAccessBicycle!=tagIgnore);
    assert(tagAccessBicycleForward!=tagIgnore);
    assert(tagAccessBicycleBackward!=tagIgnore);

    assert(tagAccessMotorVehicle!=tagIgnore);
    assert(tagAccessMotorVehicleForward!=tagIgnore);
    assert(tagAccessMotorVehicleBackward!=tagIgnore);

    assert(tagAccessMotorcar!=tagIgnore);
    assert(tagAccessMotorcarForward!=tagIgnore);
    assert(tagAccessMotorcarBackward!=tagIgnore);

    assert(tagAddrStreet!=tagIgnore);

    assert(tagArea!=tagIgnore);
    assert(tagNatural!=tagIgnore);
    assert(tagType!=tagIgnore);
    assert(tagRestriction!=tagIgnore);
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

  TagId TypeConfig::RegisterTagForInternalUse(const std::string& tagName)
  {
    OSMSCOUT_HASHMAP<std::string,TagId>::const_iterator mapping=stringToTagMap.find(tagName);

    if (mapping!=stringToTagMap.end()) {
      return mapping->second;
    }

    TagInfo tagInfo(tagName,true);

    if (tagInfo.GetId()==0) {
      tagInfo.SetId(nextTagId);

      nextTagId++;
    }
    else {
      nextTagId=std::max(nextTagId,(TagId)(tagInfo.GetId()+1));
    }

    tags.push_back(tagInfo);
    stringToTagMap[tagInfo.GetName()]=tagInfo.GetId();

    return tagInfo.GetId();
  }

  TagId TypeConfig::RegisterTagForExternalUse(const std::string& tagName)
  {
    OSMSCOUT_HASHMAP<std::string,TagId>::const_iterator mapping=stringToTagMap.find(tagName);

    if (mapping!=stringToTagMap.end()) {
      tags[mapping->second].SetToExternal();

      return mapping->second;
    }

    TagInfo tagInfo(tagName,false);

    if (tagInfo.GetId()==0) {
      tagInfo.SetId(nextTagId);

      nextTagId++;
    }
    else {
      nextTagId=std::max(nextTagId,(TagId)(tagInfo.GetId()+1));
    }

    tags.push_back(tagInfo);
    stringToTagMap[tagInfo.GetName()]=tagInfo.GetId();

    return tagInfo.GetId();
  }

  TagId TypeConfig::RegisterNameTag(const std::string& tagName, uint32_t priority)
  {
    TagId tagId=RegisterTagForExternalUse(tagName);

    nameTagIdToPrioMap.insert(std::make_pair(tagId,priority));

    return tagId;
  }

  TagId TypeConfig::RegisterNameAltTag(const std::string& tagName, uint32_t priority)
  {
    TagId tagId=RegisterTagForExternalUse(tagName);

    nameAltTagIdToPrioMap.insert(std::make_pair(tagId,priority));

    return tagId;
  }

  void TypeConfig::RegisterFeature(const FeatureRef& feature)
  {
    assert(feature.Valid());
    assert(!feature->GetName().empty());

    nameToFeatureMap[feature->GetName()]=feature;
  }

  FeatureRef TypeConfig::GetFeature(const std::string& name) const
  {
    OSMSCOUT_HASHMAP<std::string,FeatureRef>::const_iterator feature=nameToFeatureMap.find(name);

    if (feature!=nameToFeatureMap.end()) {
      return feature->second;
    }
    else {
      return NULL;
    }
  }

  TypeConfig& TypeConfig::AddTypeInfo(TypeInfo& typeInfo)
  {
    if (nameToTypeMap.find(typeInfo.GetName())!=nameToTypeMap.end()) {
      return *this;
    }

    if (typeInfo.GetId()==0) {
      typeInfo.SetId(nextTypeId);

      nextTypeId++;
    }
    else {
      nextTypeId=std::max(nextTypeId,(TypeId)(typeInfo.GetId()+1));
    }

    //std::cout << "Type: " << typeInfo.GetId() << " " << typeInfo.GetName() << std::endl;

    types.push_back(typeInfo);
    nameToTypeMap[typeInfo.GetName()]=typeInfo;

    idToTypeMap[typeInfo.GetId()]=typeInfo;

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
    OSMSCOUT_HASHMAP<std::string,TagId>::const_iterator iter=stringToTagMap.find(name);

    if (iter!=stringToTagMap.end()) {
      return iter->second;
    }
    else {
      return tagIgnore;
    }
  }

  const TagInfo& TypeConfig::GetTagInfo(TagId id) const
  {
    assert(id<tags.size());

    return tags[id];
  }

  const TypeInfo& TypeConfig::GetTypeInfo(TypeId id) const
  {
    assert(id<types.size());

    return types[id];
  }

  void TypeConfig::ResolveTags(const std::map<TagId,std::string>& map,
                               std::vector<Tag>& tags) const
  {
    tags.clear();

    for (std::map<TagId,std::string>::const_iterator t=map.begin();
         t!=map.end();
         ++t) {
      if (GetTagInfo(t->first).IsInternalOnly()) {
        continue;
      }

      Tag tag;

      tag.key=t->first;
      tag.value=t->second;

      tags.push_back(tag);
    }
  }

  bool TypeConfig::IsNameTag(TagId tag, uint32_t& priority) const
  {
    if (nameTagIdToPrioMap.empty()) {
      return false;
    }

    OSMSCOUT_HASHMAP<TagId,uint32_t>::const_iterator entry=nameTagIdToPrioMap.find(tag);

    if (entry==nameTagIdToPrioMap.end()) {
      return false;
    }

    priority=entry->second;

    return true;
  }

  bool TypeConfig::IsNameAltTag(TagId tag, uint32_t& priority) const
  {
    if (nameAltTagIdToPrioMap.empty()) {
      return false;
    }

    OSMSCOUT_HASHMAP<TagId,uint32_t>::const_iterator entry=nameAltTagIdToPrioMap.find(tag);

    if (entry==nameAltTagIdToPrioMap.end()) {
      return false;
    }

    priority=entry->second;

    return true;
  }

  bool TypeConfig::GetNodeTypeId(const std::map<TagId,std::string>& tagMap,
                                 TypeId &typeId) const
  {
    typeId=typeIgnore;

    if (tagMap.empty()) {
      return false;
    }

    for (size_t i=0; i<types.size(); i++) {
      if (!types[i].HasConditions() ||
          !types[i].CanBeNode()) {
        continue;
      }

      for (std::list<TypeInfo::TypeCondition>::const_iterator cond=types[i].GetConditions().begin();
           cond!=types[i].GetConditions().end();
           ++cond) {
        if (!(cond->types & TypeInfo::typeNode)) {
          continue;
        }

        if (cond->condition->Evaluate(tagMap)) {
          typeId=types[i].GetId();
          return true;
        }
      }
    }

    return false;
  }

  bool TypeConfig::GetWayAreaTypeId(const std::map<TagId,std::string>& tagMap,
                                    TypeId &wayType,
                                    TypeId &areaType) const
  {
    wayType=typeIgnore;
    areaType=typeIgnore;

    if (tagMap.empty()) {
      return false;
    }

    for (size_t i=0; i<types.size(); i++) {
      if (!((types[i].CanBeWay() ||
             types[i].CanBeArea()) &&
             types[i].HasConditions())) {
        continue;
      }

      for (std::list<TypeInfo::TypeCondition>::const_iterator cond=types[i].GetConditions().begin();
           cond!=types[i].GetConditions().end();
           ++cond) {
        if (!((cond->types & TypeInfo::typeWay) || (cond->types & TypeInfo::typeArea))) {
          continue;
        }

        if (cond->condition->Evaluate(tagMap)) {
          if (wayType==typeIgnore &&
              (cond->types & TypeInfo::typeWay)) {
            wayType=types[i].GetId();
          }

          if (areaType==typeIgnore &&
              (cond->types & TypeInfo::typeArea)) {
            areaType=types[i].GetId();
          }

          if (wayType!=typeIgnore ||
              areaType!=typeIgnore) {
            return true;
          }
        }
      }
    }

    return false;
  }

  bool TypeConfig::GetRelationTypeId(const std::map<TagId,std::string>& tagMap,
                                     TypeId &typeId) const
  {
    typeId=typeIgnore;

    if (tagMap.empty()) {
      return false;
    }

    std::map<TagId,std::string>::const_iterator relationType=tagMap.find(tagType);

    if (relationType!=tagMap.end() &&
        relationType->second=="multipolygon") {
      for (size_t i=0; i<types.size(); i++) {
        if (!types[i].HasConditions() ||
            !types[i].CanBeArea()) {
          continue;
        }

        for (std::list<TypeInfo::TypeCondition>::const_iterator cond=types[i].GetConditions().begin();
             cond!=types[i].GetConditions().end();
             ++cond) {
          if (!(cond->types & TypeInfo::typeArea)) {
            continue;
          }

          if (cond->condition->Evaluate(tagMap)) {
            typeId=types[i].GetId();
            return true;
          }
        }
      }
    }
    else {
      for (size_t i=0; i<types.size(); i++) {
        if (!types[i].HasConditions() ||
            !types[i].CanBeRelation()) {
          continue;
        }

        for (std::list<TypeInfo::TypeCondition>::const_iterator cond=types[i].GetConditions().begin();
             cond!=types[i].GetConditions().end();
             ++cond) {
          if (!(cond->types & TypeInfo::typeRelation)) {
            continue;
          }

          if (cond->condition->Evaluate(tagMap)) {
            typeId=types[i].GetId();
            return true;
          }
        }
      }
    }

    return false;
  }

  TypeId TypeConfig::GetTypeId(const std::string& name) const
  {
    OSMSCOUT_HASHMAP<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetNodeTypeId(const std::string& name) const
  {
    OSMSCOUT_HASHMAP<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end() &&
        iter->second.CanBeNode()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetWayTypeId(const std::string& name) const
  {
    OSMSCOUT_HASHMAP<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end() &&
        iter->second.CanBeWay()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetAreaTypeId(const std::string& name) const
  {
    OSMSCOUT_HASHMAP<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end() &&
        iter->second.CanBeArea()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetRelationTypeId(const std::string& name) const
  {
    OSMSCOUT_HASHMAP<std::string,TypeInfo>::const_iterator iter=nameToTypeMap.find(name);

    if (iter!=nameToTypeMap.end() &&
        iter->second.CanBeRelation()) {
      return iter->second.GetId();
    }

    return typeIgnore;
  }

  void TypeConfig::GetAreaTypes(std::set<TypeId>& types) const
  {
    for (std::vector<TypeInfo>::const_iterator type=this->types.begin();
        type!=this->types.end();
        type++) {
      if (type->GetId()==typeIgnore) {
        continue;
      }

      if (type->GetIgnore()) {
        continue;
      }

      if (type->CanBeArea()) {
        types.insert(type->GetId());
      }
    }
  }

  void TypeConfig::GetWayTypes(std::set<TypeId>& types) const
  {
    for (std::vector<TypeInfo>::const_iterator type=this->types.begin();
        type!=this->types.end();
        type++) {
      if (type->GetId()==typeIgnore) {
        continue;
      }

      if (type->GetIgnore()) {
        continue;
      }

      if (type->CanBeWay()) {
        types.insert(type->GetId());
      }
    }
  }

  void TypeConfig::GetRoutables(std::set<TypeId>& types) const
  {
    types.clear();

    for (std::vector<TypeInfo>::const_iterator type=this->types.begin();
         type!=this->types.end();
         ++type) {
      if (type->CanRouteFoot() ||
          type->CanRouteBicycle() ||
          type->CanRouteCar()) {
        types.insert(type->GetId());
      }
    }
  }

  void TypeConfig::GetIndexAsLocationTypes(OSMSCOUT_HASHSET<TypeId>& types) const
  {
    types.clear();

    for (std::vector<TypeInfo>::const_iterator type=this->types.begin();
         type!=this->types.end();
         ++type) {
      if (type->GetIndexAsLocation()) {
        types.insert(type->GetId());
      }
    }
  }

  void TypeConfig::GetIndexAsRegionTypes(OSMSCOUT_HASHSET<TypeId>& types) const
  {
    types.clear();

    for (std::vector<TypeInfo>::const_iterator type=this->types.begin();
         type!=this->types.end();
         ++type) {
      if (type->GetIndexAsRegion()) {
        types.insert(type->GetId());
      }
    }
  }

  void TypeConfig::GetIndexAsPOITypes(OSMSCOUT_HASHSET<TypeId>& types) const
  {
    types.clear();

    for (std::vector<TypeInfo>::const_iterator type=this->types.begin();
         type!=this->types.end();
         ++type) {
      if (type->GetIndexAsPOI()) {
        types.insert(type->GetId());
      }
    }
  }

  void TypeConfig::RegisterSurfaceToGradeMapping(const std::string& surface,
                                                 size_t grade)
  {
    surfaceToGradeMap.insert(std::make_pair(surface,
                                            grade));
  }

  bool TypeConfig::GetGradeForSurface(const std::string& surface,
                                      size_t& grade) const
  {
    OSMSCOUT_HASHMAP<std::string,size_t>::const_iterator entry=surfaceToGradeMap.find(surface);

    if (entry!=surfaceToGradeMap.end()) {
      grade=entry->second;

      return true;
    }
    else {
      return false;
    }
  }

  /**
   * Loads the type configuration from the given *.ost file.
   *
   * Note:
   * Make sure that you load from a OST file only onto a freshly initialized
   * TypeConfig instance.
   *
   * @param filename
   *    Full filename including path of the OST file
   * @return
   *    True, if there were no errors, else false
   */
  bool TypeConfig::LoadFromOSTFile(const std::string& filename)
  {
    FileOffset fileSize;
    FILE*      file;
    bool       success=false;

    if (!GetFileSize(filename,
                     fileSize)) {
      std::cerr << "Cannot get size of file '" << filename << "'" << std::endl;
      return false;
    }

    file=fopen(filename.c_str(),"rb");
    if (file==NULL) {
      std::cerr << "Cannot open file '" << filename << "'" << std::endl;
      return false;
    }

    unsigned char* content=new unsigned char[fileSize];

    if (fread(content,1,fileSize,file)!=(size_t)fileSize) {
      std::cerr << "Cannot load file '" << filename << "'" << std::endl;
      delete [] content;
      fclose(file);
      return false;
    }

    fclose(file);

    ost::Scanner *scanner=new ost::Scanner(content,
                                           fileSize);
    ost::Parser  *parser=new ost::Parser(scanner,
                                         *this);

    delete [] content;

    parser->Parse();

    success=!parser->errors->hasErrors;

    delete parser;
    delete scanner;

    return success;
  }

  /**
   * Loads the type configuration from the given binary data file.
   *
   * Note:
   * Make sure that you load from afile only onto a freshly initialized
   * TypeConfig instance.
   *
   * @param directory
   *    Full path excluding the actual filename of the data file
   *    (filename is always "types.dat")
   * @return
   *    True, if there were no errors, else false
   */
  bool TypeConfig::LoadFromDataFile(const std::string& directory)
  {
    FileScanner scanner;

    if (!scanner.Open(AppendFileToDir(directory,
                                      "types.dat"),
                      FileScanner::Sequential,
                      true)) {
      std::cerr << "Cannot open file '" << scanner.GetFilename() << "'" << std::endl;
     return false;
    }

    uint32_t tagCount;

    if (!scanner.ReadNumber(tagCount)) {
      std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      return false;
    }

    for (size_t i=1; i<=tagCount; i++) {
      TagId       requestedId;
      TagId       actualId;
      std::string name;
      bool        internalOnly;

      if (!(scanner.ReadNumber(requestedId) &&
            scanner.Read(name),
            scanner.Read(internalOnly))) {
        std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
        return false;
      }

      if (internalOnly) {
        actualId=RegisterTagForInternalUse(name);
      }
      else {
        actualId=RegisterTagForExternalUse(name);
      }

      if (actualId!=requestedId) {
        std::cerr << "Requested and actual tag id do not match" << std::endl;
        return false;
      }
    }

    uint32_t nameTagCount;

    if (!scanner.ReadNumber(nameTagCount)) {
      std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      return false;
    }

    for (size_t i=1; i<=nameTagCount; i++) {
      TagId       requestedId;
      TagId       actualId;
      std::string name;
      uint32_t    priority = 0;

      if (!(scanner.ReadNumber(requestedId) &&
            scanner.Read(name) &&
            scanner.ReadNumber(priority))) {
        std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      }

      actualId=RegisterNameTag(name,priority);

      if (actualId!=requestedId) {
        std::cerr << "Requested and actual name tag id do not match" << std::endl;
        return false;
      }
    }

    uint32_t nameAltTagCount;

    if (!scanner.ReadNumber(nameAltTagCount)) {
      std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      return false;
    }

    for (size_t i=1; i<=nameAltTagCount; i++) {
      TagId       requestedId;
      TagId       actualId;
      std::string name;
      uint32_t    priority = 0;

      if (!(scanner.ReadNumber(requestedId) &&
            scanner.Read(name) &&
            scanner.ReadNumber(priority))) {
        std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      }

      actualId=RegisterNameAltTag(name,priority);

      if (actualId!=requestedId) {
        std::cerr << "Requested and actual name alt tag id do not match" << std::endl;
        return false;
      }
    }

    uint32_t typeCount;

    if (!scanner.ReadNumber(typeCount)) {
      std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      return false;
    }

    for (size_t i=1; i<=typeCount; i++) {
      TypeId      id;
      std::string name;
      bool        canBeNode;
      bool        canBeWay;
      bool        canBeArea;
      bool        canBeRelation;
      bool        canRouteFoot;
      bool        canRouteBicycle;
      bool        canRouteCar;
      bool        indexAsLocation;
      bool        indexAsRegion;
      bool        indexAsPOI;
      bool        optimizeLowZoom;
      bool        multipolygon;
      bool        pinWay;
      bool        ignore;
      bool        ignoreSeaLand;

      if (!(scanner.ReadNumber(id) &&
            scanner.Read(name) &&
            scanner.Read(canBeNode) &&
            scanner.Read(canBeWay) &&
            scanner.Read(canBeArea) &&
            scanner.Read(canBeRelation) &&
            scanner.Read(canRouteFoot) &&
            scanner.Read(canRouteBicycle) &&
            scanner.Read(canRouteCar) &&
            scanner.Read(indexAsLocation) &&
            scanner.Read(indexAsRegion) &&
            scanner.Read(indexAsPOI) &&
            scanner.Read(optimizeLowZoom) &&
            scanner.Read(multipolygon) &&
            scanner.Read(pinWay) &&
            scanner.Read(ignoreSeaLand) &&
            scanner.Read(ignore))) {

        std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
        return false;
      }

      TypeInfo typeInfo;

      typeInfo.SetId(id);

      typeInfo.SetType(name);

      typeInfo.CanBeNode(canBeNode);
      typeInfo.CanBeWay(canBeWay);
      typeInfo.CanBeArea(canBeArea);
      typeInfo.CanBeRelation(canBeRelation);
      typeInfo.CanRouteFoot(canRouteFoot);
      typeInfo.CanRouteBicycle(canRouteBicycle);
      typeInfo.CanRouteCar(canRouteCar);
      typeInfo.SetIndexAsLocation(indexAsLocation);
      typeInfo.SetIndexAsRegion(indexAsRegion);
      typeInfo.SetIndexAsPOI(indexAsPOI);
      typeInfo.SetIgnore(optimizeLowZoom);
      typeInfo.SetIgnore(multipolygon);
      typeInfo.SetIgnore(pinWay );
      typeInfo.SetIgnore(ignoreSeaLand);
      typeInfo.SetIgnore(ignore);

      uint32_t featureCount;

      if (!scanner.ReadNumber(featureCount)) {
        return false;
      }

      for (size_t i=0; i<featureCount; i++) {
        std::string featureName;

        if (!scanner.Read(featureName)) {
          return false;
        }

        FeatureRef feature=GetFeature(featureName);

        if (feature.Invalid()) {
          std::cerr << "Feature '" << featureName << "' not found" << std::endl;
          return false;
        }

        typeInfo.AddFeature(feature);
      }

      AddTypeInfo(typeInfo);
    }

    return !scanner.HasError() && scanner.Close();
  }

  /**
   * Store the part of the TypeConfig information to a data file,
   * which is necessary to review later on when reading and
   * evaluation an import.
   *
   * @param directory
   *    Directory the data file should be written to
   * @return
   *    True, if there were no errors, else false
   */
  bool TypeConfig::StoreToDataFile(const std::string& directory) const
  {
    FileWriter writer;

     if (!writer.Open(AppendFileToDir(directory,
                                      "types.dat"))) {
       //progress.Error("Cannot create 'types.dat'");
       return false;
     }

     writer.WriteNumber((uint32_t)GetTags().size());
     for (std::vector<TagInfo>::const_iterator tag=GetTags().begin();
          tag!=GetTags().end();
          ++tag) {
       writer.WriteNumber(tag->GetId());
       writer.Write(tag->GetName());
       writer.Write(tag->IsInternalOnly());
     }

     uint32_t nameTagCount=0;
     uint32_t nameAltTagCount=0;

     for (std::vector<TagInfo>::const_iterator tag=GetTags().begin();
          tag!=GetTags().end();
          ++tag) {
       uint32_t priority;

       if (IsNameTag(tag->GetId(),priority)) {
         nameTagCount++;
       }

       if (IsNameAltTag(tag->GetId(),priority)) {
         nameAltTagCount++;
       }
     }

     writer.WriteNumber(nameTagCount);
     for (std::vector<TagInfo>::const_iterator tag=GetTags().begin();
      tag!=GetTags().end();
      ++tag) {
       uint32_t priority;

       if (IsNameTag(tag->GetId(),priority)) {
         writer.WriteNumber(tag->GetId());
         writer.Write(tag->GetName());
         writer.WriteNumber((uint32_t)priority);
       }
     }

     writer.WriteNumber(nameAltTagCount);
     for (std::vector<TagInfo>::const_iterator tag=GetTags().begin();
      tag!=GetTags().end();
      ++tag) {
       uint32_t priority;

       if (IsNameAltTag(tag->GetId(),priority)) {
         writer.WriteNumber(tag->GetId());
         writer.Write(tag->GetName());
         writer.WriteNumber((uint32_t)priority);
       }
     }


     writer.WriteNumber((uint32_t)GetTypes().size());

     for (std::vector<TypeInfo>::const_iterator type=GetTypes().begin();
          type!=GetTypes().end();
          ++type) {
       writer.WriteNumber(type->GetId());
       writer.Write(type->GetName());
       writer.Write(type->CanBeNode());
       writer.Write(type->CanBeWay());
       writer.Write(type->CanBeArea());
       writer.Write(type->CanBeRelation());
       writer.Write(type->CanRouteFoot());
       writer.Write(type->CanRouteBicycle());
       writer.Write(type->CanRouteCar());
       writer.Write(type->GetIndexAsLocation());
       writer.Write(type->GetIndexAsRegion());
       writer.Write(type->GetIndexAsPOI());
       writer.Write(type->GetOptimizeLowZoom());
       writer.Write(type->GetMultipolygon());
       writer.Write(type->GetPinWay());
       writer.Write(type->GetIgnoreSeaLand());
       writer.Write(type->GetIgnore());

       writer.WriteNumber((uint32_t)type->GetFeatures().size());
       for (std::list<FeatureRef>::const_iterator feature=type->GetFeatures().begin();
           feature!=type->GetFeatures().end();
           ++feature) {
         writer.Write((*feature)->GetName());
       }
     }

     return !writer.HasError() && writer.Close();
  }
}
