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

#include <osmscout/feature/PostalCodeFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void PostalCodeFeatureValue::Read(FileScanner& scanner)
  {
    postalCode=scanner.ReadString();
  }

  void PostalCodeFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(postalCode);
  }

  PostalCodeFeatureValue& PostalCodeFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const PostalCodeFeatureValue&>(other);

      postalCode=otherValue.postalCode;
    }

    return *this;
  }

  bool PostalCodeFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const PostalCodeFeatureValue&>(other);

    return postalCode==otherValue.postalCode;
  }

  const char* const PostalCodeFeature::NAME = "PostalCode";
  const char* const PostalCodeFeature::NAME_LABEL = "name";
  const size_t      PostalCodeFeature::NAME_LABEL_INDEX = 0;

  PostalCodeFeature::PostalCodeFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,NAME_LABEL);
  }

  void PostalCodeFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagPostalCode=tagRegistry.RegisterTag("postal_code");
    tagAddrPostCode=tagRegistry.RegisterTag("addr:postcode");
  }

  std::string PostalCodeFeature::GetName() const
  {
    return NAME;
  }

  size_t PostalCodeFeature::GetValueAlignment() const
  {
    return alignof(PostalCodeFeatureValue);
  }

  size_t PostalCodeFeature::GetValueSize() const
  {
    return sizeof(PostalCodeFeatureValue);
  }

  FeatureValue* PostalCodeFeature::AllocateValue(void* buffer)
  {
    return new (buffer) PostalCodeFeatureValue();
  }

  void PostalCodeFeature::Parse(TagErrorReporter& errorReporter,
                                const TagRegistry& /*tagRegistry*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& object,
                                const TagMap& tags,
                                FeatureValueBuffer& buffer) const
  {
    auto postalCode=tags.find(tagPostalCode);
    auto addrPostCode=tags.find(tagAddrPostCode);

    std::string postalCodeValue;

    if (postalCode!=tags.end()) {
      postalCodeValue = postalCode->second;
    }

    if (addrPostCode!=tags.end()) {
      postalCodeValue = addrPostCode->second;
    }

    try {
      if (!postalCodeValue.empty()) {
        size_t idx = feature.GetIndex();
        FeatureValue* fv = buffer.AllocateValue(idx);
        auto* value=static_cast<PostalCodeFeatureValue*>(fv);

        value->SetPostalCode(postalCodeValue);
      }
    }
    catch (const std::exception &e) {
      errorReporter.ReportTag(object,tags,std::string("Postal code parse exception: ")+e.what());
    }
  }
}
