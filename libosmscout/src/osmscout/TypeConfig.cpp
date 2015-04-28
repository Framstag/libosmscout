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

#include <osmscout/TypeFeatures.h>

#include <osmscout/system/Assert.h>

#include <osmscout/ost/Parser.h>
#include <osmscout/ost/Scanner.h>

#include <osmscout/util/File.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/String.h>

namespace osmscout {

  FeatureValue::FeatureValue()
  {
    // no code
  }

  FeatureValue::~FeatureValue()
  {
    // no code
  }

  bool FeatureValue::Read(FileScanner& /*scanner*/)
  {
    assert(false);

    return true;
  }

  bool FeatureValue::Write(FileWriter& /*writer*/)
  {
    assert(false);

    return true;
  }

  Feature::Feature()
  {
    // no code
  }

  Feature::~Feature()
  {
    // no code
  }

  size_t Feature::RegisterLabel(const std::string& labelName,
                                size_t index)
  {
    assert(labels.find(labelName)==labels.end());

    labels[labelName]=index;

    return index;
  }

  bool Feature::GetLabelIndex(const std::string& labelName,
                             size_t& index) const
  {
    const auto entry=labels.find(labelName);

    if (entry==labels.end()) {
      return false;
    }

    index=entry->second;

    return true;
  }

  FeatureValue* Feature::AllocateValue(void* /*buffer*/)
  {
    assert(false);
    return NULL;
  }

  FeatureInstance::FeatureInstance(const FeatureRef& feature,
                                   const TypeInfo* type,
                                   size_t index,
                                   size_t offset)
  : feature(feature),
    type(type),
    index(index),
    offset(offset)
  {
    assert(feature.Valid());
  }

  FeatureValueBuffer::FeatureValueBuffer()
  : featureBits(NULL),
    featureValueBuffer(NULL)
  {
    // no code
  }

  FeatureValueBuffer::FeatureValueBuffer(const FeatureValueBuffer& other)
  : featureBits(NULL),
    featureValueBuffer(NULL)
  {
    Set(other);
  }

  FeatureValueBuffer::~FeatureValueBuffer()
  {
    if (type) {
      DeleteData();
    }
  }

  void FeatureValueBuffer::Set(const FeatureValueBuffer& other)
  {
    if (other.GetType()) {
      SetType(other.GetType());

      for (size_t idx=0; idx<other.GetFeatureCount(); idx++) {
        if (other.HasValue(idx)) {
          if (other.GetFeature(idx).GetFeature()->HasValue()) {
            FeatureValue* otherValue=other.GetValue(idx);
            FeatureValue* thisValue=AllocateValue(idx);

            *thisValue=*otherValue;
          }
          else {
            size_t byteIdx=idx/8;

            featureBits[byteIdx]=featureBits[byteIdx] | (1 << idx%8);
          }
        }
      }
    }
    else if (type) {
      DeleteData();
    }
  }

  void FeatureValueBuffer::SetType(const TypeInfoRef& type)
  {
    if (this->type) {
      DeleteData();
    }

    this->type=type;

    AllocateData();
  }

  void FeatureValueBuffer::DeleteData()
  {
    if (featureValueBuffer!=NULL) {
      for (size_t i=0; i<type->GetFeatureCount(); i++) {
        if (HasValue(i)) {
          FreeValue(i);
        }
      }

      ::operator delete((void*)featureValueBuffer);
      featureValueBuffer=NULL;
    }

    if (featureBits!=NULL) {
      delete [] featureBits;
      featureBits=NULL;
    }

    type=NULL;
  }

  void FeatureValueBuffer::AllocateData()
  {
    if (type->HasFeatures()) {
      featureBits=new uint8_t[type->GetFeatureMaskBytes()]();
      featureValueBuffer=static_cast<char*>(::operator new(type->GetFeatureValueBufferSize()));
    }
    else
    {
      featureBits=NULL;
      featureValueBuffer=NULL;
    }
  }

  FeatureValue* FeatureValueBuffer::AllocateValue(size_t idx)
  {
    size_t byteIdx=idx/8;

    featureBits[byteIdx]=featureBits[byteIdx] | (1 << idx%8);

    if (type->GetFeature(idx).GetFeature()->HasValue()) {
      FeatureValue* value=GetValue(idx);

      return type->GetFeature(idx).GetFeature()->AllocateValue(value);
    }
    else {
      return NULL;
    }
  }

