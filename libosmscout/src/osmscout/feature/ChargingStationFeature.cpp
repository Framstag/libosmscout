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
    sockets.resize(scanner.ReadUInt64Number());

    for (auto& socket : sockets) {
      socket.type=static_cast<ChargingStationFeatureValue::SocketType>(scanner.ReadUInt8());
      socket.capacity=scanner.ReadUInt8();
      socket.output=scanner.ReadString();
    }
  }

  void ChargingStationFeatureValue::Write(FileWriter& writer)
  {
    writer.WriteNumber(static_cast<uint64_t>(sockets.size()));

    for (const auto& socket : sockets) {
      writer.Write(static_cast<uint8_t>(socket.type));
      writer.Write(socket.capacity);
      writer.Write(socket.output);
    }
  }

  ChargingStationFeatureValue& ChargingStationFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const ChargingStationFeatureValue&>(other);

     this->sockets=otherValue.sockets;
    }

    return *this;
  }

  bool ChargingStationFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const ChargingStationFeatureValue&>(other);

    return this->sockets==otherValue.sockets;
  }

  const char* const ChargingStationFeature::NAME = "ChargingStation";

  void ChargingStationFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAmenity=tagRegistry.RegisterTag("amenity");
    tagCapacity=tagRegistry.RegisterTag("capacity");

    socketTagsMap = {
            {
              ChargingStationFeatureValue::SocketType::Type1,
                    {tagRegistry.RegisterTag("socket:type1"),
                     tagRegistry.RegisterTag("socket:type1:output")}
              },
            {
              ChargingStationFeatureValue::SocketType::Typ1Combo,
                    {tagRegistry.RegisterTag("socket:type1_combo"),
                     tagRegistry.RegisterTag("socket:type1_combo:output")}
              },
            {
              ChargingStationFeatureValue::SocketType::Type2,
                    {tagRegistry.RegisterTag("socket:type2"),
                     tagRegistry.RegisterTag("socket:type2:output")}
              },
            {
              ChargingStationFeatureValue::SocketType::Type2Cable,
                    {tagRegistry.RegisterTag("socket:type2_cable"),
                     tagRegistry.RegisterTag("socket:type2_cable:output")}
              },
            {
              ChargingStationFeatureValue::SocketType::Type2Combo,
                    {tagRegistry.RegisterTag("socket:type1_combo"),
                     tagRegistry.RegisterTag("socket:type1_combo:output")}
              },
            {
              ChargingStationFeatureValue::SocketType::Chademo,
                    {tagRegistry.RegisterTag("socket:chademo"),
                     tagRegistry.RegisterTag("socket:chademo:output")}
              },
            {
              ChargingStationFeatureValue::SocketType::TeslaSupercharger,
                    {tagRegistry.RegisterTag("socket:tesla_supercharger"),
                     tagRegistry.RegisterTag("socket:tesla_supercharger:output")}
              },
            {
              ChargingStationFeatureValue::SocketType::TeslaDestination,
                    {tagRegistry.RegisterTag("socket:tesla_detination"),
                     tagRegistry.RegisterTag("socket:tesla_detination:output")}
            }
    };
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

  void ChargingStationFeature::Parse(TagErrorReporter& errorReporter,
                            const TagRegistry& /*tagRegistry*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& object,
                            const TagMap& tags,
                            FeatureValueBuffer& buffer) const
  {
    if (auto amenity = tags.find(tagAmenity);
        amenity == tags.end() || amenity->second!="charging_station") {
      return;
    }

    auto *value = static_cast<ChargingStationFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));
    uint8_t capacityValue=0;

    // capacity

    if (auto capacity = tags.find(tagCapacity); capacity!=tags.end()) {
      StringToNumber(capacity->second,
                     capacityValue);
    }

    for (const auto& [socketType, socketTags] : socketTagsMap) {
      if (auto socketTypeTag = tags.find(socketTags.socketType); socketTypeTag != tags.end()) {
        ChargingStationFeatureValue::Socket socket;
        if (StringToNumber(socketTypeTag->second,
                           socket.capacity)) {
          socket.type=socketType;

          if (auto outputTag = tags.find(socketTags.output); outputTag != tags.end()) {
            socket.output=outputTag->second;
          }

          value->AddSocket(socket);
        }
      }
    }

    if (capacityValue > 0 && !value->HasSockets()) {
      errorReporter.ReportTag(object,tags,"Only capacity given for charging_station, but no socket:type!");

      ChargingStationFeatureValue::Socket socket;

      socket.type=ChargingStationFeatureValue::SocketType::Unknown;
      socket.capacity=capacityValue;
      value->AddSocket(socket);
    }
  }

  const char* EnumToString(const ChargingStationFeatureValue::SocketType& value)
  {
    switch (value) {
      case ChargingStationFeatureValue::SocketType::Unknown:
        return "Unknown";
      case ChargingStationFeatureValue::SocketType::Type1:
        return "Type 1";
      case ChargingStationFeatureValue::SocketType::Typ1Combo:
        return "Type 1 Combo";
      case ChargingStationFeatureValue::SocketType::Type2:
        return "Type 2";
      case ChargingStationFeatureValue::SocketType::Type2Cable:
        return "Type 2 Cable";
      case ChargingStationFeatureValue::SocketType::Type2Combo:
        return "Type 2 Combo";
      case ChargingStationFeatureValue::SocketType::Chademo:
        return "Chademo";
      case ChargingStationFeatureValue::SocketType::TeslaSupercharger:
        return "Tesla Supercharger";
      case ChargingStationFeatureValue::SocketType::TeslaDestination:
        return "Tesla Destination";
    }

    return "???";
  }
}
