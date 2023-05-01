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

#include <osmscout/feature/AccessFeature.h>

namespace osmscout {

  void AccessFeatureValue::Read(FileScanner& scanner)
  {
    access=scanner.ReadUInt8();
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

  size_t AccessFeature::GetValueAlignment() const
  {
    return alignof(AccessFeatureValue);
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
        if ((access & AccessFeatureValue::onewayBackward)==0) {
          access|=(AccessFeatureValue::bicycleForward);
        }
        if ((access & AccessFeatureValue::onewayForward)==0) {
          access|=(AccessFeatureValue::bicycleBackward);
        }
      }
    }

    auto accessMotorVehicleValue=tags.find(tagMotorVehicle);

    if (accessMotorVehicleValue!=tags.end()) {
      access&=~(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);

      if (!(accessMotorVehicleValue->second=="no")) {
        if ((access & AccessFeatureValue::onewayBackward)==0) {
          access|=(AccessFeatureValue::carForward);
        }
        if ((access & AccessFeatureValue::onewayForward)==0) {
          access|=(AccessFeatureValue::carBackward);
        }
      }
    }

    auto accessMotorcarValue=tags.find(tagMotorcar);

    if (accessMotorcarValue!=tags.end()) {
      access&=~(AccessFeatureValue::carForward|AccessFeatureValue::carBackward);

      if (!(accessMotorcarValue->second=="no")) {
        if ((access & AccessFeatureValue::onewayBackward)==0) {
          access|=(AccessFeatureValue::carForward);
        }
        if ((access & AccessFeatureValue::onewayForward)==0) {
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
}
