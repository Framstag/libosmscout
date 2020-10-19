/*
  This source is part of the libosmscout library
  Copyright (C) 2020  Tim Teulings

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

#include <osmscout/PublicTransport.h>

namespace osmscout {


  void PTRoute::Stop::SetType(PTRoute::StopType stopType)
  {
    Stop::type=stopType;
  }

  void PTRoute::Stop::SetStop(const ObjectFileRef& stop)
  {
    Stop::stop=stop;
  }

  void PTRoute::Platform::SetType(PTRoute::PlatformType platformType)
  {
    Platform::type=platformType;
  }

  void PTRoute::Platform::SetPlatform(const ObjectFileRef& platform)
  {
    Platform::platform=platform;
  }

  void PTRoute::Variant::SetName(const std::string& name)
  {
    PTRoute::Variant::name=name;
  }

  void PTRoute::Variant::SetRef(const std::string& ref)
  {
    PTRoute::Variant::ref=ref;
  }

  void PTRoute::Variant::SetOperator(const std::string& operatorName)
  {
    PTRoute::Variant::operatorName=operatorName;
  }

  void PTRoute::Variant::SetNetwork(const std::string& network)
  {
    PTRoute::Variant::network=network;
  }

  void PTRoute::Variant::SetFrom(const std::string& from)
  {
    Variant::from=from;
  }

  void PTRoute::Variant::SetTo(const std::string& to)
  {
    Variant::to=to;
  }

  void PTRoute::Variant::SetColor(const Color& color)
  {
    Variant::color=color;
  }

  void PTRoute::SetType(const TypeInfoRef& type)
  {
    PTRoute::type=type;
  }

  void PTRoute::SetName(const std::string& name)
  {
    PTRoute::name=name;
  }

  void PTRoute::SetRef(const std::string& ref)
  {
    PTRoute::ref=ref;
  }

  void PTRoute::SetOperator(const std::string& operatorName)
  {
    PTRoute::operatorName=operatorName;
  }

  void PTRoute::SetNetwork(const std::string& network)
  {
    PTRoute::network=network;
  }

  void PTRoute::SetColor(const Color& color)
  {
    PTRoute::color=color;
  }

  void PTRoute::Read(const TypeConfig& typeConfig,
                     FileScanner& scanner)
  {
    fileOffset=scanner.GetPos();

    uint16_t typeIndex=scanner.ReadUInt16();

    type=typeConfig.GetTypeInfo(typeIndex);

    name=scanner.ReadString();
    ref=scanner.ReadString();
    operatorName=scanner.ReadString();
    network=scanner.ReadString();
    color=scanner.ReadColor();

    uint32_t variantCount=scanner.ReadUInt32Number();

    variants.resize(variantCount);

    for (auto& variant : variants) {
      variant.name=scanner.ReadString();
      variant.ref=scanner.ReadString();
      variant.operatorName=scanner.ReadString();
      variant.network=scanner.ReadString();
      variant.from=scanner.ReadString();
      variant.to=scanner.ReadString();
      variant.color=scanner.ReadColor();

      uint32_t stopCount=scanner.ReadUInt32Number();
      uint32_t platformCount=scanner.ReadUInt32Number();

      variant.stops.resize(stopCount);
      variant.platforms.resize(platformCount);

      for (auto& stop : variant.stops) {
        uint8_t type=scanner.ReadUInt8();

        stop.type=(StopType)type;
        stop.stop=scanner.ReadObjectFileRef();
      }

      for (auto& platform : variant.platforms) {
        uint8_t type=scanner.ReadUInt8();

        platform.type=(PlatformType)type;
        platform.platform=scanner.ReadObjectFileRef();
      }
    }

    nextFileOffset=scanner.GetPos();
  }

  void PTRoute::Write(const TypeConfig& /*typeConfig*/,
                      FileWriter& writer) const
  {
    // TODO: Find a better way to get and store a stable id for the type
    writer.Write(static_cast<uint16_t>(type->GetIndex()));
    writer.Write(name);
    writer.Write(ref);
    writer.Write(operatorName);
    writer.Write(network);
    writer.Write(color);

    writer.WriteNumber(static_cast<uint32_t>(variants.size()));

    for (const auto& variant : variants) {
      writer.Write(variant.name);
      writer.Write(variant.ref);
      writer.Write(variant.operatorName);
      writer.Write(variant.network);
      writer.Write(variant.from);
      writer.Write(variant.to);
      writer.Write(variant.color);

      writer.WriteNumber(static_cast<uint32_t>(variant.stops.size()));
      writer.WriteNumber(static_cast<uint32_t>(variant.platforms.size()));

      for (const auto& stop : variant.stops) {
        writer.Write((uint8_t)stop.type);
        writer.Write(stop.stop);
      }

      for (const auto& platform : variant.platforms) {
        writer.Write((uint8_t)platform.type);
        writer.Write(platform.platform);
      }
    }
  }
}
