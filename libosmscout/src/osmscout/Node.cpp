/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/Node.h>

namespace osmscout {

  void NodeAttributes::SetFeatures(const TypeConfig& typeConfig,
                                   const TypeInfo& type,
                                   FeatureValue* featureValues[])
  {
    for (size_t i=0; i<type.GetFeatureCount(); i++) {
      if (dynamic_cast<NameFeatureValue*>(featureValues[i])!=NULL) {
        NameFeatureValue* value=dynamic_cast<NameFeatureValue*>(featureValues[i]);

        name=value->GetName();
        nameAlt=value->GetNameAlt();
      }
      else if (dynamic_cast<AddressFeatureValue*>(featureValues[i])!=NULL) {
        AddressFeatureValue* value=dynamic_cast<AddressFeatureValue*>(featureValues[i]);

        address=value->GetAddress();

        tags.push_back(Tag(typeConfig.tagAddrStreet,
                           value->GetLocation()));
      }
    }
  }

  void NodeAttributes::GetFlags(uint8_t& flags) const
  {
    flags=0;

    if (!name.empty()) {
      flags|=hasName;
    }

    if (!nameAlt.empty()) {
      flags|=hasNameAlt;
    }

    if (!address.empty()) {
      flags|=hasAddress;
    }

    if (!tags.empty()) {
      flags|=hasTags;
    }
  }

  bool NodeAttributes::Read(FileScanner& scanner)
  {
    scanner.Read(flags);

    if (scanner.HasError()) {
      return false;
    }

    if (flags & hasName) {
      scanner.Read(name);
    }

    if (flags & hasNameAlt) {
      scanner.Read(nameAlt);
    }

    if (flags & hasAddress) {
      scanner.Read(address);
    }

    if (flags & hasTags) {
      uint32_t tagCount;

      scanner.ReadNumber(tagCount);
      if (scanner.HasError()) {
        return false;
      }

      tags.resize(tagCount);
      for (size_t i=0; i<tagCount; i++) {
        scanner.ReadNumber(tags[i].key);
        scanner.Read(tags[i].value);
      }
    }

    return !scanner.HasError();
  }

  bool NodeAttributes::Write(FileWriter& writer) const
  {
    uint8_t flags;

    GetFlags(flags);

    writer.Write(flags);

    if (flags & hasName) {
      writer.Write(name);
    }

    if (flags & hasNameAlt) {
      writer.Write(nameAlt);
    }

    if (flags & hasAddress) {
      writer.Write(address);
    }

    if (flags & hasTags) {
      writer.WriteNumber((uint32_t)tags.size());

      for (size_t i=0; i<tags.size(); i++) {
        writer.WriteNumber(tags[i].key);
        writer.Write(tags[i].value);
      }
    }

    return !writer.HasError();
  }

  bool NodeAttributes::operator==(const NodeAttributes& other) const
  {
    if (name!=other.name ||
        nameAlt!=other.nameAlt ||
        address!=other.address) {
      return false;
    }

    if (tags.empty() && other.tags.empty()) {
      return true;
    }

    if (tags.size()!=other.tags.size()) {
      return false;
    }

    for (size_t t=0; t<tags.size(); t++) {
      if (tags[t].key!=other.tags[t].key) {
        return false;
      }

      if (tags[t].value!=other.tags[t].value) {
        return false;
      }
    }

    return true;
  }

  bool NodeAttributes::operator!=(const NodeAttributes& other) const
  {
    return !this->operator==(other);
  }


  void Node::SetType(TypeId type)
  {
    this->type=type;
  }

  void Node::SetCoords(const GeoCoord& coords)
  {
    this->coords=coords;
  }

  void Node::SetFeatures(const TypeConfig& typeConfig,
                         const TypeInfo& type,
                         FeatureValue* featureValues[])
  {
    attributes.SetFeatures(typeConfig,
                           type,
                           featureValues);
  }

  bool Node::Read(FileScanner& scanner)
  {
    uint32_t tmpType;

    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    scanner.ReadNumber(tmpType);
    type=(TypeId)tmpType;

    scanner.ReadCoord(coords);

    if (!attributes.Read(scanner)) {
      return false;
    }

    return !scanner.HasError();
  }

  bool Node::Write(FileWriter& writer) const
  {
    writer.WriteNumber(type);
    writer.WriteCoord(coords);

    if (!attributes.Write(writer)) {
      return false;
    }

    return !writer.HasError();
  }
}

