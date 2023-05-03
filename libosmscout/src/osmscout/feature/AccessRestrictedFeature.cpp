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

#include <osmscout/feature/AccessRestrictedFeature.h>

namespace osmscout {

  void AccessRestrictedFeatureValue::Read(FileScanner& scanner)
  {
    access=scanner.ReadUInt8();
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

  size_t AccessRestrictedFeature::GetValueAlignment() const
  {
    return alignof(AccessRestrictedFeatureValue);
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
}
