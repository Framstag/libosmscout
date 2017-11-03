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

#include <algorithm>

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

  FeatureValue& FeatureValue::operator=(const FeatureValue& /*other*/)
  {
    assert(false);

    return *this;
  }

  /**
   * Read the value of the Feature from the FileScanner
   *
   * @throws IOException
   */
  void FeatureValue::Read(FileScanner& /*scanner*/)
  {
    assert(false);
  }

  /**
   * Write the FeatureValue to disk.
   *
   * @throws IOException.
   */
  void FeatureValue::Write(FileWriter& /*writer*/)
  {
    assert(false);
  }

  Feature::Feature()
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

  /**
   * Add a description of the feature for the given language code
   * @param languageCode
   *    language code like for example 'en'or 'de'
   * @param description
   *    description of the type
   * @return
   *    type info instance
   */
  void Feature::AddDescription(const std::string& languageCode,
                               const std::string& description)
  {
    descriptions[languageCode]=description;
  }

  /**
   * Returns the description for the given language code. Returns an empty string, if
   * no description is available for the given language code.
   *
   * @param languageCode
   *    languageCode like for example 'en' or 'de'
   * @return
   *    Description or empty string
   */
  std::string Feature::GetDescription(const std::string& languageCode) const
  {
    auto entry=descriptions.find(languageCode);

    if (entry!=descriptions.end()) {
      return entry->second;
    }
    else {
      return "";
    }
  }

  /**
   * Just to make the compiler happy :-/
   */
  FeatureInstance::FeatureInstance()
  : type(NULL),
    featureBit(0),
    index(0),
    offset(0)
  {

  }

  FeatureInstance::FeatureInstance(const FeatureRef& feature,
                                   const TypeInfo* type,
                                   size_t featureBit,
                                   size_t index,
                                   size_t offset)
  : feature(feature),
    type(type),
    featureBit(featureBit),
    index(index),
    offset(offset)
  {
    assert(feature);
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
    if (type) {
      DeleteData();
    }
    if (other.GetType()) {
      SetType(other.GetType());

      for (size_t idx=0; idx<other.GetFeatureCount(); idx++) {
        if (other.HasFeature(idx)) {
          if (other.GetFeature(idx).GetFeature()->HasValue()) {
            FeatureValue* otherValue=other.GetValue(idx);
            FeatureValue* thisValue=AllocateValue(idx);

            *thisValue=*otherValue;
          }
          else {
            size_t featureBit=GetFeature(idx).GetFeatureBit();
            size_t byteIdx=featureBit/8;

            featureBits[byteIdx]=featureBits[byteIdx] | (1 << featureBit%8);
          }
        }
      }
    }
  }

  void FeatureValueBuffer::SetType(const TypeInfoRef& type)
  {
    if (this->type) {
      DeleteData();
    }

    this->type=type;

    AllocateBits();
    featureValueBuffer=NULL; // buffer is allocated on first usage
  }

  void FeatureValueBuffer::DeleteData()
  {
    if (featureValueBuffer!=NULL) {
      for (size_t i=0; i<type->GetFeatureCount(); i++) {
        if (HasFeature(i)) {
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

  void FeatureValueBuffer::AllocateBits()
  {
    if (type && type->HasFeatures()) {
      featureBits=new uint8_t[type->GetFeatureMaskBytes()]();
    }
    else
    {
      featureBits=NULL;
    }
  }

  void FeatureValueBuffer::AllocateValueBufferLazy()
  {
    if (featureValueBuffer==NULL &&
        type &&
        type->HasFeatures()) {
      featureValueBuffer=static_cast<char*>(::operator new(type->GetFeatureValueBufferSize()));
    }
  }

  FeatureValue* FeatureValueBuffer::AllocateValue(size_t idx)
  {
    size_t featureBit=GetFeature(idx).GetFeatureBit();
    size_t byteIdx=featureBit/8;

    featureBits[byteIdx]=featureBits[byteIdx] | (1 << featureBit%8);

    if (type->GetFeature(idx).GetFeature()->HasValue()) {
      FeatureValue* value=GetValueAndAllocateBuffer(idx);

      return type->GetFeature(idx).GetFeature()->AllocateValue(value);
    }
    else {
      return NULL;
    }
  }

  void FeatureValueBuffer::FreeValue(size_t idx)
  {
    if (HasFeature(idx)) {
      if (type->GetFeature(idx).GetFeature()->HasValue()) {
          FeatureValue* value=GetValue(idx);
          value->~FeatureValue();
      }

      // clear feature bit
      size_t featureBit = GetFeature(idx).GetFeatureBit();
      size_t byteIdx = featureBit / 8;
      featureBits[byteIdx] = featureBits[byteIdx] & ~(1 << featureBit % 8);
    }
  }

  void FeatureValueBuffer::Parse(TagErrorReporter& errorReporter,
                                 const TypeConfig& typeConfig,
                                 const ObjectOSMRef& object,
                                 const TagMap& tags)
  {
    for (const auto &feature : type->GetFeatures()) {
      feature.GetFeature()->Parse(errorReporter,
                                  typeConfig,
                                  feature,
                                  object,
                                  tags,
                                  *this);
    }
  }

  /**
   * Read the FeatureValueBuffer from the given FileScanner.
   *
   * @throws IOException
   */
  void FeatureValueBuffer::Read(FileScanner& scanner)
  {
    for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
      scanner.Read(featureBits[i]);
    }
    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasFeature(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=feature.GetFeature()->AllocateValue(GetValueAndAllocateBuffer(idx));

        value->Read(scanner);
      }
    }
  }

  /**
   * Reads the FeatureValueBuffer to the given FileScanner.
   * It also reads the value of the special flag as passed to the Write method.
   *
   * @throws IOException
   */
  void FeatureValueBuffer::Read(FileScanner& scanner,
                                bool& specialFlag)
  {
    for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
      scanner.Read(featureBits[i]);
    }

    if (BitsToBytes(type->GetFeatureCount())==BitsToBytes(type->GetFeatureCount()+1)) {
      specialFlag=(featureBits[type->GetFeatureMaskBytes()-1] & 0x80)!=0;
    }
    else {
      uint8_t addByte;

      scanner.Read(addByte);

      specialFlag=(addByte & 0x80)!=0;
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasFeature(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=feature.GetFeature()->AllocateValue(GetValueAndAllocateBuffer(idx));

        value->Read(scanner);
      }
    }
  }

  /**
   * Reads the FeatureValueBuffer to the given FileScanner.
   * It also reads the value of two special flags as passed to the Write method.
   *
   * @throws IOException
   */
  void FeatureValueBuffer::Read(FileScanner& scanner,
                                bool& specialFlag1,
                                bool& specialFlag2)
  {
    for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
      scanner.Read(featureBits[i]);
    }

    if (BitsToBytes(type->GetFeatureCount())==BitsToBytes(type->GetFeatureCount()+2)) {
      specialFlag1=(featureBits[type->GetFeatureMaskBytes()-1] & 0x80)!=0;
      specialFlag2=(featureBits[type->GetFeatureMaskBytes()-1] & 0x40)!=0;
    }
    else {
      uint8_t addByte;

      scanner.Read(addByte);

      specialFlag1=(addByte & 0x80)!=0;
      specialFlag2=(addByte & 0x40)!=0;
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasFeature(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=feature.GetFeature()->AllocateValue(GetValueAndAllocateBuffer(idx));

        value->Read(scanner);
      }
    }
  }

  /**
   * Writes the FeatureValueBuffer to the given FileWriter.
   *
   * @throws IOException
   */
  void FeatureValueBuffer::Write(FileWriter& writer) const
  {
    for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
      writer.Write(featureBits[i]);
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasFeature(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=GetValue(idx);

        value->Write(writer);
      }
    }
  }

  /**
   * Writes the FeatureValueBuffer to the given FileWriter.
   * It also writes the value of the special flag passed. The flag can later be retrieved
   * by using the matching Read method.
   *
   * @throws IOException
   */
  void FeatureValueBuffer::Write(FileWriter& writer,
                                 bool specialFlag) const
  {
    if (BitsToBytes(type->GetFeatureCount())==BitsToBytes(type->GetFeatureCount()+1)) {
      if (specialFlag) {
        featureBits[type->GetFeatureMaskBytes()-1]|=0x80;
      }
      else {
        featureBits[type->GetFeatureMaskBytes()-1]&=~0x80;
      }

      for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
        writer.Write(featureBits[i]);
      }
    }
    else {
      for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
        writer.Write(featureBits[i]);
      }

      uint8_t addByte=specialFlag ? 0x80 : 0x00;

      writer.Write(addByte);
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasFeature(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=GetValue(idx);

        value->Write(writer);
      }
    }
  }

  /**
   * Writes the FeatureValueBuffer to the given FileWriter.
   * It also writes the value of the special flag passed. The flag can later be retrieved
   * by using the matching Read method.
   *
   * @throws IOException
   */
  void FeatureValueBuffer::Write(FileWriter& writer,
                                 bool specialFlag1,
                                 bool specialFlag2) const
  {
    if (BitsToBytes(type->GetFeatureCount())==BitsToBytes(type->GetFeatureCount()+2)) {
      if (specialFlag1) {
        featureBits[type->GetFeatureMaskBytes()-1]|=0x80;
      }
      else {
        featureBits[type->GetFeatureMaskBytes()-1]&=~0x80;
      }

      if (specialFlag2) {
        featureBits[type->GetFeatureMaskBytes()-1]|=0x40;
      }
      else {
        featureBits[type->GetFeatureMaskBytes()-1]&=~0x40;
      }

      for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
        writer.Write(featureBits[i]);
      }
    }
    else {
      for (size_t i=0; i<type->GetFeatureMaskBytes(); i++) {
        writer.Write(featureBits[i]);
      }

      uint8_t addByte=0;

      if (specialFlag1) {
        addByte|= 0x80;
      }

      if (specialFlag2) {
        addByte|= 0x40;
      }

      writer.Write(addByte);
    }

    for (const auto &feature : type->GetFeatures()) {
      size_t idx=feature.GetIndex();

      if (HasFeature(idx) &&
          feature.GetFeature()->HasValue()) {
        FeatureValue* value=GetValue(idx);

        value->Write(writer);
      }
    }
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
      if (HasFeature(i)!=other.HasFeature(i)) {
        return false;
      }

      // If a feature has a value, we compare the values
      if (HasFeature(i) &&
          other.HasFeature(i) &&
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

  void FeatureValueBuffer::CopyMissingValues(const FeatureValueBuffer& other)
  {
    for (size_t i=0; i<other.GetFeatureCount(); i++) {
      // Feature set?
      if (!other.HasFeature(i)) {
        continue;
      }

      std::string featureName=other.GetFeature(i).GetFeature()->GetName();
      size_t      featureIndex;

      // Does our type has this feature, too?
      if (!GetType()->GetFeature(featureName,
                                 featureIndex)) {
        continue;
      }

      // We do not overwrite existing feature values
      if (HasFeature(featureIndex)) {
        continue;
      }

      // Copy feature with/without value
      if (other.GetFeature(i).GetFeature()->HasValue()) {
        FeatureValue* otherValue=other.GetValue(i);
        FeatureValue* thisValue=AllocateValue(featureIndex);

        *thisValue=*otherValue;
      }
      else {
        size_t featureBit=GetFeature(featureIndex).GetFeatureBit();
        size_t byteIdx=featureBit/8;

        featureBits[byteIdx]=featureBits[byteIdx] | (1 << featureBit%8);
      }
    }
  }

  void FeatureValueBuffer::ClearFeatureValues()
  {
    for (size_t i=0; i<GetFeatureCount(); i++) {
      FreeValue(i);
    }
  }


  const char* TypeConfig::FILE_TYPES_DAT="types.dat";

  TypeInfo::TypeInfo(const std::string& name)
    : nodeId(0),
      wayId(0),
      areaId(0),
      name(name),
      index(0),
      internal(false),
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

  TypeInfo& TypeInfo::SetInternal()
  {
    this->internal=true;

    return *this;
  }

  TypeInfo& TypeInfo::SetType(const std::string& name)
  {
    this->name=name;

    return *this;
  }

  TypeInfo& TypeInfo::AddCondition(unsigned char types,
                                   const TagConditionRef& condition)
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
    assert(feature);
    assert(nameToFeatureMap.find(feature->GetName())==nameToFeatureMap.end());

    size_t featureBit=0;
    size_t index=0;
    size_t offset=0;
    size_t alignment=std::max(sizeof(size_t),sizeof(void*));

    if (!features.empty()) {
      featureBit=features.back().GetFeatureBit()+1+feature->GetFeatureBitCount();
      index=features.back().GetIndex()+1;
      offset=features.back().GetOffset()+features.back().GetFeature()->GetValueSize();
      if (offset%alignment!=0) {
        offset=(offset/alignment+1)*alignment;
      }
    }


    features.push_back(FeatureInstance(feature,
                                       this,
                                       featureBit,
                                       index,
                                       offset));
    nameToFeatureMap.insert(std::make_pair(feature->GetName(),index));

    size_t featureBitCount=0;

    if (!features.empty()) {
      featureBitCount=features.back().GetFeatureBit()+feature->GetFeatureBitCount()+1;
    }

    featureMaskBytes=BitsToBytes(featureBitCount);
    specialFeatureMaskBytes=BitsToBytes(featureBitCount+1);

    valueBufferSize=offset+feature->GetValueSize();

    return *this;
  }

  TypeInfo& TypeInfo::AddGroup(const std::string& groupName)
  {
    groups.insert(groupName);

    return *this;
  }

  /**
   * Add a description of the type for the given language code
   * @param languageCode
   *    language code like for example 'en'or 'de'
   * @param description
   *    description of the type
   * @return
   *    type info instance
   */
  TypeInfo& TypeInfo::AddDescription(const std::string& languageCode,
                                     const std::string& description)
  {
    descriptions[languageCode]=description;

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

  /**
   * Returns the description for the given language code. Returns an empty string, if
   * no description is available for the given language code.
   *
   * @param languageCode
   *    languageCode like for example 'en' or 'de'
   * @return
   *    Description or empty string
   */
  std::string TypeInfo::GetDescription(const std::string& languageCode) const
  {
    auto entry=descriptions.find(languageCode);

    if (entry!=descriptions.end()) {
      return entry->second;
    }
    else {
      return "";
    }
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

  TypeInfoSet::TypeInfoSet(TypeInfoSet&& other)
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

  void TypeInfoSet::Set(const TypeInfoSet& other)
  {
    types=other.types;
    count=other.count;
  }

  void TypeInfoSet::Set(const std::vector<TypeInfoRef>& types)
  {
    Clear();

    for (const auto& type : types) {
      Set(type);
    }
  }

  void TypeInfoSet::Add(const TypeInfoSet& types)
  {
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

  void TypeInfoSet::Intersection(const TypeInfoSet& otherTypes)
  {
    for (size_t i=0; i<types.size(); i++) {
      if (types[i] &&
          (i>=otherTypes.types.size() ||
          !otherTypes.types[i])) {
        types[i]=NULL;
        count--;
      }
    }
  }

  /**
   * Returns 'true' if at least one type is set in both Sets. Else
   * 'false' is returned.
   */
  bool TypeInfoSet::Intersects(const TypeInfoSet& otherTypes) const
  {
    size_t minSize=std::min(types.size(),otherTypes.types.size());

    for (size_t i=0; i<minSize; i++) {
      if (types[i] && otherTypes.types[i]) {
        return true;
      }
    }

    return false;
  }

  bool TypeInfoSet::operator==(const TypeInfoSet& other) const
  {
    if (this==&other) {
      return true;
    }

    if (count!=other.count) {
      return false;
    }

    for (size_t i=0; i<std::max(types.size(),other.types.size()); i++) {
      if (i<types.size() && i<other.types.size()) {
        if (types[i]!=other.types[i]) {
          return false;
        }
      }
      else if (i<types.size()) {
        if (types[i]) {
          return false;
        }
      }
      else if (i<other.types.size()) {
        if (other.types[i]) {
          return false;
        }
      }
    }

    return true;
  }

  bool TypeInfoSet::operator!=(const TypeInfoSet& other) const
  {
    if (this==&other) {
      return false;
    }

    if (count!=other.count) {
      return true;
    }

    for (size_t i=0; i<std::max(types.size(),other.types.size()); i++) {
      if (i<types.size() && i<other.types.size()) {
        if (types[i]!=other.types[i]) {
          return true;
        }
      }
      else if (i<types.size()) {
        if (types[i]) {
          return true;
        }
      }
      else if (i<other.types.size()) {
        if (other.types[i]) {
          return true;
        }
      }
    }

    return false;
  }

  TypeConfig::TypeConfig()
   : nextTagId(0),
     nodeTypeIdBytes(1),
     wayTypeIdBytes(1),
     areaTypeIdBits(1),
     areaTypeIdBytes(1)
  {
    log.Debug() << "TypeConfig::TypeConfig()";

    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for tagIgnore
    RegisterTag("");

    RegisterTag("area");
    RegisterTag("natural");
    RegisterTag("datapolygon");
    RegisterTag("type");
    RegisterTag("restriction");
    RegisterTag("junction");

    featureName=std::make_shared<NameFeature>();
    RegisterFeature(featureName);

    RegisterFeature(std::make_shared<NameAltFeature>());

    featureRef=std::make_shared<RefFeature>();
    RegisterFeature(featureRef);

    featureLocation=std::make_shared<LocationFeature>();
    RegisterFeature(featureLocation);

    featureAddress=std::make_shared<AddressFeature>();
    RegisterFeature(featureAddress);

    featureAccess=std::make_shared<AccessFeature>();
    RegisterFeature(featureAccess);

    featureAccessRestricted=std::make_shared<AccessRestrictedFeature>();
    RegisterFeature(featureAccessRestricted);

    featureLayer=std::make_shared<LayerFeature>();
    RegisterFeature(featureLayer);

    featureWidth=std::make_shared<WidthFeature>();
    RegisterFeature(featureWidth);

    featureMaxSpeed=std::make_shared<MaxSpeedFeature>();
    RegisterFeature(featureMaxSpeed);

    featureGrade=std::make_shared<GradeFeature>();
    RegisterFeature(featureGrade);

    RegisterFeature(std::make_shared<AdminLevelFeature>());

    featurePostalCode = std::make_shared<PostalCodeFeature>();
    RegisterFeature(featurePostalCode);

    featureWebsite = std::make_shared<WebsiteFeature>();
    RegisterFeature(featureWebsite);

    featurePhone = std::make_shared<PhoneFeature>();
    RegisterFeature(featurePhone);

    featureBridge=std::make_shared<BridgeFeature>();
    RegisterFeature(featureBridge);

    featureTunnel=std::make_shared<TunnelFeature>();
    RegisterFeature(featureTunnel);

    featureEmbankment=std::make_shared<EmbankmentFeature>();
    RegisterFeature(featureEmbankment);

    featureRoundabout=std::make_shared<RoundaboutFeature>();
    RegisterFeature(featureRoundabout);

    RegisterFeature(std::make_shared<EleFeature>());
    RegisterFeature(std::make_shared<DestinationFeature>());
    RegisterFeature(std::make_shared<BuildingFeature>());

    RegisterFeature(std::make_shared<IsInFeature>());

    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for typeIgnore
    typeInfoIgnore=std::make_shared<TypeInfo>("");
    typeInfoIgnore->SetIgnore(true);

    RegisterType(typeInfoIgnore);


    //
    // Internal type for showing routes
    //

    TypeInfoRef route=std::make_shared<TypeInfo>("_route");
    route->SetInternal().CanBeWay(true);
    RegisterType(route);

    //
    // Internal types for the land/sea/coast tiles building the base layer for map drawing
    //

    typeInfoTileLand=std::make_shared<TypeInfo>("_tile_land");
    typeInfoTileLand->SetInternal().CanBeArea(true);
    RegisterType(typeInfoTileLand);


    typeInfoTileSea=std::make_shared<TypeInfo>("_tile_sea");
    typeInfoTileSea->SetInternal().CanBeArea(true);
    RegisterType(typeInfoTileSea);


    typeInfoTileCoast=std::make_shared<TypeInfo>("_tile_coast");
    typeInfoTileCoast->SetInternal().CanBeArea(true);
    RegisterType(typeInfoTileCoast);


    typeInfoTileUnknown=std::make_shared<TypeInfo>("_tile_unknown");
    typeInfoTileUnknown->SetInternal().CanBeArea(true);
    RegisterType(typeInfoTileUnknown);


    typeInfoCoastline=std::make_shared<TypeInfo>("_tile_coastline");
    typeInfoCoastline->SetInternal().CanBeWay(true);
    RegisterType(typeInfoCoastline);

    typeInfoOSMTileBorder=std::make_shared<TypeInfo>("_osm_tile_border");
    typeInfoOSMTileBorder->SetInternal().CanBeWay(true);
    RegisterType(typeInfoOSMTileBorder);

    typeInfoOSMSubTileBorder=std::make_shared<TypeInfo>("_osm_subtile_border");
    typeInfoOSMSubTileBorder->SetInternal().CanBeWay(true);
    RegisterType(typeInfoOSMSubTileBorder);

    tagArea=GetTagId("area");
    tagNatural=GetTagId("natural");
    tagDataPolygon=GetTagId("datapolygon");
    tagType=GetTagId("type");
    tagRestriction=GetTagId("restriction");
    tagJunction=GetTagId("junction");

    assert(tagArea!=tagIgnore);
    assert(tagNatural!=tagIgnore);
    assert(tagDataPolygon!=tagIgnore);
    assert(tagType!=tagIgnore);
    assert(tagRestriction!=tagIgnore);
    assert(tagJunction!=tagIgnore);
  }

  TypeConfig::~TypeConfig()
  {
    log.Debug() << "TypeConfig::~TypeConfig()";
  }

  TagId TypeConfig::RegisterTag(const std::string& tagName)
  {
    auto mapping=stringToTagMap.find(tagName);

    if (mapping!=stringToTagMap.end()) {
      return mapping->second;
    }

    TagInfo tagInfo(nextTagId,tagName);

    nextTagId++;

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
    assert(feature);
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

    // All ways have a layer
    if (typeInfo->CanBeWay()) {
      if (!typeInfo->HasFeature(LayerFeature::NAME)) {
        typeInfo->AddFeature(featureLayer);
      }
    }

    // All that is PATH-like automatically has a number of features,
    // even if it is not routable
    if (typeInfo->CanBeWay() &&
        typeInfo->IsPath()) {
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
      if (!typeInfo->HasFeature(EmbankmentFeature::NAME)) {
        typeInfo->AddFeature(featureEmbankment);
      }
      if (!typeInfo->HasFeature(RoundaboutFeature::NAME)) {
        typeInfo->AddFeature(featureRoundabout);
      }
    }

    // Everything routable should have access information and max speed information
    if ((typeInfo->CanBeArea() ||
         typeInfo->CanBeWay()) &&
         typeInfo->CanRoute()) {
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

    // All addressable areas and nodes get the postal code, location and address feature
    if ((typeInfo->CanBeArea() ||
         typeInfo->CanBeNode()) &&
        typeInfo->GetIndexAsAddress()) {
      if (!typeInfo->HasFeature(PostalCodeFeature::NAME)) {
        typeInfo->AddFeature(featurePostalCode);
      }
      if (!typeInfo->HasFeature(LocationFeature::NAME)) {
        typeInfo->AddFeature(featureLocation);
      }
      if (!typeInfo->HasFeature(AddressFeature::NAME)) {
        typeInfo->AddFeature(featureAddress);
      }
    }

    // All ways with a name have a postal code and a location
    if (typeInfo->CanBeWay() &&
        typeInfo->HasFeature(NameFeature::NAME)) {
      if (!typeInfo->HasFeature(PostalCodeFeature::NAME)) {
        typeInfo->AddFeature(featurePostalCode);
      }
    }

    // Something that has a name and is a POI automatically gets the
    // postal code, location, address, website and phone features, too.
    if (typeInfo->HasFeature(NameFeature::NAME) &&
        typeInfo->GetIndexAsPOI()) {
      if (!typeInfo->HasFeature(PostalCodeFeature::NAME)) {
        typeInfo->AddFeature(featurePostalCode);
      }
      if (!typeInfo->HasFeature(LocationFeature::NAME)) {
        typeInfo->AddFeature(featureLocation);
      }
      if (!typeInfo->HasFeature(AddressFeature::NAME)) {
        typeInfo->AddFeature(featureAddress);
      }
      if (!typeInfo->HasFeature(WebsiteFeature::NAME)) {
        typeInfo->AddFeature(featureWebsite);
      }
      if (!typeInfo->HasFeature(PhoneFeature::NAME)) {
        typeInfo->AddFeature(featurePhone);
      }
    }

    typeInfo->SetIndex(types.size());

    types.push_back(typeInfo);

    if (!typeInfo->GetIgnore() &&
        !typeInfo->IsInternal() &&
        (typeInfo->CanBeNode() ||
         typeInfo->CanBeWay() ||
         typeInfo->CanBeArea())) {
      if (typeInfo->CanBeNode()) {
        typeInfo->SetNodeId((TypeId)(nodeTypes.size()+1));
        nodeTypes.push_back(typeInfo);

        nodeTypeIdBytes=BytesNeededToEncodeNumber(typeInfo->GetNodeId());
      }

      if (typeInfo->CanBeWay()) {
        typeInfo->SetWayId((TypeId)(wayTypes.size()+1));
        wayTypes.push_back(typeInfo);

        wayTypeIdBytes=BytesNeededToEncodeNumber(typeInfo->GetWayId());
      }

      if (typeInfo->CanBeArea()) {
        typeInfo->SetAreaId((TypeId)(areaTypes.size()+1));
        areaTypes.push_back(typeInfo);

        areaTypeIdBytes=BytesNeededToEncodeNumber(typeInfo->GetAreaId());
        areaTypeIdBits=BitsNeededToEncodeNumber(typeInfo->GetAreaId());
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
      return (TypeId)types.size();
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

  TagId TypeConfig::GetTagId(const std::string& name) const
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

    for (const auto& type : types) {
      if (!((type->CanBeWay() ||
             type->CanBeArea()) &&
             type->HasConditions())) {
        continue;
      }

      for (const auto& cond : type->GetConditions()) {
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
    bool success=false;

    try {
      fileSize=GetFileSize(filename);

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
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
    }

    return success;
  }

  /**
   * Loads the type configuration from the given binary data file.
   *
   * Note:
   * Make sure that you load from file only onto a freshly initialized
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
    StopClock timer;

    FileScanner scanner;

    try {
      scanner.Open(AppendFileToDir(directory,
                                   "types.dat"),
                   FileScanner::Sequential,
                   true);

      uint32_t fileFormatVersion;

      scanner.Read(fileFormatVersion);

      if (fileFormatVersion!=FILE_FORMAT_VERSION) {
        log.Error() << "File '" << scanner.GetFilename() << "' does not have the expected format version! Actual " << fileFormatVersion << ", expected: " << FILE_FORMAT_VERSION;
        return false;
      }

      // Features
      uint32_t featureCount;

      scanner.ReadNumber(featureCount);

      for (uint32_t f=1; f<=featureCount; f++) {
        std::string featureName;
        uint32_t    descriptionCount;
        FeatureRef  feature;

        scanner.Read(featureName);
        scanner.ReadNumber(descriptionCount);

        feature=GetFeature(featureName);

        for (uint32_t d=1; d<=descriptionCount; d++) {
          std::string languageCode;
          std::string description;

          scanner.Read(languageCode);
          scanner.Read(description);

          if (feature) {
            feature->AddDescription(languageCode,
                                   description);
          }
        }
      }

      // Types

      uint32_t typeCount;

      scanner.ReadNumber(typeCount);

      for (uint32_t i=1; i<=typeCount; i++) {
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

        scanner.Read(name);
        scanner.Read(canBeNode);
        scanner.Read(canBeWay);
        scanner.Read(canBeArea);
        scanner.Read(canBeRelation);
        scanner.Read(isPath);
        scanner.Read(canRouteFoot);
        scanner.Read(canRouteBicycle);
        scanner.Read(canRouteCar);
        scanner.Read(indexAsAddress);
        scanner.Read(indexAsLocation);
        scanner.Read(indexAsRegion);
        scanner.Read(indexAsPOI);
        scanner.Read(optimizeLowZoom);
        scanner.Read(multipolygon);
        scanner.Read(pinWay);
        scanner.Read(mergeAreas);
        scanner.Read(ignoreSeaLand);
        scanner.Read(ignore);

        TypeInfoRef typeInfo=std::make_shared<TypeInfo>(name);

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

        scanner.ReadNumber(featureCount);

        for (uint32_t f=0; f<featureCount; f++) {
          std::string featureName;

          scanner.Read(featureName);

          FeatureRef feature=GetFeature(featureName);

          if (!feature) {
            log.Error() << "Feature '" << featureName << "' not found";
            return false;
          }

          typeInfo->AddFeature(feature);
        }

        // Groups

        uint32_t groupCount;

        scanner.ReadNumber(groupCount);

        for (uint32_t g=0; g<groupCount; g++) {
          std::string groupName;

          scanner.Read(groupName);

          typeInfo->AddGroup(groupName);
        }

        // Descriptions

        uint32_t descriptionCount;

        scanner.ReadNumber(descriptionCount);

        for (uint32_t d=1; d<=descriptionCount; d++) {
          std::string languageCode;
          std::string description;

          scanner.Read(languageCode);
          scanner.Read(description);

          typeInfo->AddDescription(languageCode,
                                   description);
        }

        RegisterType(typeInfo);
      }

      scanner.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    timer.Stop();

    log.Debug() << "Opening TypeConfig: " << timer.ResultString();

    return true;
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

    try {
      writer.Open(AppendFileToDir(directory,"types.dat"));

      writer.Write(FILE_FORMAT_VERSION);

      uint32_t typeCount=0;
      uint32_t featureCount=0;

      for (auto type : GetTypes()) {
        if (!type->IsInternal()) {
          typeCount++;
        }
      }

      for (auto feature : GetFeatures()) {
        if (!feature->GetDescriptions().empty()) {
          featureCount++;
        }
      }

      writer.WriteNumber(featureCount);

      for (auto feature : GetFeatures()) {
        if (feature->GetDescriptions().empty()) {
          continue;
        }

        writer.Write(feature->GetName());
        writer.WriteNumber((uint32_t)feature->GetDescriptions().size());
        for (const auto& descriptionEntry : feature->GetDescriptions()) {
          writer.Write(descriptionEntry.first);
          writer.Write(descriptionEntry.second);
        }
      }

      writer.WriteNumber(typeCount);

      for (auto type : GetTypes()) {
        if (type->IsInternal()) {
          continue;
        }

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
        for (const auto& feature : type->GetFeatures()) {
          writer.Write(feature.GetFeature()->GetName());
        }

        writer.WriteNumber((uint32_t)type->GetGroups().size());
        for (const auto& groupName : type->GetGroups()) {
          writer.Write(groupName);
        }

        writer.WriteNumber((uint32_t)type->GetDescriptions().size());
        for (const auto& descriptionEntry : type->GetDescriptions()) {
          writer.Write(descriptionEntry.first);
          writer.Write(descriptionEntry.second);
        }
      }

      writer.Close();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      writer.CloseFailsafe();
      return false;
    }

    return true;
  }
}