  void FeatureValueBuffer::FreeValue(size_t idx)
  {
    size_t byteIdx=idx/8;

    featureBits[byteIdx]=featureBits[byteIdx] & ~(1 << idx%8);

    if (type->GetFeature(idx).GetFeature()->HasValue()) {
      FeatureValue* value=GetValue(idx);

      value->~FeatureValue();
    }
  }

  void FeatureValueBuffer::Parse(Progress& progress,
                                 const TypeConfig& typeConfig,
                                 const ObjectOSMRef& object,
                                 const TagMap& tags)
  {
    for (const auto &feature : type->GetFeatures()) {
      feature.GetFeature()->Parse(progress,
                                  typeConfig,
                                  feature,
                                  object,
                                  tags,
                                  *this);
    }
  }

  bool FeatureValueBuffer::Read(FileScanner& scanner)
  {
    for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
      if (!scanner.Read(featureBits[i])) {
        return false;
      }
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasValue(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=feature.GetFeature()->AllocateValue(GetValue(idx));

        if (!value->Read(scanner)) {
          return false;
        }
      }
    }

    return !scanner.HasError();
  }

  bool FeatureValueBuffer::Read(FileScanner& scanner,
                                bool& specialFlag)
  {
    for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
      if (!scanner.Read(featureBits[i])) {
        return false;
      }
    }

    if (type->GetFeatureCount()%8!=0) {
      specialFlag=featureBits[type->GetFeatureMaskBytes()-1] & 0x80;
    }
    else {
      uint8_t addByte;

      if (!scanner.Read(addByte)) {
        return false;
      }

      specialFlag=addByte & 0x80;
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasValue(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=feature.GetFeature()->AllocateValue(GetValue(idx));

        if (!value->Read(scanner)) {
          return false;
        }
      }
    }

    return !scanner.HasError();
  }

  bool FeatureValueBuffer::Write(FileWriter& writer) const
  {
    for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
      if (!writer.Write(featureBits[i])) {
        return false;
      }
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasValue(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=GetValue(idx);

        if (!value->Write(writer)) {
          return false;
        }
      }
    }

    return !writer.HasError();
  }

  bool FeatureValueBuffer::Write(FileWriter& writer,
                                 bool specialFlag) const
  {
    if (type->GetFeatureCount()%8!=0) {
      if (specialFlag) {
        featureBits[type->GetFeatureMaskBytes()-1]|=0x80;
      }
      else {
        featureBits[type->GetFeatureMaskBytes()-1]&=~0x80;
      }

      for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
        if (!writer.Write(featureBits[i])) {
          return false;
        }
      }
    }
    else {
      for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
        if (!writer.Write(featureBits[i])) {
          return false;
        }
      }

      uint8_t addByte=specialFlag ? 0x80 : 0x00;

      if (!writer.Write(addByte)) {
        return false;
      }
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasValue(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=GetValue(idx);

        if (!value->Write(writer)) {
          return false;
        }
      }
    }

    return !writer.HasError();
  }

  FeatureValueBuffer& FeatureValueBuffer::operator=(const FeatureValueBuffer& other)
  {
    Set(other);

    return *this;
  }

  bool FeatureValueBuffer::operator==(const FeatureValueBuffer& other) const
  {
    if (this->type!=other.type) {
      return false;
    }

    for (size_t i=0; i<GetFeatureCount(); i++) {
      if (HasValue(i)!=other.HasValue(i)) {
        return false;
      }

      // If a feature has a value, we compare the values
      if (HasValue(i) &&
          other.HasValue(i) &&
          GetFeature(i).GetFeature()->HasValue()) {
        FeatureValue *thisValue=GetValue(i);
        FeatureValue *otherValue=other.GetValue(i);

        if (!(*thisValue==*otherValue)) {
          return false;
        }
      }
    }

    return true;
  }

  bool FeatureValueBuffer::operator!=(const FeatureValueBuffer& other) const
  {
    return !operator==(other);
  }

  TypeInfo::TypeInfo()
   : nodeId(0),
     wayId(0),
     areaId(0),
     index(0),
     featureMaskBytes(0),
     specialFeatureMaskBytes(0),
     valueBufferSize(0),
     canBeNode(false),
     canBeWay(false),
     canBeArea(false),
     canBeRelation(false),
     isPath(false),
     canRouteFoot(false),
     canRouteBicycle(false),
     canRouteCar(false),
     indexAsAddress(false),
     indexAsLocation(false),
     indexAsRegion(false),
     indexAsPOI(false),
     optimizeLowZoom(false),
     multipolygon(false),
     pinWay(false),
     mergeAreas(false),
     ignoreSeaLand(false),
     ignore(false)
  {
    // no code
  }

  /**
   * We forbid copying of TypeInfo instances
   *
   * @param other
   */
  TypeInfo::TypeInfo(const TypeInfo& /*other*/)
  {
    // no code
  }

  TypeInfo::~TypeInfo()
  {
    // no code
  }

  TypeInfo& TypeInfo::SetNodeId(TypeId id)
  {
    this->nodeId=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetWayId(TypeId id)
  {
    this->wayId=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetAreaId(TypeId id)
  {
    this->areaId=id;

    return *this;
  }

  TypeInfo& TypeInfo::SetIndex(size_t index)
  {
    this->index=index;

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
    assert(nameToFeatureMap.find(feature->GetName())==nameToFeatureMap.end());

    size_t index=0;
    size_t offset=0;
    size_t alignment=std::max(sizeof(size_t),sizeof(void*));

    if (!features.empty()) {
      index=features.back().GetIndex()+1;
      offset=features.back().GetOffset()+features.back().GetFeature()->GetValueSize();
      if (offset%alignment!=0) {
        offset=(offset/alignment+1)*alignment;
      }
    }


    features.push_back(FeatureInstance(feature,
                                       this,
                                       index,
                                       offset));
    nameToFeatureMap.insert(std::make_pair(feature->GetName(),index));

    featureMaskBytes=BitsToBytes(features.size());
    specialFeatureMaskBytes=BitsToBytes(features.size()+1);

    valueBufferSize=offset+feature->GetValueSize();

    return *this;
  }

  TypeInfo& TypeInfo::AddGroup(const std::string& groupName)
  {
    groups.insert(groupName);

    return *this;
  }

  bool TypeInfo::HasFeature(const std::string& featureName) const
  {
    return nameToFeatureMap.find(featureName)!=nameToFeatureMap.end();
  }

  /**
   * Return the feature with the given name
   */
  bool TypeInfo::GetFeature(const std::string& name,
                            size_t& index) const
  {
    auto entry=nameToFeatureMap.find(name);

    if (entry!=nameToFeatureMap.end()) {
      index=entry->second;

      return true;
    }
    else {
      return false;
    }
  }

  uint8_t TypeInfo::GetDefaultAccess() const
  {
    uint8_t access=0;

    if (CanRouteFoot()) {
      access|=(AccessFeatureValue::footForward|AccessFeatureValue::footBackward);
    }

    if (CanRouteBicycle()) {
      access|=(AccessFeatureValue::bicycleForward|AccessFeatureValue::bicycleBackward);
    }

    if (CanRouteCar()) {
      access|=(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);
    }

    return access;
  }

  TypeInfoSet::TypeInfoSet()
  : count(0)
  {
    // no code
  }

  TypeInfoSet::TypeInfoSet(const TypeConfig& typeConfig)
  : count(0)
  {
    types.resize(typeConfig.GetTypeCount());
  }

  TypeInfoSet::TypeInfoSet(const TypeInfoSet& other)
  : types(other.types),
    count(other.count)
  {
    // no code
  }

  TypeInfoSet::TypeInfoSet(const std::vector<TypeInfoRef>& types)
  {
    for (const auto& type : types) {
      Set(type);
    }
  }

  void TypeInfoSet::Adapt(const TypeConfig& typeConfig)
  {
    types.resize(typeConfig.GetTypeCount());
  }

  void TypeInfoSet::Set(const TypeInfoRef& type)
  {
    assert(type);

    if (type->GetIndex()>=types.size()) {
      types.resize(type->GetIndex()+1);
    }

    if (!types[type->GetIndex()]) {
      types[type->GetIndex()]=type;
      count++;
    }
  }

  void TypeInfoSet::Set(const std::vector<TypeInfoRef>& types)
  {
    Clear();

    for (const auto& type : types) {
      Set(type);
    }
  }

  void TypeInfoSet::Remove(const TypeInfoRef& type)
  {
    assert(type);

    if (type->GetIndex()<types.size() &&
        types[type->GetIndex()]) {
      types[type->GetIndex()]=NULL;
      count--;
    }
  }

  void TypeInfoSet::Remove(const TypeInfoSet& otherTypes)
  {
    for (const auto &type : otherTypes.types)
    {
      if (type &&
          type->GetIndex()<types.size() &&
          types[type->GetIndex()]) {
        types[type->GetIndex()]=NULL;
        count--;
      }
    }
  }

  TypeConfig::TypeConfig()
   : nextTagId(0),
     nodeTypIdBytes(1),
     wayTypIdBytes(1),
     areaTypIdBytes(1)
  {
    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for tagIgnore
    RegisterTag("");

    RegisterTag("area");
    RegisterTag("natural");
    RegisterTag("type");
    RegisterTag("restriction");

    featureName=new NameFeature();
    RegisterFeature(featureName);

    RegisterFeature(new NameAltFeature());

    featureRef=new RefFeature();
    RegisterFeature(featureRef);

    featureLocation=new LocationFeature();
    RegisterFeature(featureLocation);

    featureAddress=new AddressFeature();
    RegisterFeature(featureAddress);

    featureAccess=new AccessFeature();
    RegisterFeature(featureAccess);

    featureAccessRestricted=new AccessRestrictedFeature();
    RegisterFeature(featureAccessRestricted);

    featureLayer=new LayerFeature();
    RegisterFeature(featureLayer);

    featureWidth=new WidthFeature();
    RegisterFeature(featureWidth);

    featureMaxSpeed=new MaxSpeedFeature();
    RegisterFeature(featureMaxSpeed);

    featureGrade=new GradeFeature();
    RegisterFeature(featureGrade);

    RegisterFeature(new AdminLevelFeature());

    featureBridge=new BridgeFeature();
    RegisterFeature(featureBridge);

    featureTunnel=new TunnelFeature();
    RegisterFeature(featureTunnel);

    featureRoundabout=new RoundaboutFeature();
    RegisterFeature(featureRoundabout);

    RegisterFeature(new EleFeature());

    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for typeIgnore
    typeInfoIgnore=std::make_shared<TypeInfo>();
    typeInfoIgnore->SetType("");
    typeInfoIgnore->SetIgnore(true);

    RegisterType(typeInfoIgnore);


    //
    // Internal type for showing routes
    //

    TypeInfoRef route(new TypeInfo());

    route->SetType("_route")
          .CanBeWay(true);

    RegisterType(route);

    //
    // Internal types for the land/sea/coast tiles building the base layer for map drawing
    //

    typeInfoTileLand=std::make_shared<TypeInfo>();

    typeInfoTileLand->SetType("_tile_land")
              .CanBeArea(true);

    RegisterType(typeInfoTileLand);


    typeInfoTileSea=std::make_shared<TypeInfo>();

    typeInfoTileSea->SetType("_tile_sea")
             .CanBeArea(true);

    RegisterType(typeInfoTileSea);


    typeInfoTileCoast=std::make_shared<TypeInfo>();

    typeInfoTileCoast->SetType("_tile_coast")
               .CanBeArea(true);

    RegisterType(typeInfoTileCoast);


    typeInfoTileUnknown=std::make_shared<TypeInfo>();

    typeInfoTileUnknown->SetType("_tile_unknown")
                .CanBeArea(true);

    RegisterType(typeInfoTileUnknown);


    typeInfoTileCoastline=std::make_shared<TypeInfo>();

    typeInfoTileCoastline->SetType("_tile_coastline")
                   .CanBeWay(true);

    RegisterType(typeInfoTileCoastline);


    tagArea=GetTagId("area");
    tagNatural=GetTagId("natural");
    tagType=GetTagId("type");
    tagRestriction=GetTagId("restriction");

    assert(tagArea!=tagIgnore);
    assert(tagNatural!=tagIgnore);
    assert(tagType!=tagIgnore);
    assert(tagRestriction!=tagIgnore);
  }

  TypeConfig::~TypeConfig()
  {
    // no code
  }

  TagId TypeConfig::RegisterTag(const std::string& tagName)
  {
    auto mapping=stringToTagMap.find(tagName);

    if (mapping!=stringToTagMap.end()) {
      return mapping->second;
    }

    TagInfo tagInfo(tagName);

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
    TagId tagId=RegisterTag(tagName);

    nameTagIdToPrioMap.insert(std::make_pair(tagId,priority));

    return tagId;
  }

  TagId TypeConfig::RegisterNameAltTag(const std::string& tagName, uint32_t priority)
  {
    TagId tagId=RegisterTag(tagName);

    nameAltTagIdToPrioMap.insert(std::make_pair(tagId,priority));

    return tagId;
  }

  void TypeConfig::RegisterFeature(const FeatureRef& feature)
  {
    assert(feature.Valid());
    assert(!feature->GetName().empty());

    if (nameToFeatureMap.find(feature->GetName())!=nameToFeatureMap.end()) {
      return;
    }

    features.push_back(feature);
    nameToFeatureMap[feature->GetName()]=feature;

    feature->Initialize(*this);
  }

  FeatureRef TypeConfig::GetFeature(const std::string& name) const
  {
    auto feature=nameToFeatureMap.find(name);

    if (feature!=nameToFeatureMap.end()) {
      return feature->second;
    }
    else {
      return NULL;
    }
  }

  TypeInfoRef TypeConfig::RegisterType(const TypeInfoRef& typeInfo)
  {
    assert(typeInfo);

    auto existingType=nameToTypeMap.find(typeInfo->GetName());

    if (existingType!=nameToTypeMap.end()) {
      return existingType->second;
    }

    if ((typeInfo->CanBeArea() ||
         typeInfo->CanBeNode()) &&
        typeInfo->GetIndexAsAddress()) {
      if (!typeInfo->HasFeature(LocationFeature::NAME)) {
        typeInfo->AddFeature(featureLocation);
      }
      if (!typeInfo->HasFeature(AddressFeature::NAME)) {
        typeInfo->AddFeature(featureAddress);
      }
    }

    // All ways have a layer
    if (typeInfo->CanBeWay()) {
      if (!typeInfo->HasFeature(LayerFeature::NAME)) {
        typeInfo->AddFeature(featureLayer);
      }
    }

    // All that is PATH-like automatically has a number of features,
    // even if it is not routable
    if (typeInfo->IsPath()) {
      if (!typeInfo->HasFeature(WidthFeature::NAME)) {
        typeInfo->AddFeature(featureWidth);
      }
      if (!typeInfo->HasFeature(GradeFeature::NAME)) {
        typeInfo->AddFeature(featureGrade);
      }
      if (!typeInfo->HasFeature(BridgeFeature::NAME)) {
        typeInfo->AddFeature(featureBridge);
      }
      if (!typeInfo->HasFeature(TunnelFeature::NAME)) {
        typeInfo->AddFeature(featureTunnel);
      }
      if (!typeInfo->HasFeature(RoundaboutFeature::NAME)) {
        typeInfo->AddFeature(featureRoundabout);
      }
    }

    // Everything routable should have access information and max speed information
    if (typeInfo->CanRoute()) {
      if (!typeInfo->HasFeature(AccessFeature::NAME)) {
        typeInfo->AddFeature(featureAccess);
      }
      if (!typeInfo->HasFeature(AccessRestrictedFeature::NAME)) {
        typeInfo->AddFeature(featureAccessRestricted);
      }
      if (!typeInfo->HasFeature(MaxSpeedFeature::NAME)) {
        typeInfo->AddFeature(featureMaxSpeed);
      }
    }

    // Something that has a name and is a POI automatically get the
    // location and address features, too.
    if (typeInfo->HasFeature(NameFeature::NAME) &&
        typeInfo->GetIndexAsPOI()) {
      if (!typeInfo->HasFeature(LocationFeature::NAME)) {
        typeInfo->AddFeature(featureLocation);
      }
      if (!typeInfo->HasFeature(AddressFeature::NAME)) {
        typeInfo->AddFeature(featureAddress);
      }
    }

    typeInfo->SetIndex(types.size());

    types.push_back(typeInfo);

    if (!typeInfo->GetIgnore() &&
        (typeInfo->CanBeNode() ||
         typeInfo->CanBeWay() ||
         typeInfo->CanBeArea())) {
      if (typeInfo->CanBeNode()) {
        typeInfo->SetNodeId(nodeTypes.size()+1);
        nodeTypes.push_back(typeInfo);

        if (nodeTypes.size()<256) {
          nodeTypIdBytes=1;
        }
        else {
          nodeTypIdBytes=2;
        }
      }

      if (typeInfo->CanBeWay()) {
        typeInfo->SetWayId(wayTypes.size()+1);
        wayTypes.push_back(typeInfo);

        if (wayTypes.size()<256) {
          wayTypIdBytes=1;
        }
        else {
          wayTypIdBytes=2;
        }
      }

      if (typeInfo->CanBeArea()) {
        typeInfo->SetAreaId(areaTypes.size()+1);
        areaTypes.push_back(typeInfo);

        if (areaTypes.size()<256) {
          areaTypIdBytes=1;
        }
        else {
          areaTypIdBytes=2;
        }
      }
    }

    nameToTypeMap[typeInfo->GetName()]=typeInfo;

    return typeInfo;
  }

  TypeId TypeConfig::GetMaxTypeId() const
  {
    if (types.empty()) {
      return 0;
    }
    else {
      return types.size();
    }
  }

  TagId TypeConfig::GetTagId(const char* name) const
  {
    auto iter=stringToTagMap.find(name);

    if (iter!=stringToTagMap.end()) {
      return iter->second;
    }
    else {
      return tagIgnore;
    }
  }

  const TypeInfoRef TypeConfig::GetTypeInfo(const std::string& name) const
  {
    auto typeEntry=nameToTypeMap.find(name);

    if (typeEntry!=nameToTypeMap.end()) {
      return typeEntry->second;
    }

    return TypeInfoRef();
  }

  bool TypeConfig::IsNameTag(TagId tag, uint32_t& priority) const
  {
    if (nameTagIdToPrioMap.empty()) {
      return false;
    }

    auto entry=nameTagIdToPrioMap.find(tag);

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

    auto entry=nameAltTagIdToPrioMap.find(tag);

    if (entry==nameAltTagIdToPrioMap.end()) {
      return false;
    }

    priority=entry->second;

    return true;
  }

  TypeInfoRef TypeConfig::GetNodeType(const TagMap& tagMap) const
  {
    if (tagMap.empty()) {
      return typeInfoIgnore;
    }

    for (const auto &type : types) {
      if (!type->HasConditions() ||
          !type->CanBeNode()) {
        continue;
      }

      for (const auto &cond : type->GetConditions()) {
        if (!(cond.types & TypeInfo::typeNode)) {
          continue;
        }

        if (cond.condition->Evaluate(tagMap)) {
          return type;
        }
      }
    }

    return typeInfoIgnore;
  }

  bool TypeConfig::GetWayAreaType(const TagMap& tagMap,
                                  TypeInfoRef& wayType,
                                  TypeInfoRef& areaType) const
  {
    wayType=typeInfoIgnore;
    areaType=typeInfoIgnore;

    if (tagMap.empty()) {
      return false;
    }

    for (const auto &type : types) {
      if (!((type->CanBeWay() ||
             type->CanBeArea()) &&
             type->HasConditions())) {
        continue;
      }

      for (const auto &cond : type->GetConditions()) {
        if (!((cond.types & TypeInfo::typeWay) ||
              (cond.types & TypeInfo::typeArea))) {
          continue;
        }

        if (cond.condition->Evaluate(tagMap)) {
          if (wayType==typeInfoIgnore &&
              (cond.types & TypeInfo::typeWay)) {
            wayType=type;
          }

          if (areaType==typeInfoIgnore &&
              (cond.types & TypeInfo::typeArea)) {
            areaType=type;
          }

          if (wayType!=typeInfoIgnore ||
              areaType!=typeInfoIgnore) {
            return true;
          }
        }
      }
    }

    return false;
  }

  TypeInfoRef TypeConfig::GetRelationType(const TagMap& tagMap) const
  {
    if (tagMap.empty()) {
      return typeInfoIgnore;
    }

    auto relationType=tagMap.find(tagType);

    if (relationType!=tagMap.end() &&
        relationType->second=="multipolygon") {
      for (size_t i=0; i<types.size(); i++) {
        if (!types[i]->HasConditions() ||
            !types[i]->CanBeArea()) {
          continue;
        }

        for (const auto &cond : types[i]->GetConditions()) {
          if (!(cond.types & TypeInfo::typeArea)) {
            continue;
          }

          if (cond.condition->Evaluate(tagMap)) {
            return types[i];
          }
        }
      }
    }
    else {
      for (size_t i=0; i<types.size(); i++) {
        if (!types[i]->HasConditions() ||
            !types[i]->CanBeRelation()) {
          continue;
        }

        for (const auto &cond : types[i]->GetConditions()) {
          if (!(cond.types & TypeInfo::typeRelation)) {
            continue;
          }

          if (cond.condition->Evaluate(tagMap)) {
            return types[i];
          }
        }
      }
    }

    return typeInfoIgnore;
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
    auto entry=surfaceToGradeMap.find(surface);

    if (entry!=surfaceToGradeMap.end()) {
      grade=entry->second;

      return true;
    }
    else {
      return false;
    }
  }

  void TypeConfig::RegisterMaxSpeedAlias(const std::string& alias,
                                         uint8_t maxSpeed)
  {
    nameToMaxSpeedMap.insert(std::make_pair(alias,
                                            maxSpeed));
  }

  bool TypeConfig::GetMaxSpeedFromAlias(const std::string& alias,
                                        uint8_t& maxSpeed) const
  {
    auto entry=nameToMaxSpeedMap.find(alias);

    if (entry!=nameToMaxSpeedMap.end()) {
      maxSpeed=entry->second;

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
      log.Error() << "Cannot get size of file '" << filename << "'";
      return false;
    }

    file=fopen(filename.c_str(),"rb");
    if (file==NULL) {
      log.Error() << "Cannot open file '" << filename << "'";
      return false;
    }

    unsigned char* content=new unsigned char[fileSize];

    if (fread(content,1,fileSize,file)!=(size_t)fileSize) {
      log.Error() << "Cannot load file '" << filename << "'";
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
      log.Error() << "Cannot open file '" << scanner.GetFilename() << "'";
     return false;
    }

    // Tags

    uint32_t tagCount;

    if (!scanner.ReadNumber(tagCount)) {
      log.Error() << "Format error in file '" << scanner.GetFilename() << "'";
      return false;
    }

    for (size_t i=1; i<=tagCount; i++) {
      TagId       requestedId;
      TagId       actualId;
      std::string name;

      if (!(scanner.ReadNumber(requestedId) &&
            scanner.Read(name))) {
        log.Error() << "Format error in file '" << scanner.GetFilename() << "'";
        return false;
      }

      actualId=RegisterTag(name);

      if (actualId!=requestedId) {
        log.Error() << "Requested and actual tag id do not match";
        return false;
      }
    }

    // Name Tags

    uint32_t nameTagCount;

    if (!scanner.ReadNumber(nameTagCount)) {
      log.Error() << "Format error in file '" << scanner.GetFilename() << "'";
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
        log.Error() << "Format error in file '" << scanner.GetFilename() << "'";
      }

      actualId=RegisterNameTag(name,priority);

      if (actualId!=requestedId) {
        log.Error() << "Requested and actual name tag id do not match";
        return false;
      }
    }

    // Alternative Name Tags

    uint32_t nameAltTagCount;

    if (!scanner.ReadNumber(nameAltTagCount)) {
      log.Error() << "Format error in file '" << scanner.GetFilename() << "'";
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
        log.Error() << "Format error in file '" << scanner.GetFilename() << "'";
      }

      actualId=RegisterNameAltTag(name,priority);

      if (actualId!=requestedId) {
        log.Error() << "Requested and actual name alt tag id do not match";
        return false;
      }
    }

    // Types

    uint32_t typeCount;

    if (!scanner.ReadNumber(typeCount)) {
      log.Error() << "Format error in file '" << scanner.GetFilename() << "'";
      return false;
    }

    for (size_t i=1; i<=typeCount; i++) {
      std::string name;
      bool        canBeNode;
      bool        canBeWay;
      bool        canBeArea;
      bool        canBeRelation;
      bool        isPath;
      bool        canRouteFoot;
      bool        canRouteBicycle;
      bool        canRouteCar;
      bool        indexAsAddress;
      bool        indexAsLocation;
      bool        indexAsRegion;
      bool        indexAsPOI;
      bool        optimizeLowZoom;
      bool        multipolygon;
      bool        pinWay;
      bool        mergeAreas;
      bool        ignore;
      bool        ignoreSeaLand;

      if (!(scanner.Read(name) &&
            scanner.Read(canBeNode) &&
            scanner.Read(canBeWay) &&
            scanner.Read(canBeArea) &&
            scanner.Read(canBeRelation) &&
            scanner.Read(isPath) &&
            scanner.Read(canRouteFoot) &&
            scanner.Read(canRouteBicycle) &&
            scanner.Read(canRouteCar) &&
            scanner.Read(indexAsAddress) &&
            scanner.Read(indexAsLocation) &&
            scanner.Read(indexAsRegion) &&
            scanner.Read(indexAsPOI) &&
            scanner.Read(optimizeLowZoom) &&
            scanner.Read(multipolygon) &&
            scanner.Read(pinWay) &&
            scanner.Read(mergeAreas) &&
            scanner.Read(ignoreSeaLand) &&
            scanner.Read(ignore))) {

        log.Error() << "Format error in file '" << scanner.GetFilename() << "'";
        return false;
      }

      TypeInfoRef typeInfo=std::make_shared<TypeInfo>();

      typeInfo->SetType(name);

      typeInfo->CanBeNode(canBeNode);
      typeInfo->CanBeWay(canBeWay);
      typeInfo->CanBeArea(canBeArea);
      typeInfo->CanBeRelation(canBeRelation);
      typeInfo->SetIsPath(isPath);
      typeInfo->CanRouteFoot(canRouteFoot);
      typeInfo->CanRouteBicycle(canRouteBicycle);
      typeInfo->CanRouteCar(canRouteCar);
      typeInfo->SetIndexAsAddress(indexAsAddress);
      typeInfo->SetIndexAsLocation(indexAsLocation);
      typeInfo->SetIndexAsRegion(indexAsRegion);
      typeInfo->SetIndexAsPOI(indexAsPOI);
      typeInfo->SetOptimizeLowZoom(optimizeLowZoom);
      typeInfo->SetMultipolygon(multipolygon);
      typeInfo->SetPinWay(pinWay);
      typeInfo->SetMergeAreas(mergeAreas);
      typeInfo->SetIgnoreSeaLand(ignoreSeaLand);
      typeInfo->SetIgnore(ignore);

      // Type Features

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
          log.Error() << "Feature '" << featureName << "' not found";
          return false;
        }

        typeInfo->AddFeature(feature);
      }

      // Groups

      uint32_t groupCount;

      if (!scanner.ReadNumber(groupCount)) {
        return false;
      }

      for (size_t i=0; i<groupCount; i++) {
        std::string groupName;

        if (!scanner.Read(groupName)) {
          return false;
        }

        typeInfo->AddGroup(groupName);
      }

      typeInfo=RegisterType(typeInfo);
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

    if (!writer.Open(AppendFileToDir(directory,"types.dat"))) {
      //progress.Error("Cannot create 'types.dat'");
      return false;
    }

    writer.WriteNumber((uint32_t)tags.size());
    for (const auto &tag : tags) {
      writer.WriteNumber(tag.GetId());
      writer.Write(tag.GetName());
    }

    uint32_t nameTagCount=0;
    uint32_t nameAltTagCount=0;

    for (const auto &tag : tags) {
      uint32_t priority;

      if (IsNameTag(tag.GetId(),priority)) {
        nameTagCount++;
      }

      if (IsNameAltTag(tag.GetId(),priority)) {
        nameAltTagCount++;
      }
    }

    writer.WriteNumber(nameTagCount);
    for (const auto &tag : tags) {
      uint32_t priority;

      if (IsNameTag(tag.GetId(),priority)) {
        writer.WriteNumber(tag.GetId());
        writer.Write(tag.GetName());
        writer.WriteNumber((uint32_t)priority);
      }
    }

    writer.WriteNumber(nameAltTagCount);
    for (const auto &tag : tags) {
      uint32_t priority;

      if (IsNameAltTag(tag.GetId(),priority)) {
        writer.WriteNumber(tag.GetId());
        writer.Write(tag.GetName());
        writer.WriteNumber((uint32_t)priority);
      }
    }

    writer.WriteNumber((uint32_t)GetTypes().size());

    for (auto type : GetTypes()) {
      writer.Write(type->GetName());
      writer.Write(type->CanBeNode());
      writer.Write(type->CanBeWay());
      writer.Write(type->CanBeArea());
      writer.Write(type->CanBeRelation());
      writer.Write(type->IsPath());
      writer.Write(type->CanRouteFoot());
      writer.Write(type->CanRouteBicycle());
      writer.Write(type->CanRouteCar());
      writer.Write(type->GetIndexAsAddress());
      writer.Write(type->GetIndexAsLocation());
      writer.Write(type->GetIndexAsRegion());
      writer.Write(type->GetIndexAsPOI());
      writer.Write(type->GetOptimizeLowZoom());
      writer.Write(type->GetMultipolygon());
      writer.Write(type->GetPinWay());
      writer.Write(type->GetMergeAreas());
      writer.Write(type->GetIgnoreSeaLand());
      writer.Write(type->GetIgnore());

      writer.WriteNumber((uint32_t)type->GetFeatures().size());
      for (const auto &feature : type->GetFeatures()) {
        writer.Write(feature.GetFeature()->GetName());
      }

      writer.WriteNumber((uint32_t)type->GetGroups().size());
      for (const auto &groupName : type->GetGroups()) {
        writer.Write(groupName);
      }
    }

    return !writer.HasError()&&writer.Close();
  }
}
