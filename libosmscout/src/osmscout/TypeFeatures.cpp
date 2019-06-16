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

#include <osmscout/TypeFeatures.h>

#include <algorithm>

#include <osmscout/util/String.h>

#include <iostream>
namespace osmscout {

  void NameFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(name);
  }

  void NameFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(name);
  }


  NameFeatureValue& NameFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const NameFeatureValue&>(other);

      name=otherValue.name;
    }

    return *this;
  }

  bool NameFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const NameFeatureValue&>(other);

    return name==otherValue.name;
  }

  const char* const NameFeature::NAME             = "Name";
  const char* const NameFeature::NAME_LABEL       = "name";
  const size_t      NameFeature::NAME_LABEL_INDEX = 0;


  NameFeature::NameFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void NameFeature::Initialize(TagRegistry& /*tagRegistry*/)
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
                          const TagRegistry& tagRegistry,
                          const FeatureInstance& feature,
                          const ObjectOSMRef& /*object*/,
                          const TagMap& tags,
                          FeatureValueBuffer& buffer) const
  {
    std::string name;
    uint32_t    namePriority=0;

    for (const auto &tag : tags) {
      uint32_t ntPrio;
      bool     isNameTag=tagRegistry.IsNameTag(tag.first,ntPrio);

      if (isNameTag &&
          (name.empty() || ntPrio>namePriority)) {
        name=tag.second;
        namePriority=ntPrio;
      }
    }

    if (!name.empty()) {
      auto* value=static_cast<NameFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  NameAltFeatureValue& NameAltFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const NameAltFeatureValue&>(other);

      nameAlt=otherValue.nameAlt;
    }

    return *this;
  }

  bool NameAltFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const NameAltFeatureValue&>(other);

    return nameAlt==otherValue.nameAlt;
  }

  const char* const NameAltFeature::NAME             = "NameAlt";
  const char* const NameAltFeature::NAME_LABEL       = "name";
  const size_t      NameAltFeature::NAME_LABEL_INDEX = 0;

  void NameAltFeature::Initialize(TagRegistry& /*tagRegistry*/)
  {
    // no code
  }

  NameAltFeature::NameAltFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
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
                             const TagRegistry& tagRegistry,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    std::string nameAlt;
    uint32_t    nameAltPriority=0;

    for (const auto &tag : tags) {
      uint32_t natPrio;
      bool     isNameAltTag=tagRegistry.IsNameAltTag(tag.first,natPrio);

      if (isNameAltTag &&
          (nameAlt.empty() || natPrio>nameAltPriority)) {
        nameAlt=tag.second;
        nameAltPriority=natPrio;
      }
    }

    if (!nameAlt.empty()) {
      auto* value=static_cast<NameAltFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  RefFeatureValue& RefFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const RefFeatureValue&>(other);

      ref=otherValue.ref;
    }

    return *this;
  }

  bool RefFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const RefFeatureValue&>(other);

    return ref==otherValue.ref;
  }

  const char* const RefFeature::NAME             = "Ref";
  const char* const RefFeature::NAME_LABEL       = "name";
  const size_t      RefFeature::NAME_LABEL_INDEX = 0;

  RefFeature::RefFeature()
  : tagRef(0)
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void RefFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagRef=tagRegistry.RegisterTag("ref");
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
                         const TagRegistry& /*tagRegistry*/,
                         const FeatureInstance& feature,
                         const ObjectOSMRef& /*object*/,
                         const TagMap& tags,
                         FeatureValueBuffer& buffer) const
  {
    auto ref=tags.find(tagRef);

    if (ref!=tags.end() &&
        !ref->second.empty()) {
      auto* value=static_cast<RefFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  LocationFeatureValue& LocationFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const LocationFeatureValue&>(other);

      location=otherValue.location;
    }

    return *this;
  }

  bool LocationFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const LocationFeatureValue&>(other);

    return location==otherValue.location;
  }

  const char* const LocationFeature::NAME = "Location";

  void LocationFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAddrHouseNr=tagRegistry.RegisterTag("addr:housenumber");
    tagAddrStreet=tagRegistry.RegisterTag("addr:street");
    tagAddrPlace=tagRegistry.RegisterTag("addr:place");
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
                              const TagRegistry& /*tagRegistry*/,
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
      auto* value=static_cast<LocationFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  AddressFeatureValue& AddressFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const AddressFeatureValue&>(other);

      address=otherValue.address;
    }

    return *this;
  }

  bool AddressFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const AddressFeatureValue&>(other);

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
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void AddressFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAddrHouseNr=tagRegistry.RegisterTag("addr:housenumber");
    tagAddrStreet=tagRegistry.RegisterTag("addr:street");
    tagAddrPlace=tagRegistry.RegisterTag("addr:place");
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
                             const TagRegistry& /*tagRegistry*/,
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
      auto* value=static_cast<AddressFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  AccessFeatureValue& AccessFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const AccessFeatureValue&>(other);

      access=otherValue.access;
    }

    return *this;
  }

  bool AccessFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const AccessFeatureValue&>(other);

    return access==otherValue.access;
  }

  const char* const AccessFeature::NAME = "Access";

  void AccessFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagOneway=tagRegistry.RegisterTag("oneway");
    tagJunction=tagRegistry.RegisterTag("junction");

    tagAccess=tagRegistry.RegisterTag("access");
    tagAccessForward=tagRegistry.RegisterTag("access:forward");
    tagAccessBackward=tagRegistry.RegisterTag("access:backward");

    tagFoot=tagRegistry.RegisterTag("foot");
    tagFootForward=tagRegistry.RegisterTag("foot:forward");
    tagFootBackward=tagRegistry.RegisterTag("foot:backward");

    tagBicycle=tagRegistry.RegisterTag("bicycle");
    tagBicycleForward=tagRegistry.RegisterTag("bicycle:forward");
    tagBicycleBackward=tagRegistry.RegisterTag("bicycle:backward");

    tagMotorVehicle=tagRegistry.RegisterTag("motor_vehicle");
    tagMotorVehicleForward=tagRegistry.RegisterTag("motor_vehicle:forward");
    tagMotorVehicleBackward=tagRegistry.RegisterTag("motor_vehicle:backward");

    tagMotorcar=tagRegistry.RegisterTag("motorcar");
    tagMotorcarForward=tagRegistry.RegisterTag("motorcar:forward");
    tagMotorcarBackward=tagRegistry.RegisterTag("motorcar:backward");
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
                            const TagRegistry& /*tagRegistry*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const TagMap& tags,
                            FeatureValueBuffer& buffer) const
  {
    uint8_t access=0u;

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
      access=0u;

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
      auto* value=static_cast<AccessFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  AccessRestrictedFeatureValue& AccessRestrictedFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const AccessRestrictedFeatureValue&>(other);

      access=otherValue.access;
    }

    return *this;
  }

  bool AccessRestrictedFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const AccessRestrictedFeatureValue&>(other);

    return access==otherValue.access;
  }

  const char* const AccessRestrictedFeature::NAME = "AccessRestricted";

  void AccessRestrictedFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAccess=tagRegistry.RegisterTag("access");
    tagFoot=tagRegistry.RegisterTag("foot");
    tagBicycle=tagRegistry.RegisterTag("bicycle");
    tagMotorVehicle=tagRegistry.RegisterTag("motor_vehicle");
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
                                      const TagRegistry& /*tagRegistry*/,
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
      auto* value=static_cast<AccessRestrictedFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  LayerFeatureValue& LayerFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const LayerFeatureValue&>(other);

      layer=otherValue.layer;
    }

    return *this;
  }

  bool LayerFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const LayerFeatureValue&>(other);

    return layer==otherValue.layer;
  }

  const char* const LayerFeature::NAME = "Layer";

  void LayerFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagLayer=tagRegistry.RegisterTag("layer");
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
                           const TagRegistry& /*tagRegistry*/,
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
          auto* value=static_cast<LayerFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  WidthFeatureValue& WidthFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const WidthFeatureValue&>(other);

      width=otherValue.width;
    }

    return *this;
  }

  bool WidthFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const WidthFeatureValue&>(other);

    return width==otherValue.width;
  }

  const char* const WidthFeature::NAME = "Width";

  void WidthFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagWidth=tagRegistry.RegisterTag("width");
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
                           const TagRegistry& /*tagRegistry*/,
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
    else if (w<0 || w>255.5) {
      errorReporter.ReportTag(object,tags,std::string("Width tag value '")+width->second+"' value is too small or too big!");
    }
    else {
      auto* value=static_cast<WidthFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  MaxSpeedFeatureValue& MaxSpeedFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const MaxSpeedFeatureValue&>(other);

      maxSpeed=otherValue.maxSpeed;
    }

    return *this;
  }

  bool MaxSpeedFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const MaxSpeedFeatureValue&>(other);

    return maxSpeed==otherValue.maxSpeed;
  }

  const char* const MaxSpeedFeature::NAME = "MaxSpeed";

  void MaxSpeedFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagMaxSpeed=tagRegistry.RegisterTag("maxspeed");
    tagMaxSpeedForward=tagRegistry.RegisterTag("maxspeed:forward");
    tagMaxSpeedBackward=tagRegistry.RegisterTag("maxspeed:backward");
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

  bool MaxSpeedFeature::GetTagValue(TagErrorReporter& errorReporter,
                                    const TagRegistry& tagRegistry,
                                    const ObjectOSMRef& object,
                                    const TagMap& tags,
                                    const std::string& input, uint8_t& speed) const
  {
    std::string valueString(input);
    size_t      valueNumeric;
    bool        isMph=false;

    if (valueString=="signals" ||
        valueString=="none" ||
        valueString=="no") {
      return false;
    }

    // "walk" should not be used, but we provide an estimation anyway,
    // since it is likely still better than the default
    if (valueString=="walk") {
      speed=10;
      return true;
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

    while (valueString.length()>0 &&
           valueString[valueString.length()-1]==' ') {
      valueString.erase(valueString.length()-1);
    }

    if (!StringToNumber(valueString,
                        valueNumeric)) {
      uint8_t maxSpeedValue;

      if (tagRegistry.GetMaxSpeedFromAlias(valueString,
                                           maxSpeedValue)) {
        valueNumeric=maxSpeedValue;
      }
      else {
        errorReporter.ReportTag(object,tags,std::string("Max speed tag value '")+input+"' is not numeric!");
        return false;
      }
    }

    if (isMph) {
      if (valueNumeric>std::numeric_limits<uint8_t>::max()/lround(1.609)) {

        speed=std::numeric_limits<uint8_t>::max();
      }
      else {
        speed=(uint8_t)lround(valueNumeric*1.609);
      }
    }
    else {
      if (valueNumeric>std::numeric_limits<uint8_t>::max()) {
        speed=std::numeric_limits<uint8_t>::max();
      }
      else {
        speed=(uint8_t)valueNumeric;
      }
    }

    return true;
  }


  void MaxSpeedFeature::Parse(TagErrorReporter& errorReporter,
                              const TagRegistry& tagRegistry,
                              const FeatureInstance& feature,
                              const ObjectOSMRef& object,
                              const TagMap& tags,
                              FeatureValueBuffer& buffer) const
  {
    auto maxSpeed=tags.find(tagMaxSpeed);
    auto maxSpeedForward=tags.find(tagMaxSpeedForward);
    auto maxSpeedBackward=tags.find(tagMaxSpeedBackward);

    if (maxSpeedForward!=tags.end() &&
        maxSpeedBackward!=tags.end()) {
      uint8_t forwardSpeed=0;
      uint8_t backwardSpeed=0;

      if (!GetTagValue(errorReporter,
                       tagRegistry,
                       object,
                       tags,
                       maxSpeedForward->second,
                       forwardSpeed)) {
        return;
      }

      if (!GetTagValue(errorReporter,
                       tagRegistry,
                       object,
                       tags,
                       maxSpeedBackward->second,
                       backwardSpeed)) {
        return;
      }

      auto* featureValue=static_cast<MaxSpeedFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      featureValue->SetMaxSpeed(std::min(forwardSpeed,backwardSpeed));
    }
    else if (maxSpeedForward!=tags.end()) {
      uint8_t speed=0;

      if (!GetTagValue(errorReporter,
                       tagRegistry,
                       object,
                       tags,
                       maxSpeedForward->second,
                       speed)) {
        return;
      }

      auto* featureValue=static_cast<MaxSpeedFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      featureValue->SetMaxSpeed(speed);
    }
    else if (maxSpeedBackward!=tags.end()) {
      uint8_t speed=0;

      if (!GetTagValue(errorReporter,
                       tagRegistry,
                       object,
                       tags,
                       maxSpeedBackward->second,
                       speed)) {
        return;
      }

      auto* featureValue=static_cast<MaxSpeedFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      featureValue->SetMaxSpeed(speed);
    }
    else if (maxSpeed!=tags.end()) {
      uint8_t speed=0;

      if (!GetTagValue(errorReporter,
                       tagRegistry,
                       object,
                       tags,
                       maxSpeed->second,
                       speed)) {
        return;
      }

      auto* featureValue=static_cast<MaxSpeedFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      featureValue->SetMaxSpeed(speed);
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

  GradeFeatureValue& GradeFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const GradeFeatureValue&>(other);

      grade=otherValue.grade;
    }

    return *this;
  }

  bool GradeFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const GradeFeatureValue&>(other);

    return grade==otherValue.grade;
  }

  const char* const GradeFeature::NAME = "Grade";

  void GradeFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagSurface=tagRegistry.RegisterTag("surface");
    tagTrackType=tagRegistry.RegisterTag("tracktype");
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
                           const TagRegistry& tagRegistry,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    auto tracktype=tags.find(tagTrackType);

    if (tracktype!=tags.end()) {
      if (tracktype->second=="grade1") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(1);

        return;
      }
      else if (tracktype->second=="grade2") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(2);

        return;
      }
      else if (tracktype->second=="grade3") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(3);

        return;
      }
      else if (tracktype->second=="grade4") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(4);

        return;
      }
      else if (tracktype->second=="grade5") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

      if (tagRegistry.GetGradeForSurface(surface->second,
                                        grade)) {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  AdminLevelFeatureValue& AdminLevelFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const AdminLevelFeatureValue&>(other);

      adminLevel=otherValue.adminLevel;
      isIn=otherValue.isIn;
    }

    return *this;
  }

  bool AdminLevelFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const AdminLevelFeatureValue&>(other);

    return adminLevel==otherValue.adminLevel && isIn==otherValue.isIn;
  }

  const char* const AdminLevelFeature::NAME = "AdminLevel";

  void AdminLevelFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAdminLevel=tagRegistry.RegisterTag("admin_level");
    tagIsIn=tagRegistry.RegisterTag("is_in");
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
                                const TagRegistry& /*tagRegistry*/,
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
        auto* value=static_cast<AdminLevelFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  PostalCodeFeatureValue& PostalCodeFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const PostalCodeFeatureValue&>(other);

      postalCode=otherValue.postalCode;
    }

    return *this;
  }

  bool PostalCodeFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const PostalCodeFeatureValue&>(other);

    return postalCode==otherValue.postalCode;
  }

  const char* const PostalCodeFeature::NAME = "PostalCode";

  PostalCodeFeature::PostalCodeFeature()
  {
    RegisterLabel(0,NAME);
  }

  void PostalCodeFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagPostalCode=tagRegistry.RegisterTag("postal_code");
    tagAddrPostCode=tagRegistry.RegisterTag("addr:postcode");
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
                                const TagRegistry& /*tagRegistry*/,
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
        auto* value=static_cast<PostalCodeFeatureValue*>(fv);

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

  WebsiteFeatureValue& WebsiteFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const WebsiteFeatureValue&>(other);

      website=otherValue.website;
    }

    return *this;
  }

  bool WebsiteFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const WebsiteFeatureValue&>(other);

    return website==otherValue.website;
  }

  const char* const WebsiteFeature::NAME = "Website";

  WebsiteFeature::WebsiteFeature()
  {
    RegisterLabel(0,NAME);
  }

  void WebsiteFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagWebsite=tagRegistry.RegisterTag("website");
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
                             const TagRegistry& /*tagRegistry*/,
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
        auto* value=static_cast<WebsiteFeatureValue*>(fv);

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

  PhoneFeatureValue& PhoneFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const PhoneFeatureValue&>(other);

      phone=otherValue.phone;
    }

    return *this;
  }

  bool PhoneFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const PhoneFeatureValue&>(other);

    return phone==otherValue.phone;
  }

  const char* const PhoneFeature::NAME = "Phone";

  PhoneFeature::PhoneFeature()
  {
    RegisterLabel(0,NAME);
  }

  void PhoneFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagPhone=tagRegistry.RegisterTag("phone");
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
                           const TagRegistry& /*tagRegistry*/,
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
        auto* value=static_cast<PhoneFeatureValue*>(fv);

        value->SetPhone(strValue);
      }
    }
    catch (const std::exception &e) {
      errorReporter.ReportTag(object,tags,std::string("Phone parse exception: ")+e.what());
    }
  }

  const char* const BridgeFeature::NAME = "Bridge";

  void BridgeFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagBridge=tagRegistry.RegisterTag("bridge");
  }

  std::string BridgeFeature::GetName() const
  {
    return NAME;
  }

  void BridgeFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TagRegistry& /*tagRegistry*/,
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

  void TunnelFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagTunnel=tagRegistry.RegisterTag("tunnel");
  }

  std::string TunnelFeature::GetName() const
  {
    return NAME;
  }

  void TunnelFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TagRegistry& /*tagRegistry*/,
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

  void EmbankmentFeature::Initialize(TagRegistry& tagRegistry)
  {
        tagEmbankment=tagRegistry.RegisterTag("embankment");
  }

  std::string EmbankmentFeature::GetName() const
  {
        return NAME;
  }

  void EmbankmentFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                const TagRegistry& /*tagRegistry*/,
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

  void RoundaboutFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagJunction=tagRegistry.RegisterTag("junction");
  }

  std::string RoundaboutFeature::GetName() const
  {
    return NAME;
  }

  void RoundaboutFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                const TagRegistry& /*tagRegistry*/,
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

  EleFeatureValue& EleFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const EleFeatureValue&>(other);

      ele=otherValue.ele;
    }

    return *this;
  }

  bool EleFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const EleFeatureValue&>(other);

    return ele==otherValue.ele;
  }

  const char* const EleFeature::NAME             = "Ele";
  const char* const EleFeature::NAME_LABEL       = "inMeter";
  const size_t      EleFeature::NAME_LABEL_INDEX = 0;

  EleFeature::EleFeature()
  : tagEle(0)
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void EleFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagEle=tagRegistry.RegisterTag("ele");
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
                         const TagRegistry& /*tagRegistry*/,
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
      auto* value=static_cast<EleFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

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

  DestinationFeatureValue& DestinationFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const DestinationFeatureValue&>(other);

      destination=otherValue.destination;
    }

    return *this;
  }

  bool DestinationFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const DestinationFeatureValue&>(other);

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
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void DestinationFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagDestination=tagRegistry.RegisterTag("destination");
    tagDestinationRef=tagRegistry.RegisterTag("destination:ref");
    tagDestinationForward=tagRegistry.RegisterTag("destination:forward");
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
                                 const TagRegistry& /*tagRegistry*/,
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
      auto* value=static_cast<DestinationFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetDestination(destination->second);
    }
  }

  const char* const BuildingFeature::NAME = "Building";

  void BuildingFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagBuilding=tagRegistry.RegisterTag("building");
  }

  std::string BuildingFeature::GetName() const
  {
    return NAME;
  }

  void BuildingFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TagRegistry& /*tagRegistry*/,
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

  IsInFeatureValue& IsInFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const IsInFeatureValue&>(other);

      isIn=otherValue.isIn;
    }

    return *this;
  }

  bool IsInFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const IsInFeatureValue&>(other);

    return isIn==otherValue.isIn;
  }

  const char* const IsInFeature::NAME = "IsIn";

  void IsInFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagIsIn=tagRegistry.RegisterTag("is_in");
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
                          const TagRegistry& /*tagRegistry*/,
                          const FeatureInstance& feature,
                          const ObjectOSMRef& /*object*/,
                          const TagMap& tags,
                          FeatureValueBuffer& buffer) const
  {
    auto isIn=tags.find(tagIsIn);

    if (isIn!=tags.end() && !isIn->second.empty()) {
      auto* value=static_cast<IsInFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));
      value->SetIsIn(isIn->second);
    }
  }

  void ConstructionYearFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(startYear);
    scanner.Read(endYear);
  }

  void ConstructionYearFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(startYear);
    writer.Write(endYear);
  }

  ConstructionYearFeatureValue& ConstructionYearFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const ConstructionYearFeatureValue&>(other);

      startYear=otherValue.startYear;
      endYear=otherValue.endYear;
    }

    return *this;
  }

  bool ConstructionYearFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const ConstructionYearFeatureValue&>(other);

    return startYear==otherValue.startYear &&
      endYear==otherValue.endYear;
  }

  const char* const ConstructionYearFeature::NAME = "ConstructionYear";

  ConstructionYearFeature::ConstructionYearFeature()
  {
    RegisterLabel(0,NAME);
  }

  void ConstructionYearFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagConstructionYear=tagRegistry.RegisterTag("year_of_construction");
    tagStartDate=tagRegistry.RegisterTag("start_date");
  }

  std::string ConstructionYearFeature::GetName() const
  {
    return NAME;
  }

  size_t ConstructionYearFeature::GetValueSize() const
  {
    return sizeof(ConstructionYearFeatureValue);
  }

  FeatureValue* ConstructionYearFeature::AllocateValue(void* buffer)
  {
    return new (buffer) ConstructionYearFeatureValue();
  }

  void ConstructionYearFeature::Parse(TagErrorReporter& errorReporter,
                                      const TagRegistry& /*tagRegistry*/,
                                      const FeatureInstance& feature,
                                      const ObjectOSMRef& object,
                                      const TagMap& tags,
                                      FeatureValueBuffer& buffer) const
  {
    auto constructionYearTag=tags.find(tagConstructionYear);

    std::string strValue;

    if (constructionYearTag!=tags.end()) {
      strValue=constructionYearTag->second;
    }
    else {
      auto startDateTag=tags.find(tagStartDate);

      if (startDateTag!=tags.end()) {
        strValue=startDateTag->second;
      }
      else {
        return;
      }
    }

    int startYear;
    int endYear;

    if (strValue[0]=='~') {
      strValue=strValue.substr(1);
    }


    if (osmscout::StringToNumber(strValue,startYear)) {
      auto* value=dynamic_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetStartYear(startYear);
      value->SetEndYear(startYear);

      return;
    }
    else {
      auto pos=strValue.find('-');

      if (pos!=std::string::npos) {
        std::string startValue=strValue.substr(0,pos);
        std::string endValue=strValue.substr(pos+1);

        if (!startValue.empty() &&
          !endValue.empty() &&
          osmscout::StringToNumber(startValue,startYear) &&
          osmscout::StringToNumber(endValue,endYear)) {

          auto* value=static_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

          value->SetStartYear(startYear);
          value->SetEndYear(endYear);
        }

        return;
      }

      pos=strValue.find('/');

      if (pos!=std::string::npos) {
        std::string startValue=strValue.substr(0,pos);
        std::string endValue=strValue.substr(pos+1);

        if (!startValue.empty() &&
            !endValue.empty() &&
            osmscout::StringToNumber(startValue,startYear) &&
            osmscout::StringToNumber(endValue,endYear)) {

          auto* value=static_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

          value->SetStartYear(startYear);
          value->SetEndYear(endYear);
        }

        return;
      }

      pos=strValue.find("..");

      if (pos!=std::string::npos) {
        std::string startValue=strValue.substr(0,pos);
        std::string endValue=strValue.substr(pos+2);

        if (!startValue.empty() &&
            !endValue.empty() &&
            osmscout::StringToNumber(startValue,startYear) &&
            osmscout::StringToNumber(endValue,endYear)) {

          auto* value=static_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

          value->SetStartYear(startYear);
          value->SetEndYear(endYear);
        }

        return;
      }

      if (strValue[0]=='C') {
        std::string startValue=strValue.substr(1);

        if (osmscout::StringToNumber(startValue,startYear)) {
          auto* value=static_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

          value->SetStartYear(startYear*100);
          value->SetEndYear((startYear+1)*100-1);

          return;
        }
      }

      errorReporter.ReportTag(object,tags,std::string("Construction startYear tag value '")+strValue+"' cannot be parsed to a startYear!");
    }
  }

  void SidewayFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(featureSet);
  }

  void SidewayFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(featureSet);
  }

  SidewayFeatureValue& SidewayFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const SidewayFeatureValue&>(other);

      featureSet=otherValue.featureSet;
    }

    return *this;
  }

  bool SidewayFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const SidewayFeatureValue&>(other);

    return featureSet==otherValue.featureSet;
  }

  const char* const SidewayFeature::NAME = "Sideway";

  SidewayFeature::SidewayFeature()
  {
    RegisterFlag((size_t)FeatureFlags::sidewalkTrackLeft,"sidewalkTrackLeft");
    RegisterFlag((size_t)FeatureFlags::sidewalkTrackRight,"sidewalkTrackRight");

    RegisterFlag((size_t)FeatureFlags::cyclewayLaneLeft,"cyclewayLaneLeft");
    RegisterFlag((size_t)FeatureFlags::cyclewayLaneRight,"cyclewayLaneRight");
    RegisterFlag((size_t)FeatureFlags::cyclewayTrackLeft,"cyclewayTrackLeft");
    RegisterFlag((size_t)FeatureFlags::cyclewayTrackRight,"cyclewayTrackRight");
  }

  void SidewayFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagSidewalk=tagRegistry.RegisterTag("sidewalk");
    tagCyclewayLeft=tagRegistry.RegisterTag("cycleway:left");
    tagCyclewayLeftSegregated=tagRegistry.RegisterTag("cycleway:left:segregated");
    tagCyclewayRight=tagRegistry.RegisterTag("cycleway:right");
    tagCyclewayRightSegregated=tagRegistry.RegisterTag("cycleway:right:segregated");
  }

  std::string SidewayFeature::GetName() const
  {
    return NAME;
  }

  size_t SidewayFeature::GetValueSize() const
  {
    return sizeof(SidewayFeatureValue);
  }

  FeatureValue* SidewayFeature::AllocateValue(void* buffer)
  {
    return new (buffer) SidewayFeatureValue();
  }

  void SidewayFeature::Parse(TagErrorReporter& /*errorReporter*/,
                             const TagRegistry& /*tagRegistry*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    uint8_t featureSet=0;
    auto sidewalkTag=tags.find(tagSidewalk);
    auto cyclewayLeftTag=tags.find(tagCyclewayLeft);
    auto cyclewayRightTag=tags.find(tagCyclewayRight);
    auto cyclewayLeftSegregatedTag=tags.find(tagCyclewayLeftSegregated);
    auto cyclewayRightSegregatedTag=tags.find(tagCyclewayRightSegregated);

    bool hasSidewalkTrackLeft=sidewalkTag!=tags.end() &&
                              (sidewalkTag->second=="left" ||
                               sidewalkTag->second=="both");

    bool hasSidewalkTrackRight=sidewalkTag!=tags.end() &&
                               (sidewalkTag->second=="right" ||
                                sidewalkTag->second=="both");

    bool hasCyclewayLaneLeft=cyclewayLeftTag!=tags.end() &&
                             (cyclewayLeftTag->second=="lane" ||
                              cyclewayLeftTag->second=="shared_lane");

    bool hasCyclewayLaneRight=cyclewayRightTag!=tags.end() &&
                              (cyclewayRightTag->second=="lane" ||
                               cyclewayRightTag->second=="shared_lane");

    bool hasCyclewayTrackLeft=cyclewayLeftTag!=tags.end() &&
                              cyclewayLeftTag->second=="track";

    bool hasCyclewayTrackRight=cyclewayRightTag!=tags.end() &&
                               cyclewayRightTag->second=="track";

    bool hasCyclewayLeftSegregated=cyclewayLeftSegregatedTag!=tags.end();
    bool hasCyclewayRightSegregated=cyclewayRightSegregatedTag!=tags.end();

    if (hasSidewalkTrackLeft) {
      featureSet|=SidewayFeatureValue::sidewalkTrackLeft;
    }
    if (hasSidewalkTrackRight) {
      featureSet|=SidewayFeatureValue::sidewalkTrackRight;
    }

    if (hasCyclewayLaneLeft) {
      featureSet|=SidewayFeatureValue::cyclewayLaneLeft;
    }
    if (hasCyclewayLaneRight) {
      featureSet|=SidewayFeatureValue::cyclewayLaneRight;
    }

    if (hasCyclewayTrackLeft) {
      featureSet|=SidewayFeatureValue::cyclewayTrackLeft;

      if (hasCyclewayLeftSegregated) {
        featureSet|=SidewayFeatureValue::sidewalkTrackLeft;
      }
    }
    if (hasCyclewayTrackRight) {
      featureSet|=SidewayFeatureValue::cyclewayTrackRight;

      if (hasCyclewayRightSegregated) {
        featureSet|=SidewayFeatureValue::sidewalkTrackRight;
      }
    }

    if (featureSet!=0) {
      auto* value=static_cast<SidewayFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetFeatureSet(featureSet);
    }
  }

  void LanesFeatureValue::Read(FileScanner& scanner)
  {
    scanner.Read(lanes);

    if (lanes & 0x01) {
      scanner.Read(turnForward);
      scanner.Read(turnBackward);
      scanner.Read(destinationForward);
      scanner.Read(destinationBackward);
    }
  }

  void LanesFeatureValue::Write(FileWriter& writer)
  {
    if (turnForward.empty() &&
        turnBackward.empty() &&
        destinationForward.empty() &&
        destinationBackward.empty()) {
      lanes=lanes & ~0x01;
    }
    else {
      lanes=lanes | 0x01;
    }

    writer.Write(lanes);

    if (lanes & 0x01) {
      writer.Write(turnForward);
      writer.Write(turnBackward);
      writer.Write(destinationForward);
      writer.Write(destinationBackward);
    }
  }

  LanesFeatureValue& LanesFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const LanesFeatureValue&>(other);

      lanes=otherValue.lanes;
      turnForward=otherValue.turnForward;
      turnBackward=otherValue.turnBackward;
      destinationForward=otherValue.destinationForward;
      destinationBackward=otherValue.destinationBackward;
    }

    return *this;
  }

  bool LanesFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const LanesFeatureValue&>(other);

    return lanes==otherValue.lanes &&
           turnForward==otherValue.turnForward &&
           turnBackward==otherValue.turnBackward &&
           destinationForward==otherValue.destinationForward &&
           destinationBackward==otherValue.destinationBackward;
  }

  uint8_t LanesFeatureValue::GetLanes() const
  {
    if (lanes & 0x01) {
      return 1;
    }
    else {
      return GetForwardLanes() + GetBackwardLanes();
    }
  }

  const char* const LanesFeature::NAME             = "Lanes";
  const char* const LanesFeature::NAME_LABEL       = "label";
  const size_t      LanesFeature::NAME_LABEL_INDEX = 0;

  LanesFeature::LanesFeature()
    : tagOneway(0),
       tagLanes(0)

  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void LanesFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagOneway=tagRegistry.RegisterTag("oneway");
    tagLanes=tagRegistry.RegisterTag("lanes");
    tagLanesForward=tagRegistry.RegisterTag("lanes:forward");
    tagLanesBackward=tagRegistry.RegisterTag("lanes:backward");
    tagTurnLanes=tagRegistry.RegisterTag("turn:lanes");
    tagTurnLanesForward=tagRegistry.RegisterTag("turn:lanes:forward");
    tagTurnLanesBackward=tagRegistry.RegisterTag("turn:lanes:backward");
    tagDestinationLanes=tagRegistry.RegisterTag("destination:lanes");
    tagDestinationLanesForward=tagRegistry.RegisterTag("destination:lanes:forward");
    tagDestinationLanesBackward=tagRegistry.RegisterTag("destination:lanes:backward");
  }

  std::string LanesFeature::GetName() const
  {
    return NAME;
  }

  size_t LanesFeature::GetValueSize() const
  {
    return sizeof(LanesFeatureValue);
  }

  FeatureValue* LanesFeature::AllocateValue(void* buffer)
  {
    return new (buffer) LanesFeatureValue();
  }

  void LanesFeature::Parse(TagErrorReporter& errorReporter,
                           const TagRegistry& /*tagRegistry*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    bool        oneway=false;
    bool        additionalInfos=false;
    uint8_t     lanes=0;
    uint8_t     lanesForward=0;
    uint8_t     lanesBackward=0;
    auto        onewayTag=tags.find(tagOneway);
    auto        lanesTag=tags.find(tagLanes);
    auto        lanesForwardTag=tags.find(tagLanesForward);
    auto        lanesBackwardTag=tags.find(tagLanesBackward);
    auto        turnLanesTag=tags.find(tagTurnLanes);
    auto        turnLanesForwardTag=tags.find(tagTurnLanesForward);
    auto        turnLanesBackwardTag=tags.find(tagTurnLanesBackward);
    auto        destinationLanesTag=tags.find(tagDestinationLanes);
    auto        destinationLanesForwardTag=tags.find(tagDestinationLanesForward);
    auto        destinationLanesBackwardTag=tags.find(tagDestinationLanesBackward);
    std::string turnForward;
    std::string turnBackward;
    std::string destinationForward;
    std::string destinationBackward;

    if (onewayTag!=tags.end()) {
      // TODO: What happens with -1?
      oneway=onewayTag->second!="no" && onewayTag->second!="false" && onewayTag->second!="0";
    }

    if (lanesTag!=tags.end() &&
      !StringToNumber(lanesTag->second,lanes)) {
      errorReporter.ReportTag(object,tags,std::string("lanes tag value '")+lanesTag->second+"' is not numeric!");

      return;
    }

    if (lanesForwardTag!=tags.end() &&
        !StringToNumber(lanesForwardTag->second,lanesForward)) {
      errorReporter.ReportTag(object,tags,std::string("lanes:forward tag value '")+lanesForwardTag->second+"' is not numeric!");

      return;
    }

    if (lanesBackwardTag!=tags.end() &&
        !StringToNumber(lanesBackwardTag->second,lanesBackward)) {
      errorReporter.ReportTag(object,tags,std::string("lanes:backward tag value '")+lanesBackwardTag->second+"' is not numeric!");

      return;
    }

    /* Too many warnings :-/
    if (!oneway && lanesTag!=tags.end() && lanes%2 != 0) {
      errorReporter.ReportTag(object,tags,std::string("No oneway, but lanes tag is set with uneven value"));
    }*/

    if (!oneway &&
        turnLanesTag!=tags.end()) {
      errorReporter.ReportTag(object,tags,std::string("No oneway, but turn:lanes tag is set"));
    }

    if (!oneway &&
        destinationLanesTag!=tags.end()) {
      errorReporter.ReportTag(object,tags,std::string("No oneway, but destination:lanes tag is set"));
    }

    if (turnLanesTag!=tags.end()) {
      turnForward=turnLanesTag->second;
      additionalInfos=true;
    }

    if (turnLanesForwardTag!=tags.end()) {
      turnForward=turnLanesForwardTag->second;
      additionalInfos=true;
    }

    if (turnLanesBackwardTag!=tags.end()) {
      turnBackward=turnLanesBackwardTag->second;
      additionalInfos=true;
    }

    if (destinationLanesTag!=tags.end()) {
      destinationForward=destinationLanesTag->second;
      additionalInfos=true;
    }

    if (destinationLanesForwardTag!=tags.end()) {
      destinationForward=destinationLanesForwardTag->second;
      additionalInfos=true;
    }

    if (destinationLanesBackwardTag!=tags.end()) {
      destinationBackward=destinationLanesBackwardTag->second;
      additionalInfos=true;
    }

    if (lanesForwardTag!=tags.end() &&
        lanesBackwardTag!=tags.end() &&
        lanesTag!=tags.end()) {
      if (lanesForward+lanesBackward!=lanes) {
        errorReporter.ReportTag(object,tags,std::string("lanes tag value '")+lanesTag->second+"' is not equal sum of lanes:forward and lanes:backward");
      }
    }
    else if (lanesForwardTag!=tags.end() &&
             lanesTag!=tags.end()) {
      lanesBackward=lanes-lanesForward;
    }
    else if (lanesBackwardTag!=tags.end() &&
             lanesTag!=tags.end()) {
      lanesForward=lanes-lanesBackward;
    }
    else if (lanesTag!=tags.end()) {
      if (oneway) {
        lanesForward=lanes;
        lanesBackward=0;
      }
      else {
        lanesForward=lanes/(uint8_t)2;
        lanesBackward=lanes/(uint8_t)2;
      }
    }
    else {
      return;
    }

    if (!additionalInfos) {
      if (oneway) {
        if (lanes==feature.GetType()->GetOnewayLanes()) {
          return;
        }
      }
      else {
        if (lanes==feature.GetType()->GetLanes()) {
          return;
        }
      }
    }

    auto* value=static_cast<LanesFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

    if (lanes==1) {
      // One (most of the time implicit) lane for both directions together
      value->SetLanes(0,0);
    }
    else {
      value->SetLanes(lanesForward,lanesBackward);
    }

    if (additionalInfos) {
      value->SetTurnLanes(turnForward,turnBackward);
      value->SetDestinationLanes(destinationForward,destinationBackward);
    }
  }
}
