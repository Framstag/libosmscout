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

#include <osmscout/feature/IsInFeature.h>

namespace osmscout {

  void IsInFeatureValue::Read(FileScanner& scanner)
  {
    isIn=scanner.ReadString();
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

  size_t IsInFeature::GetValueAlignment() const
  {
    return alignof(IsInFeatureValue);
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
}
