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

#include <osmscout/feature/MaxSpeedFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void MaxSpeedFeatureValue::Read(FileScanner& scanner)
  {
    maxSpeed=scanner.ReadUInt8();
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

  size_t MaxSpeedFeature::GetValueAlignment() const
  {
    return alignof(MaxSpeedFeatureValue);
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
      if (valueNumeric>std::numeric_limits<uint8_t>::max()/static_cast<unsigned long>(lround(1.609))) {

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
}
