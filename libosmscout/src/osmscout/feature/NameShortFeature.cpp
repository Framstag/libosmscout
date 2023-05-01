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

#include <osmscout/feature/NameShortFeature.h>

namespace osmscout {

  void NameShortFeatureValue::Read(FileScanner& scanner)
  {
    nameShort=scanner.ReadString();
  }

  void NameShortFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(nameShort);
  }

  NameShortFeatureValue& NameShortFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const NameShortFeatureValue&>(other);

      nameShort=otherValue.nameShort;
    }

    return *this;
  }

  bool NameShortFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const NameShortFeatureValue&>(other);

    return nameShort==otherValue.nameShort;
  }

  const char* const NameShortFeature::NAME             = "ShortName";
  const char* const NameShortFeature::NAME_LABEL       = "name";
  const size_t      NameShortFeature::NAME_LABEL_INDEX = 0;

  NameShortFeature::NameShortFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void NameShortFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagShortName=tagRegistry.RegisterTag("short_name");
  }

  std::string NameShortFeature::GetName() const
  {
    return NAME;
  }

  size_t NameShortFeature::GetValueAlignment() const
  {
    return alignof(NameShortFeatureValue);
  }

  size_t NameShortFeature::GetValueSize() const
  {
    return sizeof(NameShortFeatureValue);
  }

  FeatureValue* NameShortFeature::AllocateValue(void* buffer)
  {
    return new (buffer) NameShortFeatureValue();
  }

  void NameShortFeature::Parse(TagErrorReporter& /*errorReporter*/,
                               const TagRegistry& /*tagRegistry*/,
                               const FeatureInstance& feature,
                               const ObjectOSMRef& /*object*/,
                               const TagMap& tags,
                               FeatureValueBuffer& buffer) const
  {
    auto shortName=tags.find(tagShortName);

    if (shortName!=tags.end() &&
        !shortName->second.empty()) {
      auto* value=static_cast<NameShortFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetNameShort(shortName->second);
    }
  }
}
