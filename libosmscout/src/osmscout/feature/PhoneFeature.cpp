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

#include <osmscout/feature/PhoneFeature.h>

#include <set>

namespace osmscout {

  void PhoneFeatureValue::Read(FileScanner& scanner)
  {
    phone=scanner.ReadString();
  }

  void PhoneFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(phone);
  }

  PhoneFeatureValue& PhoneFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const PhoneFeatureValue&>(other);

      phone=otherValue.phone;
    }

    return *this;
  }

  bool PhoneFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const PhoneFeatureValue&>(other);

    return phone==otherValue.phone;
  }

  const char* const PhoneFeature::NAME = "Phone";
  const char* const PhoneFeature::NUMBER_LABEL = "number";
  const size_t      PhoneFeature::NUMBER_LABEL_INDEX = 0;

  PhoneFeature::PhoneFeature()
  {
    RegisterLabel(NUMBER_LABEL_INDEX,NUMBER_LABEL);
  }

  void PhoneFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagPhone=tagRegistry.RegisterTag("phone");
    tagContactPhone=tagRegistry.RegisterTag("contact:phone");
    tagContactMobile=tagRegistry.RegisterTag("contact:mobile");
  }

  std::string PhoneFeature::GetName() const
  {
    return NAME;
  }

  size_t PhoneFeature::GetValueAlignment() const
  {
    return alignof(PhoneFeatureValue);
  }

  size_t PhoneFeature::GetValueSize() const
  {
    return sizeof(PhoneFeatureValue);
  }

  FeatureValue* PhoneFeature::AllocateValue(void* buffer)
  {
    return new (buffer) PhoneFeatureValue();
  }

  void PhoneFeature::Parse(TagErrorReporter& errorReporter,
                           const TagRegistry& /*tagRegistry*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    std::string strValue;

    // object may hold multiple phone tags - phone, contact:phone, contact:mobile
    // we will append all unique values to one string separated by semicolon
    // note: when some tag contains multiple phones separated by semicolon, deduplication is not working
    std::set<std::string,std::less<>> knownPhones;
    std::vector<TagId> phoneTags{tagPhone, tagContactPhone, tagContactMobile};
    for (auto tagId:phoneTags) {
      auto phone = tags.find(tagId);
      if (phone == tags.end()) {
        continue;
      }

      std::string phoneStr=phone->second;
      // remove invalid characters from phone number [0123456789+;,] http://wiki.openstreetmap.org/wiki/Key:phone
      // - there can be multiple phone numbers separated by semicolon (some mappers use comma)
      phoneStr.erase(
          std::remove_if(phoneStr.begin(), phoneStr.end(), [](char x){return (x<'0'||x>'9') && x!='+' && x!=';' && x!=',';}),
          phoneStr.end());

      if (phoneStr.empty() || knownPhones.find(phoneStr) != knownPhones.end()) {
        continue;
      }
      if (!strValue.empty()) {
        strValue += ";";
      }
      strValue += phoneStr;
      knownPhones.insert(phoneStr);
    }

    try {
      if (!strValue.empty()) {
        size_t idx = feature.GetIndex();
        FeatureValue* fv = buffer.AllocateValue(idx);
        auto* value = static_cast<PhoneFeatureValue*>(fv);

        value->SetPhone(strValue);
      }
    }
    catch (const std::exception &e) {
      errorReporter.ReportTag(object,tags,std::string("Phone parse exception: ")+e.what());
    }
  }
}
