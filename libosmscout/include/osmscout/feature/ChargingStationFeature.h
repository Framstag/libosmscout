#ifndef OSMSCOUT_FEATURE_CHARGING_STATION_FEATURE_H
#define OSMSCOUT_FEATURE_CHARGING_STATION_FEATURE_H

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

#include <unordered_map>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeature.h>

#include <osmscout/FeatureReader.h>

namespace osmscout {

  class OSMSCOUT_API ChargingStationFeatureValue : public FeatureValue
  {
  public:
    enum class SocketType : uint8_t
    {
      Unknown           = 0,
      Type1             = 1,
      Typ1Combo         = 2,
      Type2             = 3,
      Type2Cable        = 4,
      Type2Combo        = 5,
      Chademo           = 6,
      TeslaSupercharger = 7,
      TeslaDestination  = 8
    };

    struct Socket
    {
      SocketType  type;
      uint8_t     capacity;
      std::string output;

      std::strong_ordering operator<=>(const Socket& other) const = default;
    };

  private:
    std::vector<Socket>  sockets;

  public:
    void AddSocket(const Socket& socket) {
      sockets.push_back(socket);
    }

    bool HasSockets() const
    {
      return !sockets.empty();
    }

    const std::vector<Socket>& GetSockets() const
    {
      return sockets;
    }

  public:
    ChargingStationFeatureValue() = default;
    ChargingStationFeatureValue(const ChargingStationFeatureValue& other) = default;

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    ChargingStationFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API ChargingStationFeature : public Feature
  {
  public:
    /** Name of this feature */
    static const char* const NAME;

  private:
    struct SocketTags
    {
      TagId socketType;
      TagId output;
    };

    TagId   tagAmenity;
    TagId   tagCapacity;

    std::unordered_map<ChargingStationFeatureValue::SocketType,SocketTags> socketTagsMap;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueAlignment() const override;
    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  using ChargingStationFeatureValueReader = FeatureValueReader<ChargingStationFeature, ChargingStationFeatureValue>;

  extern OSMSCOUT_API const char* EnumToString(const ChargingStationFeatureValue::SocketType& value);
}

#endif
