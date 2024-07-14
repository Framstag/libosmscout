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

#include <osmscout/feature/AddressFeature.h>

namespace osmscout {

  void AddressFeatureValue::Read(FileScanner& scanner)
  {
    address=scanner.ReadString();
  }

  void AddressFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(address);
  }

  AddressFeatureValue& AddressFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const AddressFeatureValue&>(other);

      address=otherValue.address;
    }

    return *this;
  }

  bool AddressFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const AddressFeatureValue&>(other);

    return address==otherValue.address;
  }

  const char* const AddressFeature::NAME             = "Address";
  const char* const AddressFeature::NAME_LABEL       = "name";
  const size_t      AddressFeature::NAME_LABEL_INDEX = 0;


  AddressFeature::AddressFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void AddressFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAddrHouseNr=tagRegistry.RegisterTag("addr:housenumber");
    tagAddrStreet=tagRegistry.RegisterTag("addr:street");
    tagAddrPlace=tagRegistry.RegisterTag("addr:place");
  }

  std::string AddressFeature::GetName() const
  {
    return NAME;
  }

  size_t AddressFeature::GetValueAlignment() const
  {
    return alignof(AddressFeatureValue);
  }

  size_t AddressFeature::GetValueSize() const
  {
    return sizeof(AddressFeatureValue);
  }

  FeatureValue* AddressFeature::AllocateValue(void* buffer)
  {
    return new (buffer) AddressFeatureValue();
  }

  void AddressFeature::Parse(TagErrorReporter& /*errorReporter*/,
                             const TagRegistry& /*tagRegistry*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    auto street=tags.find(tagAddrStreet);

    if (street==tags.end()) {
      // We are cheating here, but from library view, there is no
      // difference in addr:street or addr:place. It is just an address.
      street=tags.find(tagAddrPlace);
      if (street==tags.end()) {
        return;
      }
    }

    auto houseNr=tags.find(tagAddrHouseNr);

    if (houseNr==tags.end()) {
      return;
    }

    if (!street->second.empty() &&
        !houseNr->second.empty()) {
      auto* value=static_cast<AddressFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetAddress(houseNr->second);
    }
  }
}
