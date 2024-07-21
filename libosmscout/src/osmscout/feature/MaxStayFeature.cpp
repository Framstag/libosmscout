/*
This source is part of the libosmscout library
Copyright (C) 2024  Tim Teulings

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

#include <osmscout/feature/MaxStayFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void MaxStayFeatureValue::Read(FileScanner& scanner)
  {
    value=scanner.ReadString();
    condition=scanner.ReadString();
  }

  void MaxStayFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(value);
    writer.Write(condition);
  }

  MaxStayFeatureValue& MaxStayFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const MaxStayFeatureValue&>(other);

      value=otherValue.value;
      condition=otherValue.condition;
    }

    return *this;
  }

  bool MaxStayFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const MaxStayFeatureValue&>(other);

    return value==otherValue.value &&
           condition==otherValue.condition;
  }

  const char* const MaxStayFeature::NAME = "MaxStay";

  void MaxStayFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagMaxStay=tagRegistry.RegisterTag("maxstay");
    tagMaxStayCondition=tagRegistry.RegisterTag("maxstay:condition");
  }

  std::string MaxStayFeature::GetName() const
  {
    return NAME;
  }

  size_t MaxStayFeature::GetValueAlignment() const
  {
    return alignof(MaxStayFeatureValue);
  }

  size_t MaxStayFeature::GetValueSize() const
  {
    return sizeof(MaxStayFeatureValue);
  }

  FeatureValue* MaxStayFeature::AllocateValue(void* buffer)
  {
    return new (buffer) MaxStayFeatureValue();
  }

  void MaxStayFeature::Parse(TagErrorReporter& /*errorReporter*/,
                           const TagRegistry& /*tagRegistry*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& /*object*/,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    auto maxStayTag=tags.find(tagMaxStay);

    if (maxStayTag!=tags.end() &&
        !maxStayTag->second.empty()) {
      auto* value=static_cast<MaxStayFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetValue(maxStayTag->second);

      auto maxStayTagConditionTag=tags.find(tagMaxStayCondition);

      if (maxStayTagConditionTag!=tags.end() &&
          !maxStayTagConditionTag->second.empty()) {
        value->SetCondition(maxStayTagConditionTag->second);
      }
    }
  }
}
