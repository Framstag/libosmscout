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

#include <osmscout/util/String.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

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

  const char* const AddressFeature::NAME             = "Address";
  const char* const AddressFeature::NAME_LABEL       = "name";
  const size_t      AddressFeature::NAME_LABEL_INDEX = 0;


  AddressFeature::AddressFeature()
  : tagAddrHouseNr(0),
    tagAddrStreet(0)
  {
    RegisterLabel(NAME_LABEL,
                  NAME_LABEL_INDEX);
  }

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

  const char* const AccessRestrictedFeature::NAME = "AccessRestricted";

  void AccessRestrictedFeature::Initialize(TypeConfig& typeConfig)
  {
    tagAccess=typeConfig.RegisterTag("access");
  }

  std::string AccessRestrictedFeature::GetName() const
  {
    return NAME;
  }

  size_t AccessRestrictedFeature::GetValueSize() const
  {
    return 0;
  }

  void AccessRestrictedFeature::Parse(Progress& /*progress*/,
                                      const TypeConfig& /*typeConfig*/,
                                      const FeatureInstance& feature,
                                      const ObjectOSMRef& /*object*/,
                                      const OSMSCOUT_HASHMAP<TagId,std::string>& tags,
                                      FeatureValueBuffer& buffer) const
  {
    auto accessValue=tags.find(tagAccess);

    if (accessValue!=tags.end() &&
        accessValue->second!="no" &&
        accessValue->second!="yes" &&
        accessValue->second!="use_sidepath" &&
        accessValue->second!="permissive" &&
        accessValue->second!="designated") {
      buffer.AllocateValue(feature.GetIndex());
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
                              const TypeConfig& typeConfig,
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
        progress.Warning(std::string("Max speed tag value '")+maxSpeed->second+"' for "+object.GetName()+" is not numeric!");
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

  DynamicFeatureLabelReader::DynamicFeatureLabelReader()
  : labelIndex(std::numeric_limits<size_t>::max())
  {
    // no code
  }

  DynamicFeatureLabelReader::DynamicFeatureLabelReader(const TypeConfig& typeConfig,
                                                       const std::string& featureName,
                                                       size_t labelIndex)
  {
    Set(typeConfig,
        featureName,
        labelIndex);
  }

  bool DynamicFeatureLabelReader::Set(const TypeConfig& typeConfig,
                                      const std::string& featureName,
                                      size_t labelIndex)
  {
    // Reset state
    Clear();

    FeatureRef feature=typeConfig.GetFeature(featureName);

    if (feature.Invalid()) {
      return false;
    }

    if (!feature->HasLabel()) {
      return false;
    }

    this->labelIndex=labelIndex;

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(featureName,
                          index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }

    return true;
  }

  void DynamicFeatureLabelReader::Clear()
  {
    labelIndex=std::numeric_limits<size_t>::max();
    lookupTable.clear();
  }

  bool DynamicFeatureLabelReader::HasLabel() const
  {
    return labelIndex!=std::numeric_limits<size_t>::max();
  }

  std::string DynamicFeatureLabelReader::GetLabel(const FeatureValueBuffer& buffer) const
  {
    if (!HasLabel()) {
      return "";
    }

    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max() &&
        buffer.HasValue(index)) {
      FeatureValue *value=buffer.GetValue(index);

      if (value!=NULL) {
        return value->GetLabel();
      }
    }

    return "";

  }

}
