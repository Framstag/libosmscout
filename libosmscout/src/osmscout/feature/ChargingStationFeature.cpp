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

#include <osmscout/feature/ChargingStationFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void ChargingStationFeatureValue::Read(FileScanner& scanner)
  {
    capacity=scanner.ReadUInt8();
  }

  void ChargingStationFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(capacity);
  }

  ChargingStationFeatureValue& ChargingStationFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const ChargingStationFeatureValue&>(other);

     this->capacity=otherValue.capacity;
    }

    return *this;
  }

  bool ChargingStationFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const ChargingStationFeatureValue&>(other);

    return this->capacity==otherValue.capacity;
  }

  const char* const ChargingStationFeature::NAME = "ChargingStation";

  void ChargingStationFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAmenity=tagRegistry.RegisterTag("amenity");
    tagCapacity=tagRegistry.RegisterTag("capacity");
  }

  std::string ChargingStationFeature::GetName() const
  {
    return NAME;
  }

  size_t ChargingStationFeature::GetValueAlignment() const
  {
    return alignof(ChargingStationFeatureValue);
  }

  size_t ChargingStationFeature::GetValueSize() const
  {
    return sizeof(ChargingStationFeatureValue);
  }

  FeatureValue* ChargingStationFeature::AllocateValue(void* buffer)
  {
    return new (buffer) ChargingStationFeatureValue();
  }

  void ChargingStationFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TagRegistry& /*tagRegistry*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const TagMap& tags,
                            FeatureValueBuffer& buffer) const
  {
    auto amenity = tags.find(tagAmenity);

    if (amenity != tags.end() && amenity->second=="charging_station") {
      auto *value = static_cast<ChargingStationFeatureValue *>(buffer.AllocateValue(feature.GetIndex()));
      uint8_t capacityValue=0;

      auto capacity = tags.find(tagCapacity);
      if (capacity!=tags.end()) {
        StringToNumber(capacity->second,
                       capacityValue);
      }
      value->SetCapacity(capacityValue);
    }
  }
}
