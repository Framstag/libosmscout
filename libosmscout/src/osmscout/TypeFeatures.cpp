  /*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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

#include <algorithm>

#include <osmscout/TypeFeatures.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  void NameFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(name);
  }

  void NameFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(name);
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

  const char* const NameFeature::NAME             = "Name";
  const char* const NameFeature::NAME_LABEL       = "name";
  const size_t      NameFeature::NAME_LABEL_INDEX = 0;


  NameFeature::NameFeature()
  {
    RegisterLabel(NAME_LABEL,
                  NAME_LABEL_INDEX);
  }

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

  void NameFeature::Parse(TagErrorReporter& /*errorReporter*/,
                          const TypeConfig& typeConfig,
                          const FeatureInstance& feature,
                          const ObjectOSMRef& /*object*/,
                          const TagMap& tags,
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

  void NameAltFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(nameAlt);
  }

  void NameAltFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(nameAlt);
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

  const char* const NameAltFeature::NAME             = "NameAlt";
  const char* const NameAltFeature::NAME_LABEL       = "name";
  const size_t      NameAltFeature::NAME_LABEL_INDEX = 0;

  void NameAltFeature::Initialize(TypeConfig& /*typeConfig*/)
  {
    // no code
  }

  NameAltFeature::NameAltFeature()
  {
    RegisterLabel(NAME_LABEL,
                  NAME_LABEL_INDEX);
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

  void NameAltFeature::Parse(TagErrorReporter& /*errorReporter*/,
                             const TypeConfig& typeConfig,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const TagMap& tags,
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

  void RefFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(ref);
  }

  void RefFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(ref);
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

  const char* const RefFeature::NAME             = "Ref";
  const char* const RefFeature::NAME_LABEL       = "name";
  const size_t      RefFeature::NAME_LABEL_INDEX = 0;

  RefFeature::RefFeature()
  : tagRef(0)
  {
    RegisterLabel(NAME_LABEL,
                  NAME_LABEL_INDEX);
  }

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

  void RefFeature::Parse(TagErrorReporter& /*errorReporter*/,
                         const TypeConfig& /*typeConfig*/,
                         const FeatureInstance& feature,
                         const ObjectOSMRef& /*object*/,
                         const TagMap& tags,
                         FeatureValueBuffer& buffer) const
  {
    auto ref=tags.find(tagRef);

    if (ref!=tags.end() &&
        !ref->second.empty()) {
      RefFeatureValue* value=static_cast<RefFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetRef(ref->second);
    }
  }

  void LocationFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(location);
  }

  void LocationFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(location);
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
    tagAddrPlace=typeConfig.RegisterTag("addr:place");
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

  void LocationFeature::Parse(TagErrorReporter& /*errorReporter*/,
                              const TypeConfig& /*typeConfig*/,
                              const FeatureInstance& feature,
                              const ObjectOSMRef& /*object*/,
                              const TagMap& tags,
                              FeatureValueBuffer& buffer) const
  {
    auto street=tags.find(tagAddrStreet);

    if (street==tags.end()) {
      // We are cheating here, but from library view, there is no
      // difference in addr:street or addr:place. It is just a address.
      street=tags.find(tagAddrPlace);
      if (street==tags.end()) {
        return;
      }
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

  void AddressFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(address);
  }

  void AddressFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(address);
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

  const char* const AddressFeature::NAME             = "Address";
  const char* const AddressFeature::NAME_LABEL       = "name";
  const size_t      AddressFeature::NAME_LABEL_INDEX = 0;


  AddressFeature::AddressFeature()
  : tagAddrHouseNr(0),
    tagAddrStreet(0),
    tagAddrPlace(0)
  {
    RegisterLabel(NAME_LABEL,
                  NAME_LABEL_INDEX);
  }

  void AddressFeature::Initialize(TypeConfig& typeConfig)
  {
    tagAddrHouseNr=typeConfig.RegisterTag("addr:housenumber");
    tagAddrStreet=typeConfig.RegisterTag("addr:street");
    tagAddrPlace=typeConfig.RegisterTag("addr:place");
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

  void AddressFeature::Parse(TagErrorReporter& /*errorReporter*/,
                             const TypeConfig& /*typeConfig*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    auto street=tags.find(tagAddrStreet);

    if (street==tags.end()) {
      // We are cheating here, but from library view, there is no
      // difference in addr:street or addr:place. It is just a address.
      street=tags.find(tagAddrPlace);
      if (street==tags.end()) {
        return;
      }
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

  void AccessFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(access);
  }

  void AccessFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(access);
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
    tagAccessForward=typeConfig.RegisterTag("access:forward");
    tagAccessBackward=typeConfig.RegisterTag("access:backward");

    tagFoot=typeConfig.RegisterTag("foot");
    tagFootForward=typeConfig.RegisterTag("foot:forward");
    tagFootBackward=typeConfig.RegisterTag("foot:backward");

    tagBicycle=typeConfig.RegisterTag("bicycle");
    tagBicycleForward=typeConfig.RegisterTag("bicycle:forward");
    tagBicycleBackward=typeConfig.RegisterTag("bicycle:backward");

    tagMotorVehicle=typeConfig.RegisterTag("motor_vehicle");
    tagMotorVehicleForward=typeConfig.RegisterTag("motor_vehicle:forward");
    tagMotorVehicleBackward=typeConfig.RegisterTag("motor_vehicle:backward");

    tagMotorcar=typeConfig.RegisterTag("motorcar");
    tagMotorcarForward=typeConfig.RegisterTag("motorcar:forward");
    tagMotorcarBackward=typeConfig.RegisterTag("motorcar:backward");
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

  void AccessFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TypeConfig& /*typeConfig*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const TagMap& tags,
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

    if (accessValue!=tags.end() &&
        accessValue->second=="no") {
      // Everything is forbidden and possible later positive restrictions added again
      access=0;

      // In any other case this is a general access restriction for all vehicles that
      // does not change any of our existing flags, so we ignore it.
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

    // Flag oneway & junction
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

    // Flags foot, bicycle, motor_vehicle, motorcar

    auto accessFootValue=tags.find(tagFoot);

    if (accessFootValue!=tags.end()) {
      access&=~(AccessFeatureValue::footForward|AccessFeatureValue::footBackward);

      if (!(accessFootValue->second=="no")) {
        access|=(AccessFeatureValue::footForward|AccessFeatureValue::footBackward);
      }
    }

    auto accessBicycleValue=tags.find(tagBicycle);

    if (accessBicycleValue!=tags.end()) {
      access&=~(AccessFeatureValue::bicycleForward|AccessFeatureValue::bicycleBackward);

      if (!(accessBicycleValue->second=="no")) {
        if (!(access & AccessFeatureValue::onewayBackward)) {
          access|=(AccessFeatureValue::bicycleForward);
        }
        if (!(access & AccessFeatureValue::onewayForward)) {
          access|=(AccessFeatureValue::bicycleBackward);
        }
      }
    }

    auto accessMotorVehicleValue=tags.find(tagMotorVehicle);

    if (accessMotorVehicleValue!=tags.end()) {
      access&=~(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);

      if (!(accessMotorVehicleValue->second=="no")) {
        if (!(access & AccessFeatureValue::onewayBackward)) {
          access|=(AccessFeatureValue::carForward);
        }
        if (!(access & AccessFeatureValue::onewayForward)) {
          access|=(AccessFeatureValue::carBackward);
        }
      }
    }

    auto accessMotorcarValue=tags.find(tagMotorcar);

    if (accessMotorcarValue!=tags.end()) {
      access&=~(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);

      if (!(accessMotorcarValue->second=="no")) {
        if (!(access & AccessFeatureValue::onewayBackward)) {
          access|=(AccessFeatureValue::carForward);
        }
        if (!(access & AccessFeatureValue::onewayForward)) {
          access|=(AccessFeatureValue::carBackward);
        }
      }
    }

    // Flags foot:forward/foot:backward,
    //       bicycle:forward/bicycle:backward,
    //       motor_vehicle:forward/motor_vehicle:backward,
    //       motorcar:forward/motorcar:backward

    auto accessFootForwardValue=tags.find(tagFootForward);

    if (accessFootForwardValue!=tags.end()) {
      ParseAccessFlag(accessFootForwardValue->second,
                      access,
                      AccessFeatureValue::footForward);
    }

    auto accessFootBackwardValue=tags.find(tagFootBackward);

    if (accessFootBackwardValue!=tags.end()) {
      ParseAccessFlag(accessFootBackwardValue->second,
                      access,
                      AccessFeatureValue::footBackward);
    }

    auto accessBicycleForwardValue=tags.find(tagBicycleForward);

    if (accessBicycleForwardValue!=tags.end()) {
      ParseAccessFlag(accessBicycleForwardValue->second,
                      access,
                      AccessFeatureValue::bicycleForward);
    }

    auto accessBicycleBackwardValue=tags.find(tagBicycleBackward);

    if (accessBicycleBackwardValue!=tags.end()) {
      ParseAccessFlag(accessBicycleBackwardValue->second,
                      access,
                      AccessFeatureValue::bicycleBackward);
    }

    auto accessMotorVehicleForwardValue=tags.find(tagMotorVehicleForward);

    if (accessMotorVehicleForwardValue!=tags.end()) {
      ParseAccessFlag(accessMotorVehicleForwardValue->second,
                      access,
                      AccessFeatureValue::carForward);
    }

    auto accessMotorVehicleBackwardValue=tags.find(tagMotorVehicleBackward);

    if (accessMotorVehicleBackwardValue!=tags.end()) {
      ParseAccessFlag(accessMotorVehicleBackwardValue->second,
                      access,
                      AccessFeatureValue::carBackward);
    }

    auto accessMotorcarForwardValue=tags.find(tagMotorcarForward);

    if (accessMotorcarForwardValue!=tags.end()) {
      ParseAccessFlag(accessMotorcarForwardValue->second,
                      access,
                      AccessFeatureValue::carForward);
    }

    auto accessMotorcarBackwardValue=tags.find(tagMotorcarBackward);

    if (accessMotorcarBackwardValue!=tags.end()) {
      ParseAccessFlag(accessMotorcarBackwardValue->second,
                      access,
                      AccessFeatureValue::carBackward);
    }

    if (access!=defaultAccess) {
      AccessFeatureValue* value=static_cast<AccessFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetAccess(access);
    }
  }

  void AccessRestrictedFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(access);
  }

  void AccessRestrictedFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(access);
  }

  FeatureValue& AccessRestrictedFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const AccessRestrictedFeatureValue& otherValue=static_cast<const AccessRestrictedFeatureValue&>(other);

      access=otherValue.access;
    }

    return *this;
  }

  bool AccessRestrictedFeatureValue::operator==(const FeatureValue& other) const
  {
    const AccessRestrictedFeatureValue& otherValue=static_cast<const AccessRestrictedFeatureValue&>(other);

    return access==otherValue.access;
  }

  const char* const AccessRestrictedFeature::NAME = "AccessRestricted";

  void AccessRestrictedFeature::Initialize(TypeConfig& typeConfig)
  {
    tagAccess=typeConfig.RegisterTag("access");
    tagFoot=typeConfig.RegisterTag("foot");
    tagBicycle=typeConfig.RegisterTag("bicycle");
    tagMotorVehicle=typeConfig.RegisterTag("motor_vehicle");
  }

  std::string AccessRestrictedFeature::GetName() const
  {
    return NAME;
  }

  size_t AccessRestrictedFeature::GetValueSize() const
  {
    return sizeof(AccessRestrictedFeatureValue);
  }

  FeatureValue* AccessRestrictedFeature::AllocateValue(void* buffer)
  {
    return new (buffer) AccessRestrictedFeatureValue();
  }

  void AccessRestrictedFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                      const TypeConfig& /*typeConfig*/,
                                      const FeatureInstance& feature,
                                      const ObjectOSMRef& /*object*/,
                                      const TagMap& tags,
                                      FeatureValueBuffer& buffer) const
  {
    uint8_t access=AccessRestrictedFeatureValue::foot|AccessRestrictedFeatureValue::bicycle|AccessRestrictedFeatureValue::car;
    uint8_t defaultAccess=AccessRestrictedFeatureValue::foot|AccessRestrictedFeatureValue::bicycle|AccessRestrictedFeatureValue::car;

    auto accessValue=tags.find(tagAccess);

    if (accessValue!=tags.end()) {

      if (accessValue->second=="delivery" ||
          accessValue->second=="destination" ||
          accessValue->second=="private") {
        access&=~(AccessRestrictedFeatureValue::foot|AccessRestrictedFeatureValue::bicycle|AccessRestrictedFeatureValue::car);
      }
    }

    auto accessFootValue=tags.find(tagFoot);

    if (accessFootValue!=tags.end()) {
      if (accessFootValue->second=="delivery" ||
          accessFootValue->second=="destination" ||
          accessFootValue->second=="private") {
        access&=~AccessRestrictedFeatureValue::foot;
      }
      else {
        access|=AccessRestrictedFeatureValue::foot;
      }
    }

    auto accessBicycleValue=tags.find(tagBicycle);

    if (accessBicycleValue!=tags.end()) {
      if (accessBicycleValue->second=="delivery" ||
          accessBicycleValue->second=="destination" ||
          accessBicycleValue->second=="private") {
        access&=~AccessRestrictedFeatureValue::bicycle;
      }
      else {
        access|=AccessRestrictedFeatureValue::bicycle;
      }
    }

    auto accessCarValue=tags.find(tagMotorVehicle);

    if (accessCarValue!=tags.end()) {
      if (accessCarValue->second=="delivery" ||
          accessCarValue->second=="destination" ||
          accessCarValue->second=="private") {
        access&=~AccessRestrictedFeatureValue::car;
      }
      else {
        access|=AccessRestrictedFeatureValue::car;
      }
    }

    if (access!=defaultAccess) {
      AccessRestrictedFeatureValue* value=static_cast<AccessRestrictedFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetAccess(access);
    }
  }

  void LayerFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(layer);
  }

  void LayerFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(layer);
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

  void LayerFeature::Parse(TagErrorReporter& errorReporter,
                           const TypeConfig& /*typeConfig*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
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
        errorReporter.ReportTag(object,tags,std::string("Layer tag value '")+layer->second+"' is not numeric!");
      }
    }
  }

  void WidthFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(width);
  }

  void WidthFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(width);
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

  void WidthFeature::Parse(TagErrorReporter& errorReporter,
                           const TypeConfig& /*typeConfig*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
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
      errorReporter.ReportTag(object,tags,std::string("Width tag value '")+width->second+"' is no double!");
    }
    else if (w<0 && w>255.5) {
      errorReporter.ReportTag(object,tags,std::string("Width tag value '")+width->second+"' value is too small or too big!");
    }
    else {
      WidthFeatureValue* value=static_cast<WidthFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetWidth((uint8_t)floor(w+0.5));
    }
  }

  void MaxSpeedFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(maxSpeed);
  }

  void MaxSpeedFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(maxSpeed);
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

  void MaxSpeedFeature::Parse(TagErrorReporter& errorReporter,
                              const TypeConfig& typeConfig,
                              const FeatureInstance& feature,
                              const ObjectOSMRef& object,
                              const TagMap& tags,
                              FeatureValueBuffer& buffer) const
  {
    auto maxSpeed=tags.find(tagMaxSpeed);

    if (maxSpeed==tags.end()) {
      return;
    }

    std::string valueString=maxSpeed->second;
    size_t      valueNumeric;
    bool        isMph=false;

    if (valueString=="signals" ||
        valueString=="none" ||
        valueString=="no") {
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
    else {
      pos=valueString.rfind("km/h");

      if (pos!=std::string::npos) {
        valueString.erase(pos);
        isMph=false;
      }
    }

    while (valueString.length()>0 && valueString[valueString.length()-1]==' ') {
      valueString.erase(valueString.length()-1);
    }

    if (!StringToNumber(valueString,
                        valueNumeric)) {
      uint8_t maxSpeedValue;

      if (typeConfig.GetMaxSpeedFromAlias(valueString,
                                          maxSpeedValue)) {
        valueNumeric=maxSpeedValue;
      }
      else {
        errorReporter.ReportTag(object,tags,std::string("Max speed tag value '")+maxSpeed->second+"' is not numeric!");
        return;
      }
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
        value->SetMaxSpeed((uint8_t)valueNumeric);
      }
    }
  }

  void GradeFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(grade);
  }

  void GradeFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(grade);
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

  void GradeFeature::Parse(TagErrorReporter& errorReporter,
                           const TypeConfig& typeConfig,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
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
        errorReporter.ReportTag(object,tags,std::string("Unsupported tracktype value '")+tracktype->second+"'");
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
        errorReporter.ReportTag(object,tags,std::string("Unknown surface type '")+surface->second+"' !");
      }
    }
  }

  void AdminLevelFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(adminLevel);
    scanner.Read(isIn);
  }

  void AdminLevelFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(adminLevel);
    writer.Write(isIn);
  }

  FeatureValue& AdminLevelFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const AdminLevelFeatureValue& otherValue=static_cast<const AdminLevelFeatureValue&>(other);

      adminLevel=otherValue.adminLevel;
      isIn=otherValue.isIn;
    }

    return *this;
  }

  bool AdminLevelFeatureValue::operator==(const FeatureValue& other) const
  {
    const AdminLevelFeatureValue& otherValue=static_cast<const AdminLevelFeatureValue&>(other);

    return adminLevel==otherValue.adminLevel && isIn==otherValue.isIn;
  }

  const char* const AdminLevelFeature::NAME = "AdminLevel";

  void AdminLevelFeature::Initialize(TypeConfig& typeConfig)
  {
    tagAdminLevel=typeConfig.RegisterTag("admin_level");
    tagIsIn=typeConfig.RegisterTag("is_in");
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

  void AdminLevelFeature::Parse(TagErrorReporter& errorReporter,
                                const TypeConfig& /*typeConfig*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& object,
                                const TagMap& tags,
                                FeatureValueBuffer& buffer) const
  {
    auto adminLevel=tags.find(tagAdminLevel);

    if (adminLevel!=tags.end()) {
      uint8_t adminLevelValue;

      if (StringToNumber(adminLevel->second,
                         adminLevelValue)) {
        AdminLevelFeatureValue* value=static_cast<AdminLevelFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetAdminLevel(adminLevelValue);

        auto isIn=tags.find(tagIsIn);

        if (isIn!=tags.end()) {
          value->SetIsIn(isIn->second);
        }
      }
      else {
        errorReporter.ReportTag(object,tags,std::string("Admin level is not numeric '")+adminLevel->second+"'!");
      }
    }
  }

  void PostalCodeFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(postalCode);
  }

  void PostalCodeFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(postalCode);
  }

  FeatureValue& PostalCodeFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const PostalCodeFeatureValue& otherValue=static_cast<const PostalCodeFeatureValue&>(other);

      postalCode=otherValue.postalCode;
    }

    return *this;
  }

  bool PostalCodeFeatureValue::operator==(const FeatureValue& other) const
  {
    const PostalCodeFeatureValue& otherValue=static_cast<const PostalCodeFeatureValue&>(other);

    return postalCode==otherValue.postalCode;
  }

  const char* const PostalCodeFeature::NAME = "PostalCode";

  PostalCodeFeature::PostalCodeFeature()
  {
      RegisterLabel(NAME, 0);
  }

  void PostalCodeFeature::Initialize(TypeConfig& typeConfig)
  {
    tagPostalCode=typeConfig.RegisterTag("postal_code");
    tagAddrPostCode=typeConfig.RegisterTag("addr:postcode");
  }

  std::string PostalCodeFeature::GetName() const
  {
    return NAME;
  }

  size_t PostalCodeFeature::GetValueSize() const
  {
    return sizeof(PostalCodeFeatureValue);
  }

  FeatureValue* PostalCodeFeature::AllocateValue(void* buffer)
  {
    return new (buffer) PostalCodeFeatureValue();
  }

  void PostalCodeFeature::Parse(TagErrorReporter& errorReporter,
                                const TypeConfig& /*typeConfig*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& object,
                                const TagMap& tags,
                                FeatureValueBuffer& buffer) const
  {
    auto postalCode=tags.find(tagPostalCode);
    auto addrPostCode=tags.find(tagAddrPostCode);

    std::string postalCodeValue;

    if (postalCode!=tags.end()) {
      postalCodeValue = postalCode->second;
    }

    if (addrPostCode!=tags.end()) {
      postalCodeValue = addrPostCode->second;
    }

    try {
      if (!postalCodeValue.empty()) {
        size_t idx = feature.GetIndex();
        FeatureValue* fv = buffer.AllocateValue(idx);
        PostalCodeFeatureValue* value=static_cast<PostalCodeFeatureValue*>(fv);

        value->SetPostalCode(postalCodeValue);
      }
    }
    catch (const std::exception &e) {
      errorReporter.ReportTag(object,tags,std::string("Postal code parse exception: ")+e.what());
    }
  }

  void WebsiteFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(website);
  }

  void WebsiteFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(website);
  }

  FeatureValue& WebsiteFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const WebsiteFeatureValue& otherValue=static_cast<const WebsiteFeatureValue&>(other);

      website=otherValue.website;
    }

    return *this;
  }

  bool WebsiteFeatureValue::operator==(const FeatureValue& other) const
  {
    const WebsiteFeatureValue& otherValue=static_cast<const WebsiteFeatureValue&>(other);

    return website==otherValue.website;
  }

  const char* const WebsiteFeature::NAME = "Website";

  WebsiteFeature::WebsiteFeature()
  {
    RegisterLabel(NAME, 0);
  }

  void WebsiteFeature::Initialize(TypeConfig& typeConfig)
  {
    tagWebsite=typeConfig.RegisterTag("website");
  }

  std::string WebsiteFeature::GetName() const
  {
    return NAME;
  }

  size_t WebsiteFeature::GetValueSize() const
  {
    return sizeof(WebsiteFeatureValue);
  }

  FeatureValue* WebsiteFeature::AllocateValue(void* buffer)
  {
    return new (buffer) WebsiteFeatureValue();
  }

  void WebsiteFeature::Parse(TagErrorReporter& errorReporter,
                             const TypeConfig& /*typeConfig*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& object,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    // ignore ways for now
    if (object.GetType() == OSMRefType::osmRefWay)
      return;

    auto website=tags.find(tagWebsite);

    std::string strValue;

    if (website!=tags.end()) {
      strValue = website->second;
    }

    try {
      if (!strValue.empty()) {
        size_t idx = feature.GetIndex();
        FeatureValue* fv = buffer.AllocateValue(idx);
        WebsiteFeatureValue* value=static_cast<WebsiteFeatureValue*>(fv);

        value->SetWebsite(strValue);
      }
    }
    catch (const std::exception &e) {
      errorReporter.ReportTag(object,tags,std::string("Website parse exception: ")+e.what());
    }
  }

  void PhoneFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(phone);
  }

  void PhoneFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(phone);
  }

  FeatureValue& PhoneFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const PhoneFeatureValue& otherValue=static_cast<const PhoneFeatureValue&>(other);

      phone=otherValue.phone;
    }

    return *this;
  }

  bool PhoneFeatureValue::operator==(const FeatureValue& other) const
  {
    const PhoneFeatureValue& otherValue=static_cast<const PhoneFeatureValue&>(other);

    return phone==otherValue.phone;
  }

  const char* const PhoneFeature::NAME = "Phone";

  PhoneFeature::PhoneFeature()
  {
    RegisterLabel(NAME, 0);
  }

  void PhoneFeature::Initialize(TypeConfig& typeConfig)
  {
    tagPhone=typeConfig.RegisterTag("phone");
  }

  std::string PhoneFeature::GetName() const
  {
    return NAME;
  }

  size_t PhoneFeature::GetValueSize() const
  {
    return sizeof(PhoneFeatureValue);
  }

  FeatureValue* PhoneFeature::AllocateValue(void* buffer)
  {
    return new (buffer) PhoneFeatureValue();
  }

  void PhoneFeature::Parse(TagErrorReporter& errorReporter,
                           const TypeConfig& /*typeConfig*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    // ignore ways for now
    if (object.GetType() == OSMRefType::osmRefWay)
      return;

    auto phone=tags.find(tagPhone);

    std::string strValue;

    if (phone!=tags.end()) {
      strValue = phone->second;
    }

    try {
      if (!strValue.empty()) {
        // remove invalid characters from phone number [0123456789+;,] http://wiki.openstreetmap.org/wiki/Key:phone
        // - there can be multiple phone numbers separated by semicolon (some mappers use comma)
        strValue.erase(
          std::remove_if(strValue.begin(), strValue.end(), [](char x){return (x<'0'||x>'9') && x!='+' && x!=';' && x!=',';}),
          strValue.end());

        size_t idx = feature.GetIndex();
        FeatureValue* fv = buffer.AllocateValue(idx);
        PhoneFeatureValue* value=static_cast<PhoneFeatureValue*>(fv);

        value->SetPhone(strValue);
      }
    }
    catch (const std::exception &e) {
      errorReporter.ReportTag(object,tags,std::string("Phone parse exception: ")+e.what());
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

  void BridgeFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TypeConfig& /*typeConfig*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const TagMap& tags,
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

  void TunnelFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TypeConfig& /*typeConfig*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const TagMap& tags,
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

  const char* const EmbankmentFeature::NAME = "Embankment";

  void EmbankmentFeature::Initialize(TypeConfig& typeConfig)
  {
        tagEmbankment=typeConfig.RegisterTag("embankment");
  }

  std::string EmbankmentFeature::GetName() const
  {
        return NAME;
  }

  void EmbankmentFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                const TypeConfig& /*typeConfig*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& /*object*/,
                                const TagMap& tags,
                                FeatureValueBuffer& buffer) const
  {
        auto embankment=tags.find(tagEmbankment);

        if (embankment!=tags.end() &&
            !(embankment->second=="no" ||
              embankment->second=="false" ||
              embankment->second=="0")) {
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

  void RoundaboutFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                const TypeConfig& /*typeConfig*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& /*object*/,
                                const TagMap& tags,
                                FeatureValueBuffer& buffer) const
  {
    auto junction=tags.find(tagJunction);

    if (junction!=tags.end() &&
        junction->second=="roundabout") {
      buffer.AllocateValue(feature.GetIndex());
    }
  }

  void EleFeatureValue::Read(FileScanner& scanner)
  {
    scanner.ReadNumber(ele);
  }

  void EleFeatureValue::Write(FileWriter& writer)
  {
    writer.WriteNumber(ele);
  }

  FeatureValue& EleFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const EleFeatureValue& otherValue=static_cast<const EleFeatureValue&>(other);

      ele=otherValue.ele;
    }

    return *this;
  }

  bool EleFeatureValue::operator==(const FeatureValue& other) const
  {
    const EleFeatureValue& otherValue=static_cast<const EleFeatureValue&>(other);

    return ele==otherValue.ele;
  }

  const char* const EleFeature::NAME             = "Ele";
  const char* const EleFeature::NAME_LABEL       = "inMeter";
  const size_t      EleFeature::NAME_LABEL_INDEX = 0;

  EleFeature::EleFeature()
  : tagEle(0)
  {
    RegisterLabel(NAME_LABEL,
                  NAME_LABEL_INDEX);
  }

  void EleFeature::Initialize(TypeConfig& typeConfig)
  {
    tagEle=typeConfig.RegisterTag("ele");
  }

  std::string EleFeature::GetName() const
  {
    return NAME;
  }

  size_t EleFeature::GetValueSize() const
  {
    return sizeof(EleFeatureValue);
  }

  FeatureValue* EleFeature::AllocateValue(void* buffer)
  {
    return new (buffer) EleFeatureValue();
  }

  void EleFeature::Parse(TagErrorReporter& errorReporter,
                         const TypeConfig& /*typeConfig*/,
                         const FeatureInstance& feature,
                         const ObjectOSMRef& object,
                         const TagMap& tags,
                         FeatureValueBuffer& buffer) const
  {
    auto ele=tags.find(tagEle);

    if (ele==tags.end()) {
      return;
    }

    std::string eleString=ele->second;
    double      e;
    size_t      pos=0;
    size_t      count=0;

    // We expect that float values use '.' as separator, but many values use ',' instead.
    // Try try fix this if string looks reasonable
    for (size_t i=0; i<eleString.length() && count<=1; i++) {
      if (eleString[i]==',') {
        pos=i;
        count++;
      }
    }

    if (count==1) {
      eleString[pos]='.';
    }

    // Some ele tag values add an 'm' to hint that the unit is meter, remove it.
    if (eleString.length()>=2) {
      if (eleString[eleString.length()-1]=='m' &&
          ((eleString[eleString.length()-2]>='0' &&
            eleString[eleString.length()-2]<='9') ||
            eleString[eleString.length()-2]<=' ')) {
        eleString.erase(eleString.length()-1);
      }

      // Trim possible trailing spaces
      while (eleString.length()>0 &&
             eleString[eleString.length()-1]==' ') {
        eleString.erase(eleString.length()-1);
      }
    }

    if (!StringToNumber(eleString,e)) {
      errorReporter.ReportTag(object,tags,std::string("Ele tag value '")+ele->second+"' is no double!");
    }
    else if (e<0 && e>std::numeric_limits<uint32_t>::max()) {
      errorReporter.ReportTag(object,tags,std::string("Ele tag value '")+ele->second+"' value is too small or too big!");
    }
    else {
      EleFeatureValue* value=static_cast<EleFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetEle((uint32_t)floor(e+0.5));
    }
  }

  void DestinationFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(destination);
  }

  void DestinationFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(destination);
  }

  FeatureValue& DestinationFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const DestinationFeatureValue& otherValue=static_cast<const DestinationFeatureValue&>(other);

      destination=otherValue.destination;
    }

    return *this;
  }

  bool DestinationFeatureValue::operator==(const FeatureValue& other) const
  {
    const DestinationFeatureValue& otherValue=static_cast<const DestinationFeatureValue&>(other);

    return destination==otherValue.destination;
  }

  const char* const DestinationFeature::NAME             = "Destination";
  const char* const DestinationFeature::NAME_LABEL       = "label";
  const size_t      DestinationFeature::NAME_LABEL_INDEX = 0;

  DestinationFeature::DestinationFeature()
  : tagDestination(0),
    tagDestinationRef(0),
    tagDestinationForward(0)
  {
    RegisterLabel(NAME_LABEL,
                  NAME_LABEL_INDEX);
  }

  void DestinationFeature::Initialize(TypeConfig& typeConfig)
  {
    tagDestination=typeConfig.RegisterTag("destination");
    tagDestinationRef=typeConfig.RegisterTag("destination:ref");
    tagDestinationForward=typeConfig.RegisterTag("destination:forward");
  }

  std::string DestinationFeature::GetName() const
  {
    return NAME;
  }

  size_t DestinationFeature::GetValueSize() const
  {
    return sizeof(DestinationFeatureValue);
  }

  FeatureValue* DestinationFeature::AllocateValue(void* buffer)
  {
    return new (buffer) DestinationFeatureValue();
  }

  void DestinationFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                 const TypeConfig& /*typeConfig*/,
                                 const FeatureInstance& feature,
                                 const ObjectOSMRef& /*object*/,
                                 const TagMap& tags,
                                 FeatureValueBuffer& buffer) const
  {
    auto destination=tags.find(tagDestination);

    if (destination==tags.end()) {
      destination=tags.find(tagDestinationForward);
    }

    if (destination==tags.end()) {
      destination=tags.find(tagDestinationRef);
    }

    if (destination==tags.end()) {
      return;
    }

    if (!destination->second.empty()) {
      DestinationFeatureValue* value=static_cast<DestinationFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetDestination(destination->second);
    }
  }

  const char* const BuildingFeature::NAME = "Building";

  void BuildingFeature::Initialize(TypeConfig& typeConfig)
  {
    tagBuilding=typeConfig.RegisterTag("building");
  }

  std::string BuildingFeature::GetName() const
  {
    return NAME;
  }

  void BuildingFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TypeConfig& /*typeConfig*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const TagMap& tags,
                            FeatureValueBuffer& buffer) const
  {
    auto building=tags.find(tagBuilding);

    if (building!=tags.end() &&
        !(building->second=="no" ||
          building->second=="false" ||
          building->second=="0")) {
      buffer.AllocateValue(feature.GetIndex());
    }
  }

  void IsInFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(isIn);
  }

  void IsInFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(isIn);
  }

  FeatureValue& IsInFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const IsInFeatureValue& otherValue=static_cast<const IsInFeatureValue&>(other);

      isIn=otherValue.isIn;
    }

    return *this;
  }

  bool IsInFeatureValue::operator==(const FeatureValue& other) const
  {
    const IsInFeatureValue& otherValue=static_cast<const IsInFeatureValue&>(other);

    return isIn==otherValue.isIn;
  }

  const char* const IsInFeature::NAME = "IsIn";

  void IsInFeature::Initialize(TypeConfig& typeConfig)
  {
    tagIsIn=typeConfig.RegisterTag("is_in");
  }

  std::string IsInFeature::GetName() const
  {
    return NAME;
  }

  size_t IsInFeature::GetValueSize() const
  {
    return sizeof(IsInFeatureValue);
  }

  FeatureValue* IsInFeature::AllocateValue(void* buffer)
  {
    return new (buffer) IsInFeatureValue();
  }

  void IsInFeature::Parse(TagErrorReporter& /*errorReporter*/,
                          const TypeConfig& /*typeConfig*/,
                          const FeatureInstance& feature,
                          const ObjectOSMRef& /*object*/,
                          const TagMap& tags,
                          FeatureValueBuffer& buffer) const
  {
    auto isIn=tags.find(tagIsIn);

    if (isIn!=tags.end() && !isIn->second.empty()) {
      IsInFeatureValue* value=static_cast<IsInFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));
      value->SetIsIn(isIn->second);
    }
  }

  DynamicFeatureReader::DynamicFeatureReader(const TypeConfig& typeConfig,
                                             const Feature& feature)
  : featureName(feature.GetName())
  {
    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(featureName,
                           index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  bool DynamicFeatureReader::IsSet(const FeatureValueBuffer& buffer) const
  {
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max()) {
      return buffer.HasFeature(index);
    }
    else {
      return false;
    }
  }
}
