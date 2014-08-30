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

  bool TagNotCondition::Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const
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

  bool TagBoolCondition::Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const
  {
    switch (type) {
    case boolAnd:
      for (const auto &condition : conditions) {
        if (!condition->Evaluate(tagMap)) {
          return false;
        }
      }

      return true;
    case boolOr:
      for (const auto &condition : conditions) {
        if (condition->Evaluate(tagMap)) {
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

  bool TagExistsCondition::Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const
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

  bool TagBinaryCondition::Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const
  {
    auto t=tagMap.find(tag);

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

  bool TagIsInCondition::Evaluate(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const
  {
    auto t=tagMap.find(tag);

    if (t==tagMap.end()) {
      return false;
    }

    return tagValues.find(t->second)!=tagValues.end();
  }

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

  bool NameFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(name);
  }

  bool NameFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(name);
  }


  FeatureValue& NameFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const NameFeatureValue& otherValue=static_cast<const NameFeatureValue&>(other);

      name=otherValue.name;
    }

    return *this;
  }

  bool NameFeatureValue::operator==(const FeatureValue& other) const
  {
    const NameFeatureValue& otherValue=static_cast<const NameFeatureValue&>(other);

    return name==otherValue.name;
  }

  const char* const NameFeature::NAME = "Name";

  void NameFeature::Initialize(TypeConfig& /*typeConfig*/)
  {
    // no code
  }

  std::string NameFeature::GetName() const
  {
    return NAME;
  }

  size_t NameFeature::GetValueSize() const
  {
    return sizeof(NameFeatureValue);
  }

  FeatureValue* NameFeature::AllocateValue(void* buffer)
  {
    return new (buffer) NameFeatureValue();
  }

  void NameFeature::Parse(Progress& /*progress*/,
                          const TypeConfig& typeConfig,
                          const FeatureInstance& feature,
                          const ObjectOSMRef& /*object*/,
                          const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                          FeatureValueBuffer& buffer) const
  {
    std::string name;
    uint32_t    namePriority=0;

    for (const auto &tag : tags) {
      uint32_t ntPrio;
      bool     isNameTag=typeConfig.IsNameTag(tag.first,ntPrio);

      if (isNameTag &&
          (name.empty() || ntPrio>namePriority)) {
        name=tag.second;
        namePriority=ntPrio;
      }
    }

    if (!name.empty()) {
      NameFeatureValue* value=static_cast<NameFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetName(name);
    }
  }

  bool NameAltFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(nameAlt);
  }

  bool NameAltFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(nameAlt);
  }

  FeatureValue& NameAltFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const NameAltFeatureValue& otherValue=static_cast<const NameAltFeatureValue&>(other);

      nameAlt=otherValue.nameAlt;
    }

    return *this;
  }

  bool NameAltFeatureValue::operator==(const FeatureValue& other) const
  {
    const NameAltFeatureValue& otherValue=static_cast<const NameAltFeatureValue&>(other);

    return nameAlt==otherValue.nameAlt;
  }

  const char* const NameAltFeature::NAME = "NameAlt";

  void NameAltFeature::Initialize(TypeConfig& /*typeConfig*/)
  {
    // no code
  }

  std::string NameAltFeature::GetName() const
  {
    return NAME;
  }

  size_t NameAltFeature::GetValueSize() const
  {
    return sizeof(NameAltFeatureValue);
  }

  FeatureValue* NameAltFeature::AllocateValue(void* buffer)
  {
    return new (buffer) NameAltFeatureValue();
  }

  void NameAltFeature::Parse(Progress& /*progress*/,
                             const TypeConfig& typeConfig,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                             FeatureValueBuffer& buffer) const
  {
    std::string nameAlt;
    uint32_t    nameAltPriority=0;

    for (const auto &tag : tags) {
      uint32_t natPrio;
      bool     isNameAltTag=typeConfig.IsNameAltTag(tag.first,natPrio);

      if (isNameAltTag &&
          (nameAlt.empty() || natPrio>nameAltPriority)) {
        nameAlt=tag.second;
        nameAltPriority=natPrio;
      }
    }

    if (!nameAlt.empty()) {
      NameAltFeatureValue* value=static_cast<NameAltFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetNameAlt(nameAlt);
    }
  }

  bool RefFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(ref);
  }

  bool RefFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(ref);
  }

  FeatureValue& RefFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const RefFeatureValue& otherValue=static_cast<const RefFeatureValue&>(other);

      ref=otherValue.ref;
    }

    return *this;
  }

  bool RefFeatureValue::operator==(const FeatureValue& other) const
  {
    const RefFeatureValue& otherValue=static_cast<const RefFeatureValue&>(other);

    return ref==otherValue.ref;
  }

  const char* const RefFeature::NAME = "Ref";

  void RefFeature::Initialize(TypeConfig& typeConfig)
  {
    tagRef=typeConfig.RegisterTag("ref");
  }

  std::string RefFeature::GetName() const
  {
    return NAME;
  }

  size_t RefFeature::GetValueSize() const
  {
    return sizeof(RefFeatureValue);
  }

  FeatureValue* RefFeature::AllocateValue(void* buffer)
  {
    return new (buffer) RefFeatureValue();
  }

  void RefFeature::Parse(Progress& /*progress*/,
                         const TypeConfig& /*typeConfig*/,
                         const FeatureInstance& feature,
                         const ObjectOSMRef& /*object*/,
                         const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                         FeatureValueBuffer& buffer) const
  {
    auto ref=tags.find(tagRef);

    if (ref!=tags.end() &&
        !ref->second.empty()) {
      RefFeatureValue* value=static_cast<RefFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetRef(ref->second);
    }
  }

  bool LocationFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(location);
  }

  bool LocationFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(location);
  }

  FeatureValue& LocationFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const LocationFeatureValue& otherValue=static_cast<const LocationFeatureValue&>(other);

      location=otherValue.location;
    }

    return *this;
  }

  bool LocationFeatureValue::operator==(const FeatureValue& other) const
  {
    const LocationFeatureValue& otherValue=static_cast<const LocationFeatureValue&>(other);

    return location==otherValue.location;
  }

  const char* const LocationFeature::NAME = "Location";

  void LocationFeature::Initialize(TypeConfig& typeConfig)
  {
    tagAddrHouseNr=typeConfig.RegisterTag("addr:housenumber");
    tagAddrStreet=typeConfig.RegisterTag("addr:street");
  }

  std::string LocationFeature::GetName() const
  {
    return NAME;
  }

  size_t LocationFeature::GetValueSize() const
  {
    return sizeof(LocationFeatureValue);
  }

  FeatureValue* LocationFeature::AllocateValue(void* buffer)
  {
    return new (buffer) LocationFeatureValue();
  }

  void LocationFeature::Parse(Progress& /*progress*/,
                              const TypeConfig& /*typeConfig*/,
                              const FeatureInstance& feature,
                              const ObjectOSMRef& /*object*/,
                              const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                              FeatureValueBuffer& buffer) const
  {
    auto street=tags.find(tagAddrStreet);

    if (street==tags.end()) {
      return;
    }

    auto houseNr=tags.find(tagAddrHouseNr);

    if (houseNr==tags.end()) {
      return;
    }

    if (!street->second.empty() &&
        !houseNr->second.empty()) {
      LocationFeatureValue* value=static_cast<LocationFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetLocation(street->second);
    }
  }

  bool AddressFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(address);
  }

  bool AddressFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(address);
  }

  FeatureValue& AddressFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const AddressFeatureValue& otherValue=static_cast<const AddressFeatureValue&>(other);

      address=otherValue.address;
    }

    return *this;
  }

  bool AddressFeatureValue::operator==(const FeatureValue& other) const
  {
    const AddressFeatureValue& otherValue=static_cast<const AddressFeatureValue&>(other);

    return address==otherValue.address;
  }

  const char* const AddressFeature::NAME = "Address";

  void AddressFeature::Initialize(TypeConfig& typeConfig)
  {
    tagAddrHouseNr=typeConfig.RegisterTag("addr:housenumber");
    tagAddrStreet=typeConfig.RegisterTag("addr:street");
  }

  std::string AddressFeature::GetName() const
  {
    return NAME;
  }

  size_t AddressFeature::GetValueSize() const
  {
    return sizeof(AddressFeatureValue);
  }

  FeatureValue* AddressFeature::AllocateValue(void* buffer)
  {
    return new (buffer) AddressFeatureValue();
  }

  void AddressFeature::Parse(Progress& /*progress*/,
                             const TypeConfig& /*typeConfig*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                             FeatureValueBuffer& buffer) const
  {
    auto street=tags.find(tagAddrStreet);

    if (street==tags.end()) {
      return;
    }

    auto houseNr=tags.find(tagAddrHouseNr);

    if (houseNr==tags.end()) {
      return;
    }

    if (!street->second.empty() &&
        !houseNr->second.empty()) {
      AddressFeatureValue* value=static_cast<AddressFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetAddress(houseNr->second);
    }
  }

  bool AccessFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(access);
  }

  bool AccessFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(access);
  }

  FeatureValue& AccessFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const AccessFeatureValue& otherValue=static_cast<const AccessFeatureValue&>(other);

      access=otherValue.access;
    }

    return *this;
  }

  bool AccessFeatureValue::operator==(const FeatureValue& other) const
  {
    const AccessFeatureValue& otherValue=static_cast<const AccessFeatureValue&>(other);

    return access==otherValue.access;
  }

  const char* const AccessFeature::NAME = "Access";

  void AccessFeature::Initialize(TypeConfig& typeConfig)
  {
    tagOneway=typeConfig.RegisterTag("oneway");
    tagJunction=typeConfig.RegisterTag("junction");

    tagAccess=typeConfig.RegisterTag("access");
    tagAccessForward=typeConfig.RegisterTag("access:foward");
    tagAccessBackward=typeConfig.RegisterTag("access:backward");

    tagAccessFoot=typeConfig.RegisterTag("access:foot");
    tagAccessFootForward=typeConfig.RegisterTag("access:foot:foward");
    tagAccessFootBackward=typeConfig.RegisterTag("access:foot:backward");

    tagAccessBicycle=typeConfig.RegisterTag("access:bicycle");
    tagAccessBicycleForward=typeConfig.RegisterTag("access:bicycle:foward");
    tagAccessBicycleBackward=typeConfig.RegisterTag("access:bicycle:backward");

    tagAccessMotorVehicle=typeConfig.RegisterTag("access:motor_vehicle");
    tagAccessMotorVehicleForward=typeConfig.RegisterTag("access:motor_vehicle:foward");
    tagAccessMotorVehicleBackward=typeConfig.RegisterTag("access:motor_vehicle:backward");

    tagAccessMotorcar=typeConfig.RegisterTag("access:motorcar");
    tagAccessMotorcarForward=typeConfig.RegisterTag("access:motorcar:foward");
    tagAccessMotorcarBackward=typeConfig.RegisterTag("access:motorcar:backward");
  }

  std::string AccessFeature::GetName() const
  {
    return NAME;
  }

  size_t AccessFeature::GetValueSize() const
  {
    return sizeof(AccessFeatureValue);
  }

  FeatureValue* AccessFeature::AllocateValue(void* buffer)
  {
    return new (buffer) AccessFeatureValue();
  }

  void AccessFeature::Parse(Progress& /*progress*/,
                            const TypeConfig& /*typeConfig*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                            FeatureValueBuffer& buffer) const
  {
    uint8_t access=0;

    if (feature.GetType()->CanRouteFoot()) {
      access|=(AccessFeatureValue::footForward|AccessFeatureValue::footBackward);
    }

    if (feature.GetType()->CanRouteBicycle()) {
      access|=(AccessFeatureValue::bicycleForward|AccessFeatureValue::bicycleBackward);
    }

    if (feature.GetType()->CanRouteCar()) {
      access|=(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);
    }

    uint8_t defaultAccess=access;

    // Flag access

    auto accessValue=tags.find(tagAccess);

    if (accessValue!=tags.end()) {
      access=0;

      if (!(accessValue->second=="no")) {
        access=(AccessFeatureValue::footForward|AccessFeatureValue::footBackward|AccessFeatureValue::bicycleForward|AccessFeatureValue::bicycleBackward|AccessFeatureValue::carForward|AccessFeatureValue::carBackward);
      }
    }

    // Flag access:forward/access:backward

    auto accessForwardValue=tags.find(tagAccessForward);
    auto accessBackwardValue=tags.find(tagAccessBackward);

    if (accessForwardValue!=tags.end()) {
      access&=~(AccessFeatureValue::footForward|AccessFeatureValue::bicycleForward|AccessFeatureValue::carForward);

      if (!(accessForwardValue->second=="no")) {
        access|=(AccessFeatureValue::footForward|AccessFeatureValue::bicycleForward|AccessFeatureValue::carForward);
      }
    }
    else if (accessBackwardValue!=tags.end()) {
      access&=~(AccessFeatureValue::footBackward|AccessFeatureValue::bicycleBackward|AccessFeatureValue::carBackward);

      if (!(accessBackwardValue->second=="no")) {
        access|=(AccessFeatureValue::footBackward|AccessFeatureValue::bicycleBackward|AccessFeatureValue::carBackward);
      }
    }

    // Flags access:foot, access:bicycle, access:motor_vehicle, access:motorcar

    auto accessFootValue=tags.find(tagAccessFoot);
    auto accessBicycleValue=tags.find(tagAccessBicycle);
    auto accessMotorVehicleValue=tags.find(tagAccessMotorVehicle);
    auto accessMotorcarValue=tags.find(tagAccessMotorcar);

    if (accessFootValue!=tags.end()) {
      access&=~(AccessFeatureValue::footForward|AccessFeatureValue::footBackward);
      if (!(accessFootValue->second=="no")) {
        access|=(AccessFeatureValue::footForward|AccessFeatureValue::footBackward);
      }
    }
    else if (accessBicycleValue!=tags.end()) {
      access&=~(AccessFeatureValue::bicycleForward|AccessFeatureValue::bicycleBackward);

      if (!(accessBicycleValue->second=="no")) {
        if (!access & AccessFeatureValue::onewayBackward) {
          access|=(AccessFeatureValue::bicycleForward);
        }
        if (!access & AccessFeatureValue::onewayForward) {
          access|=(AccessFeatureValue::bicycleBackward);
        }
      }
    }
    else if (accessMotorVehicleValue!=tags.end()) {
      access&=~(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);

      if (!(accessMotorVehicleValue->second=="no")) {
        if (!access & AccessFeatureValue::onewayBackward) {
          access|=(AccessFeatureValue::carForward);
        }
        if (!access & AccessFeatureValue::onewayForward) {
          access|=(AccessFeatureValue::carBackward);
        }
      }
    }
    else if (accessMotorcarValue!=tags.end()) {
      access&=~(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);

      if (!(accessMotorcarValue->second=="no")) {
        if (!access & AccessFeatureValue::onewayBackward) {
          access|=(AccessFeatureValue::carForward);
        }
        if (!access & AccessFeatureValue::onewayForward) {
          access|=(AccessFeatureValue::carBackward);
        }
      }
    }

    // Flags access:foot::forward/access:foot::backward,
    //       access:bicycle::forward/access:bicycle::backward,
    //       access:motor_vehicle::forward/access:motor_vehicle::backward,
    //       access:motorcar::forward/access:motorcar::backward

    auto accessFootForwardValue=tags.find(tagAccessFootForward);
    auto accessFootBackwardValue=tags.find(tagAccessFootBackward);
    auto accessBicycleForwardValue=tags.find(tagAccessBicycleForward);
    auto accessBicycleBackwardValue=tags.find(tagAccessBicycleBackward);
    auto accessMotorVehicleForwardValue=tags.find(tagAccessMotorVehicleForward);
    auto accessMotorVehicleBackwardValue=tags.find(tagAccessMotorVehicleBackward);
    auto accessMotorcarForwardValue=tags.find(tagAccessMotorcarForward);
    auto accessMotorcarBackwardValue=tags.find(tagAccessMotorcarBackward);

    if (accessFootForwardValue!=tags.end()) {
      ParseAccessFlag(accessFootForwardValue->second,
                      access,
                      AccessFeatureValue::footForward);
    }

    if (accessFootBackwardValue!=tags.end()) {
      ParseAccessFlag(accessFootBackwardValue->second,
                      access,
                      AccessFeatureValue::footBackward);
    }

    if (accessBicycleForwardValue!=tags.end()) {
      ParseAccessFlag(accessBicycleForwardValue->second,
                      access,
                      AccessFeatureValue::bicycleForward);
    }

    if (accessBicycleBackwardValue!=tags.end()) {
      ParseAccessFlag(accessBicycleBackwardValue->second,
                      access,
                      AccessFeatureValue::bicycleBackward);
    }

    if (accessMotorVehicleForwardValue!=tags.end()) {
      ParseAccessFlag(accessMotorVehicleForwardValue->second,
                      access,
                      AccessFeatureValue::carForward);
    }

    if (accessMotorVehicleBackwardValue!=tags.end()) {
      ParseAccessFlag(accessMotorVehicleBackwardValue->second,
                      access,
                      AccessFeatureValue::carBackward);
    }

    if (accessMotorcarForwardValue!=tags.end()) {
      ParseAccessFlag(accessMotorcarForwardValue->second,
                      access,
                      AccessFeatureValue::carForward);
    }

    if (accessMotorcarBackwardValue!=tags.end()) {
      ParseAccessFlag(accessMotorcarBackwardValue->second,
                      access,
                      AccessFeatureValue::carBackward);
    }

    auto onewayValue=tags.find(tagOneway);
    auto junctionValue=tags.find(tagJunction);

    if (onewayValue!=tags.end()) {
      if (onewayValue->second=="-1") {
        access&=~(AccessFeatureValue::bicycleForward|AccessFeatureValue::carForward|AccessFeatureValue::onewayForward);
        access|=AccessFeatureValue::onewayBackward;
      }
      else if (!(onewayValue->second=="no" || onewayValue->second=="false" || onewayValue->second=="0")) {
        access&=~(AccessFeatureValue::bicycleBackward|AccessFeatureValue::carBackward|AccessFeatureValue::onewayBackward);
        access|=AccessFeatureValue::onewayForward;
      }
    }
    else if (junctionValue!=tags.end()
             && junctionValue->second=="roundabout") {
      access&=~(AccessFeatureValue::bicycleBackward|AccessFeatureValue::carBackward|AccessFeatureValue::onewayBackward);
      access|=(AccessFeatureValue::bicycleForward|AccessFeatureValue::carForward|AccessFeatureValue::onewayForward);
    }

    if (access!=defaultAccess) {
      AccessFeatureValue* value=static_cast<AccessFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetAccess(access);
    }
  }

  bool LayerFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(layer);
  }

  bool LayerFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(layer);
  }

  FeatureValue& LayerFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const LayerFeatureValue& otherValue=static_cast<const LayerFeatureValue&>(other);

      layer=otherValue.layer;
    }

    return *this;
  }

  bool LayerFeatureValue::operator==(const FeatureValue& other) const
  {
    const LayerFeatureValue& otherValue=static_cast<const LayerFeatureValue&>(other);

    return layer==otherValue.layer;
  }

  const char* const LayerFeature::NAME = "Layer";

  void LayerFeature::Initialize(TypeConfig& typeConfig)
  {
    tagLayer=typeConfig.RegisterTag("layer");
  }

  std::string LayerFeature::GetName() const
  {
    return NAME;
  }

  size_t LayerFeature::GetValueSize() const
  {
    return sizeof(LayerFeatureValue);
  }

  FeatureValue* LayerFeature::AllocateValue(void* buffer)
  {
    return new (buffer) LayerFeatureValue();
  }

  void LayerFeature::Parse(Progress& progress,
                           const TypeConfig& /*typeConfig*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                           FeatureValueBuffer& buffer) const
  {
    auto layer=tags.find(tagLayer);

    if (layer!=tags.end()) {
      int8_t layerValue;

      if (StringToNumber(layer->second,layerValue)) {
        if (layerValue!=0) {
          LayerFeatureValue* value=static_cast<LayerFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

          value->SetLayer(layerValue);
        }
      }
      else {
        progress.Warning(std::string("Layer tag value '")+layer->second+"' for "+object.GetName()+" is not numeric!");
      }
    }
  }

  bool WidthFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(width);
  }

  bool WidthFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(width);
  }

  FeatureValue& WidthFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const WidthFeatureValue& otherValue=static_cast<const WidthFeatureValue&>(other);

      width=otherValue.width;
    }

    return *this;
  }

  bool WidthFeatureValue::operator==(const FeatureValue& other) const
  {
    const WidthFeatureValue& otherValue=static_cast<const WidthFeatureValue&>(other);

    return width==otherValue.width;
  }

  const char* const WidthFeature::NAME = "Width";

  void WidthFeature::Initialize(TypeConfig& typeConfig)
  {
    tagWidth=typeConfig.RegisterTag("width");
  }

  std::string WidthFeature::GetName() const
  {
    return NAME;
  }

  size_t WidthFeature::GetValueSize() const
  {
    return sizeof(WidthFeatureValue);
  }

  FeatureValue* WidthFeature::AllocateValue(void* buffer)
  {
    return new (buffer) WidthFeatureValue();
  }

  void WidthFeature::Parse(Progress& progress,
                           const TypeConfig& /*typeConfig*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                           FeatureValueBuffer& buffer) const
  {
    auto width=tags.find(tagWidth);

    if (width==tags.end()) {
      return;
    }

    std::string widthString=width->second;
    double      w;
    size_t      pos=0;
    size_t      count=0;

    // We expect that float values use '.' as separator, but many values use ',' instead.
    // Try try fix this if string looks reasonable
    for (size_t i=0; i<widthString.length() && count<=1; i++) {
      if (widthString[i]==',') {
        pos=i;
        count++;
      }
    }

    if (count==1) {
      widthString[pos]='.';
    }

    // Some width tagvalues add an 'm' to hint that the unit is meter, remove it.
    if (widthString.length()>=2) {
      if (widthString[widthString.length()-1]=='m' &&
          ((widthString[widthString.length()-2]>='0' &&
            widthString[widthString.length()-2]<='9') ||
            widthString[widthString.length()-2]<=' ')) {
        widthString.erase(widthString.length()-1);
      }

      // Trim possible trailing spaces
      while (widthString.length()>0 &&
             widthString[widthString.length()-1]==' ') {
        widthString.erase(widthString.length()-1);
      }
    }

    if (!StringToNumber(widthString,w)) {
      progress.Warning(std::string("Width tag value '")+width->second+"' for "+object.GetName()+" is no double!");
    }
    else if (w<0 && w>255.5) {
      progress.Warning(std::string("Width tag value '")+width->second+"' for "+object.GetName()+" value is too small or too big!");
    }
    else {
      WidthFeatureValue* value=static_cast<WidthFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetWidth((uint8_t)floor(w+0.5));
    }

  }

  bool MaxSpeedFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(maxSpeed);
  }

  bool MaxSpeedFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(maxSpeed);
  }

  FeatureValue& MaxSpeedFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const MaxSpeedFeatureValue& otherValue=static_cast<const MaxSpeedFeatureValue&>(other);

      maxSpeed=otherValue.maxSpeed;
    }

    return *this;
  }

  bool MaxSpeedFeatureValue::operator==(const FeatureValue& other) const
  {
    const MaxSpeedFeatureValue& otherValue=static_cast<const MaxSpeedFeatureValue&>(other);

    return maxSpeed==otherValue.maxSpeed;
  }

  const char* const MaxSpeedFeature::NAME = "MaxSpeed";

  void MaxSpeedFeature::Initialize(TypeConfig& typeConfig)
  {
    tagMaxSpeed=typeConfig.RegisterTag("maxspeed");
  }

  std::string MaxSpeedFeature::GetName() const
  {
    return NAME;
  }

  size_t MaxSpeedFeature::GetValueSize() const
  {
    return sizeof(MaxSpeedFeatureValue);
  }

  FeatureValue* MaxSpeedFeature::AllocateValue(void* buffer)
  {
    return new (buffer) MaxSpeedFeatureValue();
  }

  void MaxSpeedFeature::Parse(Progress& progress,
                              const TypeConfig& /*typeConfig*/,
                              const FeatureInstance& feature,
                              const ObjectOSMRef& object,
                              const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                              FeatureValueBuffer& buffer) const
  {
    auto maxSpeed=tags.find(tagMaxSpeed);

    if (maxSpeed==tags.end()) {
      return;
    }

    std::string valueString=maxSpeed->second;
    size_t      valueNumeric;
    bool        isMph=false;

    if (valueString=="signals") {
      return;
    }

    if (valueString=="none") {
      return;
    }

    // "walk" should not be used, but we provide an estimation anyway,
    // since it is likely still better than the default
    if (valueString=="walk") {
      MaxSpeedFeatureValue* value=static_cast<MaxSpeedFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetMaxSpeed(10);

      return;
    }

    size_t pos;

    pos=valueString.rfind("mph");
    if (pos!=std::string::npos) {
      valueString.erase(pos);
      isMph=true;
    }

    while (valueString.length()>0 && valueString[valueString.length()-1]==' ') {
      valueString.erase(valueString.length()-1);
    }

    if (!StringToNumber(valueString,valueNumeric)) {
      progress.Warning(std::string("Max speed tag value '")+maxSpeed->second+"' for "+object.GetName()+" is not numeric!");
      return;
    }

    MaxSpeedFeatureValue* value=static_cast<MaxSpeedFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

    if (isMph) {
      if (valueNumeric>std::numeric_limits<uint8_t>::max()/1.609+0.5) {

        value->SetMaxSpeed(std::numeric_limits<uint8_t>::max());
      }
      else {
        value->SetMaxSpeed((uint8_t)(valueNumeric*1.609+0.5));
      }
    }
    else {
      if (valueNumeric>std::numeric_limits<uint8_t>::max()) {
        value->SetMaxSpeed(std::numeric_limits<uint8_t>::max());
      }
      else {
        value->SetMaxSpeed(valueNumeric);
      }
    }
  }

  bool GradeFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(grade);
  }

  bool GradeFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(grade);
  }

  FeatureValue& GradeFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const GradeFeatureValue& otherValue=static_cast<const GradeFeatureValue&>(other);

      grade=otherValue.grade;
    }

    return *this;
  }

  bool GradeFeatureValue::operator==(const FeatureValue& other) const
  {
    const GradeFeatureValue& otherValue=static_cast<const GradeFeatureValue&>(other);

    return grade==otherValue.grade;
  }

  const char* const GradeFeature::NAME = "Grade";

  void GradeFeature::Initialize(TypeConfig& typeConfig)
  {
    tagSurface=typeConfig.RegisterTag("surface");
    tagTrackType=typeConfig.RegisterTag("tracktype");
  }

  std::string GradeFeature::GetName() const
  {
    return NAME;
  }

  size_t GradeFeature::GetValueSize() const
  {
    return sizeof(GradeFeatureValue);
  }

  FeatureValue* GradeFeature::AllocateValue(void* buffer)
  {
    return new (buffer) GradeFeatureValue();
  }

  void GradeFeature::Parse(Progress& progress,
                           const TypeConfig& typeConfig,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                           FeatureValueBuffer& buffer) const
  {
    auto tracktype=tags.find(tagTrackType);

    if (tracktype!=tags.end()) {
      if (tracktype->second=="grade1") {
        GradeFeatureValue* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(1);

        return;
      }
      else if (tracktype->second=="grade2") {
        GradeFeatureValue* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(2);

        return;
      }
      else if (tracktype->second=="grade3") {
        GradeFeatureValue* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(3);

        return;
      }
      else if (tracktype->second=="grade4") {
        GradeFeatureValue* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(4);

        return;
      }
      else if (tracktype->second=="grade5") {
        GradeFeatureValue* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(5);

        return;
      }
      else {
        progress.Warning(std::string("Unsupported tracktype value '")+tracktype->second+"' for "+object.GetName());
      }
    }

    auto surface=tags.find(tagSurface);

    if (surface!=tags.end()) {
      size_t grade;

      if (typeConfig.GetGradeForSurface(surface->second,
                                        grade)) {
        GradeFeatureValue* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade((uint8_t)grade);
      }
      else {
        progress.Warning(std::string("Unknown surface type '")+surface->second+"' for "+object.GetName()+"!");
      }
    }
  }

  bool AdminLevelFeatureValue::Read(FileScanner& scanner)
  {
    return scanner.Read(adminLevel);
  }

  bool AdminLevelFeatureValue::Write(FileWriter& writer)
  {
    return writer.Write(adminLevel);
  }

  FeatureValue& AdminLevelFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const AdminLevelFeatureValue& otherValue=static_cast<const AdminLevelFeatureValue&>(other);

      adminLevel=otherValue.adminLevel;
    }

    return *this;
  }

  bool AdminLevelFeatureValue::operator==(const FeatureValue& other) const
  {
    const AdminLevelFeatureValue& otherValue=static_cast<const AdminLevelFeatureValue&>(other);

    return adminLevel==otherValue.adminLevel;
  }

  const char* const AdminLevelFeature::NAME = "AdminLevel";

  void AdminLevelFeature::Initialize(TypeConfig& typeConfig)
  {
    tagAdminLevel=typeConfig.RegisterTag("admin_level");
  }

  std::string AdminLevelFeature::GetName() const
  {
    return NAME;
  }

  size_t AdminLevelFeature::GetValueSize() const
  {
    return sizeof(AdminLevelFeatureValue);
  }

  FeatureValue* AdminLevelFeature::AllocateValue(void* buffer)
  {
    return new (buffer) AdminLevelFeatureValue();
  }

  void AdminLevelFeature::Parse(Progress& progress,
                                const TypeConfig& /*typeConfig*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& object,
                                const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                                FeatureValueBuffer& buffer) const
  {
    auto adminLevel=tags.find(tagAdminLevel);

    if (adminLevel!=tags.end()) {
      uint8_t adminLevelValue;

      if (StringToNumber(adminLevel->second,
                         adminLevelValue)) {
        AdminLevelFeatureValue* value=static_cast<AdminLevelFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetAdminLevel(adminLevelValue);
      }
      else {
        progress.Warning(std::string("Admin level is not numeric '")+adminLevel->second+"' for "+object.GetName()+"!");
      }
    }
  }

  const char* const BridgeFeature::NAME = "Bridge";

  void BridgeFeature::Initialize(TypeConfig& typeConfig)
  {
    tagBridge=typeConfig.RegisterTag("bridge");
  }

  std::string BridgeFeature::GetName() const
  {
    return NAME;
  }

  size_t BridgeFeature::GetValueSize() const
  {
    return 0;
  }

  void BridgeFeature::Parse(Progress& /*progress*/,
                            const TypeConfig& /*typeConfig*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                            FeatureValueBuffer& buffer) const
  {
    auto bridge=tags.find(tagBridge);

    if (bridge!=tags.end() &&
        !(bridge->second=="no" ||
          bridge->second=="false" ||
          bridge->second=="0")) {
      buffer.AllocateValue(feature.GetIndex());
    }
  }

  const char* const TunnelFeature::NAME = "Tunnel";

  void TunnelFeature::Initialize(TypeConfig& typeConfig)
  {
    tagTunnel=typeConfig.RegisterTag("tunnel");
  }

  std::string TunnelFeature::GetName() const
  {
    return NAME;
  }

  size_t TunnelFeature::GetValueSize() const
  {
    return 0;
  }

  void TunnelFeature::Parse(Progress& /*progress*/,
                            const TypeConfig& /*typeConfig*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                            FeatureValueBuffer& buffer) const
  {
    auto tunnel=tags.find(tagTunnel);

    if (tunnel!=tags.end() &&
        !(tunnel->second=="no" ||
          tunnel->second=="false" ||
          tunnel->second=="0")) {
      buffer.AllocateValue(feature.GetIndex());
    }
  }

  const char* const RoundaboutFeature::NAME = "Roundabout";

  void RoundaboutFeature::Initialize(TypeConfig& typeConfig)
  {
    tagJunction=typeConfig.RegisterTag("junction");
  }

  std::string RoundaboutFeature::GetName() const
  {
    return NAME;
  }

  size_t RoundaboutFeature::GetValueSize() const
  {
    return 0;
  }

  void RoundaboutFeature::Parse(Progress& /*progress*/,
                                const TypeConfig& /*typeConfig*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& /*object*/,
                                const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                                FeatureValueBuffer& buffer) const
  {
    auto junction=tags.find(tagJunction);

    if (junction!=tags.end() &&
        junction->second=="roundabout") {
      buffer.AllocateValue(feature.GetIndex());
    }
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
    if (type.Valid()) {
      DeleteData();
    }
  }

  void FeatureValueBuffer::Set(const FeatureValueBuffer& other)
  {
    if (other.GetType().Valid()) {
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
    else if (type.Valid()) {
      DeleteData();
    }
  }

  void FeatureValueBuffer::SetType(const TypeInfoRef& type)
  {
    if (this->type.Valid()) {
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
      featureBits=new uint8_t[type->GetFeatureBytes()]();
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
                                 const OSMSCOUT_HASHMAP<TagId,std::string>& tags)
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
    for (size_t i=0; i<type->GetFeatureBytes(); i++) {
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

    return true;
  }

  bool FeatureValueBuffer::Write(FileWriter& writer) const
  {
    for (size_t i=0; i<type->GetFeatureBytes(); i++) {
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
   : id(0),
     index(0),
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

  TypeInfo& TypeInfo::SetId(TypeId id)
  {
    this->id=id;

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

    valueBufferSize=offset+feature->GetValueSize();

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

  void TypeInfoSet::Adapt(const TypeConfig& typeConfig)
  {
    types.resize(typeConfig.GetTypeCount());
  }

  TypeConfig::TypeConfig()
   : nextTagId(0),
     nextTypeId(1)
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

    // Make sure, that this is always registered first.
    // It assures that id 0 is always reserved for typeIgnore
    typeInfoIgnore=new TypeInfo();
    typeInfoIgnore->SetType("");
    typeInfoIgnore->SetIgnore(true);

    RegisterType(typeInfoIgnore);

    TypeInfoRef route(new TypeInfo());

    // Internal type for showing routes
    route->SetType("_route")
          .CanBeWay(true);

    RegisterType(route);

    TypeInfoRef tileLand(new TypeInfo());

    // Internal types for the land/sea/coast tiles building the base layer for map drawing
    tileLand->SetType("_tile_land")
              .CanBeArea(true);

    RegisterType(tileLand);

    TypeInfoRef tileSea(new TypeInfo());

    tileSea->SetType("_tile_sea")
             .CanBeArea(true);

    RegisterType(tileSea);

    TypeInfoRef tileCoast(new TypeInfo());

    tileCoast->SetType("_tile_coast")
               .CanBeArea(true);

    RegisterType(tileCoast);

    TypeInfoRef tileUnknown(new TypeInfo());

    tileUnknown->SetType("_tile_unknown")
                .CanBeArea(true);

    RegisterType(tileUnknown);

    TypeInfoRef tileCoastline(new TypeInfo());

    tileCoastline->SetType("_tile_coastline")
                   .CanBeWay(true);

    RegisterType(tileCoastline);

    typeTileLand=GetTypeId("_tile_land");
    typeTileSea=GetTypeId("_tile_sea");
    typeTileCoast=GetTypeId("_tile_coast");
    typeTileUnknown=GetTypeId("_tile_unknown");
    typeTileCoastline=GetTypeId("_tile_coastline");

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
    assert(typeInfo.Valid());

    auto existingType=nameToTypeMap.find(typeInfo->GetName());

    if (existingType!=nameToTypeMap.end()) {
      return existingType->second;
    }

    if ((typeInfo->CanBeArea() || typeInfo->CanBeNode()) &&
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

    if (typeInfo->GetIgnore()) {
      typeInfo->SetId(0);
    }
    else {
      typeInfo->SetId(nextTypeId);
      nextTypeId++;
    }

    typeInfo->SetIndex(types.size());

    types.push_back(typeInfo);

    if (!typeInfo->GetIgnore() || typeInfo->GetName()=="") {
      typedTypes.push_back(typeInfo);
    }

    nameToTypeMap[typeInfo->GetName()]=typeInfo;

    idToTypeMap[typeInfo->GetId()]=typeInfo;

    return typeInfo;
  }

  TypeId TypeConfig::GetMaxTypeId() const
  {
    if (types.empty()) {
      return 0;
    }
    else {
      return nextTypeId-1;
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

  TypeInfoRef TypeConfig::GetNodeType(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const
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

  bool TypeConfig::GetWayAreaType(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap,
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

  TypeInfoRef TypeConfig::GetRelationType(const OSMSCOUT_HASHMAP<TagId,std::string>& tagMap) const
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

  TypeId TypeConfig::GetTypeId(const std::string& name) const
  {
    auto typeEntry=nameToTypeMap.find(name);

    if (typeEntry!=nameToTypeMap.end()) {
      return typeEntry->second->GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetNodeTypeId(const std::string& name) const
  {
    auto typeEntry=nameToTypeMap.find(name);

    if (typeEntry!=nameToTypeMap.end() &&
        typeEntry->second->CanBeNode()) {
      return typeEntry->second->GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetWayTypeId(const std::string& name) const
  {
    auto typeEntry=nameToTypeMap.find(name);

    if (typeEntry!=nameToTypeMap.end() &&
        typeEntry->second->CanBeWay()) {
      return typeEntry->second->GetId();
    }

    return typeIgnore;
  }

  TypeId TypeConfig::GetAreaTypeId(const std::string& name) const
  {
    auto typeEntry=nameToTypeMap.find(name);

    if (typeEntry!=nameToTypeMap.end() &&
        typeEntry->second->CanBeArea()) {
      return typeEntry->second->GetId();
    }

    return typeIgnore;
  }

  void TypeConfig::GetNodeTypes(TypeInfoSet& types) const
  {
    types.Clear();
    types.Adapt(*this);

    for (auto &type : this->types) {
      if (!type->GetIgnore() &&
          type->CanBeNode()) {
        types.Set(type);
      }
    }
  }

  void TypeConfig::GetAreaTypes(TypeInfoSet& types) const
  {
    types.Clear();
    types.Adapt(*this);

    for (auto &type : this->types) {
      if (!type->GetIgnore() &&
          type->CanBeArea()) {
        types.Set(type);
      }
    }
  }

  void TypeConfig::GetWayTypes(TypeInfoSet& types) const
  {
    types.Clear();
    types.Adapt(*this);

    for (auto &type : this->types) {
      if (!type->GetIgnore() &&
          type->CanBeWay()) {
        types.Set(type);
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
    auto entry=surfaceToGradeMap.find(surface);

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

    // Tags

    uint32_t tagCount;

    if (!scanner.ReadNumber(tagCount)) {
      std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      return false;
    }

    for (size_t i=1; i<=tagCount; i++) {
      TagId       requestedId;
      TagId       actualId;
      std::string name;

      if (!(scanner.ReadNumber(requestedId) &&
            scanner.Read(name))) {
        std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
        return false;
      }

      actualId=RegisterTag(name);

      if (actualId!=requestedId) {
        std::cerr << "Requested and actual tag id do not match" << std::endl;
        return false;
      }
    }

    // Name Tags

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

    // Alternative Name Tags

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

    // Types

    uint32_t typeCount;

    if (!scanner.ReadNumber(typeCount)) {
      std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
      return false;
    }

    for (size_t i=1; i<=typeCount; i++) {
      TypeId      requestedId;
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
      bool        ignore;
      bool        ignoreSeaLand;

      if (!(scanner.ReadNumber(requestedId) &&
            scanner.Read(name) &&
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
            scanner.Read(ignoreSeaLand) &&
            scanner.Read(ignore))) {

        std::cerr << "Format error in file '" << scanner.GetFilename() << "'" << std::endl;
        return false;
      }

      TypeInfoRef typeInfo=new TypeInfo();

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
      typeInfo->SetIgnore(optimizeLowZoom);
      typeInfo->SetIgnore(multipolygon);
      typeInfo->SetIgnore(pinWay );
      typeInfo->SetIgnore(ignoreSeaLand);
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
          std::cerr << "Feature '" << featureName << "' not found" << std::endl;
          return false;
        }

        typeInfo->AddFeature(feature);
      }

      typeInfo=RegisterType(typeInfo);

      if (typeInfo->GetId()!=requestedId) {
        std::cerr << "Requested and actual name tag id do not match" << std::endl;
        return false;
      }
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
      writer.WriteNumber(type->GetId());
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
      writer.Write(type->GetIgnoreSeaLand());
      writer.Write(type->GetIgnore());

      writer.WriteNumber((uint32_t)type->GetFeatures().size());
      for (const auto &feature : type->GetFeatures()) {
        writer.Write(feature.GetFeature()->GetName());
      }
    }

    return !writer.HasError()&&writer.Close();
  }
}
