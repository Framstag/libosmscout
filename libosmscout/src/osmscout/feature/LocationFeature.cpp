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

#include <osmscout/feature/LocationFeature.h>

namespace osmscout {

  void LocationFeatureValue::Read(FileScanner& scanner)
  {
    location=scanner.ReadString();
  }

  void LocationFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(location);
  }

  LocationFeatureValue& LocationFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const LocationFeatureValue&>(other);

      location=otherValue.location;
    }

    return *this;
  }

  bool LocationFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const LocationFeatureValue&>(other);

    return location==otherValue.location;
  }

  const char* const LocationFeature::NAME = "Location";

  void LocationFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAddrHouseNr=tagRegistry.RegisterTag("addr:housenumber");
    tagAddrStreet=tagRegistry.RegisterTag("addr:street");
    tagAddrPlace=tagRegistry.RegisterTag("addr:place");
  }

  std::string LocationFeature::GetName() const
  {
    return NAME;
  }

  size_t LocationFeature::GetValueAlignment() const
  {
    return alignof(LocationFeatureValue);
  }

  size_t LocationFeature::GetValueSize() const
  {
    return sizeof(LocationFeatureValue);
  }

  FeatureValue* LocationFeature::AllocateValue(void* buffer)
  {
    return new (buffer) LocationFeatureValue();
  }

  void LocationFeature::Parse(TagErrorReporter& /*errorReporter*/,
                              const TagRegistry& /*tagRegistry*/,
                              const FeatureInstance& feature,
                              const ObjectOSMRef& /*object*/,
                              const TagMap& tags,
                              FeatureValueBuffer& buffer) const
  {
    auto street=tags.find(tagAddrStreet);

    if (street==tags.end()) {
      // We are cheating here, but from library view, there is no
      // difference in addr:street or addr:place. It is just a address.
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
      auto* value=static_cast<LocationFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetLocation(street->second);
    }
  }
}
