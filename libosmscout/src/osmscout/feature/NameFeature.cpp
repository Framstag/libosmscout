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

#include <osmscout/feature/NameFeature.h>

namespace osmscout {

  void NameFeatureValue::Read(FileScanner& scanner)
  {
    name=scanner.ReadString();
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

  size_t NameFeature::GetValueAlignment() const
  {
    return alignof(NameFeatureValue);
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
    uint32_t    namePriority=std::numeric_limits<uint32_t>::max();

    for (const auto &tag : tags) {
      uint32_t ntPrio;
      bool     isNameTag=tagRegistry.IsNameTag(tag.first,ntPrio);

      if (isNameTag &&
          (name.empty() || ntPrio<namePriority)) {
        name=tag.second;
        namePriority=ntPrio;
      }
    }

    if (!name.empty()) {
      auto* value=static_cast<NameFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetName(name);
    }
  }
}
