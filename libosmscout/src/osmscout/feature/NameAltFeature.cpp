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

#include <osmscout/feature/NameAltFeature.h>

namespace osmscout {

  void NameAltFeatureValue::Read(FileScanner& scanner)
  {
    nameAlt=scanner.ReadString();
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

  size_t NameAltFeature::GetValueAlignment() const
  {
    return alignof(NameAltFeatureValue);
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
    uint32_t    nameAltPriority=std::numeric_limits<uint32_t>::max();

    for (const auto &tag : tags) {
      uint32_t natPrio;
      bool     isNameAltTag=tagRegistry.IsNameAltTag(tag.first,natPrio);

      if (isNameAltTag &&
          (nameAlt.empty() || natPrio<nameAltPriority)) {
        nameAlt=tag.second;
        nameAltPriority=natPrio;
      }
    }

    if (!nameAlt.empty()) {
      auto* value=static_cast<NameAltFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetNameAlt(nameAlt);
    }
  }
}
