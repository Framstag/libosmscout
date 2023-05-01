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

#include <osmscout/feature/FromToFeature.h>

namespace osmscout {

  void FromToFeatureValue::Read(FileScanner& scanner)
  {
    from=scanner.ReadString();
    to=scanner.ReadString();
  }

  void FromToFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(from);
    writer.Write(to);
  }

  FromToFeatureValue& FromToFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const FromToFeatureValue&>(other);

      from=otherValue.from;
      to=otherValue.to;
    }

    return *this;
  }

  bool FromToFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const FromToFeatureValue&>(other);

    return from==otherValue.from &&
           to==otherValue.to;
  }

  const char* const FromToFeature::NAME = "FromTo";
  const char* const FromToFeature::NUMBER_LABEL = "number";
  const size_t      FromToFeature::NUMBER_LABEL_INDEX = 0;

  FromToFeature::FromToFeature()
  {
    RegisterLabel(NUMBER_LABEL_INDEX,NUMBER_LABEL);
  }

  void FromToFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagFrom=tagRegistry.RegisterTag("from");
    tagTo=tagRegistry.RegisterTag("to");
  }

  std::string FromToFeature::GetName() const
  {
    return NAME;
  }

  size_t FromToFeature::GetValueAlignment() const
  {
    return alignof(FromToFeatureValue);
  }

  size_t FromToFeature::GetValueSize() const
  {
    return sizeof(FromToFeatureValue);
  }

  FeatureValue* FromToFeature::AllocateValue(void* buffer)
  {
    return new (buffer) FromToFeatureValue();
  }

  void FromToFeature::Parse(TagErrorReporter& /*errorReporter*/,
                             const TagRegistry& /*tagRegistry*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    auto from=tags.find(tagFrom);
    auto to=tags.find(tagTo);

    if ((from!=tags.end() && !from->second.empty()) ||
        (to!=tags.end() && !to->second.empty())) {
      auto* value = static_cast<FromToFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      if (from!=tags.end()) {
        value->SetFrom(from->second);
      }

      if (to!=tags.end()) {
        value->SetTo(to->second);
      }
    }
  }
}
