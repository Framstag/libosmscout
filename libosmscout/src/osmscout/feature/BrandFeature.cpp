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

#include <osmscout/feature/BrandFeature.h>

namespace osmscout {

  void BrandFeatureValue::Read(FileScanner& scanner)
  {
    name=scanner.ReadString();
  }

  void BrandFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(name);
  }


  BrandFeatureValue& BrandFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const BrandFeatureValue&>(other);

      name=otherValue.name;
    }

    return *this;
  }

  bool BrandFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const BrandFeatureValue&>(other);

    return name==otherValue.name;
  }

  const char* const BrandFeature::NAME             = "Brand";
  const char* const BrandFeature::NAME_LABEL       = "name";
  const size_t      BrandFeature::NAME_LABEL_INDEX = 0;


  BrandFeature::BrandFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void BrandFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagBrand=tagRegistry.RegisterTag("brand");
  }

  std::string BrandFeature::GetName() const
  {
    return NAME;
  }

  size_t BrandFeature::GetValueAlignment() const
  {
    return alignof(BrandFeatureValue);
  }

  size_t BrandFeature::GetValueSize() const
  {
    return sizeof(BrandFeatureValue);
  }

  FeatureValue* BrandFeature::AllocateValue(void* buffer)
  {
    return new (buffer) BrandFeatureValue();
  }

  void BrandFeature::Parse(TagErrorReporter& /*errorReporter*/,
                          const TagRegistry& /*tagRegistry*/,
                          const FeatureInstance& feature,
                          const ObjectOSMRef& /*object*/,
                          const TagMap& tags,
                          FeatureValueBuffer& buffer) const
  {
    auto brand=tags.find(tagBrand);

    if (brand==tags.end()) {
      return;
    }

    if (!brand->second.empty()) {
      auto* value=static_cast<BrandFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetName(brand->second);
    }

  }
}
