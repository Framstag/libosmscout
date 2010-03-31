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

#include <osmscout/Way.h>

#include <cassert>
#include <cmath>

namespace osmscout {

  bool Way::GetCenter(double& lat, double& lon) const
  {
    if (nodes.size()==0) {
      return false;
    }

    double minLat=nodes[0].lat;
    double minLon=nodes[0].lon;
    double maxLat=nodes[0].lat;
    double maxLon=nodes[0].lon;

    for (size_t i=1; i<nodes.size(); i++) {
      minLat=std::min(minLat,nodes[i].lat);
      minLon=std::min(minLon,nodes[i].lon);
      maxLat=std::max(maxLat,nodes[i].lat);
      maxLon=std::max(maxLon,nodes[i].lon);
    }

    lat=minLat+(maxLat-minLat)/2;
    lon=minLon+(maxLon-minLon)/2;

    return true;
  }

  bool Way::Read(FileReader& reader)
  {
    uint32_t nodeCount;
    uint32_t flags;


    reader.Read(id);
    reader.ReadNumber(type);
    reader.Read(flags);
    reader.ReadNumber(nodeCount);

    if (reader.HasError()) {
      return false;
    }

    this->flags=flags;
    nodes.resize(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      uint32_t latValue;
      uint32_t lonValue;

      reader.Read(nodes[i].id);
      reader.Read(latValue);
      reader.Read(lonValue);

      nodes[i].lat=latValue/conversionFactor-180.0;
      nodes[i].lon=lonValue/conversionFactor-90.0;
    }

    if (flags & hasName) {
      reader.Read(name);
    }

    if (flags & hasRef) {
      reader.Read(ref);
    }

    if (flags & hasLayer) {
      uint32_t layer;

      reader.ReadNumber(layer);

      this->layer=layer;
    }

    if (flags & hasTags) {
      uint32_t tagCount;

      reader.ReadNumber(tagCount);

      if (reader.HasError()) {
        return false;
      }

      tags.resize(tagCount);
      for (size_t i=0; i<tagCount; i++) {
        reader.ReadNumber(tags[i].key);
        reader.Read(tags[i].value);
      }
    }

    if (flags & hasRestrictions) {
      uint32_t restrictionCount;

      reader.ReadNumber(restrictionCount);

      if (reader.HasError()) {
        return false;
      }

      restrictions.resize(restrictionCount);

      for (size_t i=0; i<restrictionCount; i++) {
        uint32_t type;
        uint32_t memberCount;

        reader.ReadNumber(type);
        reader.ReadNumber(memberCount);

        if (reader.HasError()) {
          return false;
        }


        restrictions[i].type=(Way::RestrictionType)type;
        restrictions[i].members.resize(memberCount);

        for (size_t j=0; j<memberCount; j++) {
          reader.Read(restrictions[i].members[j]);
        }
      }
    }

    return !reader.HasError();
  }

  bool Way::Read(FileScanner& scanner)
  {
    uint32_t nodeCount;
    uint32_t flags;

    scanner.Read(id);
    scanner.ReadNumber(type);
    scanner.Read(flags);
    scanner.ReadNumber(nodeCount);

    if (scanner.HasError()) {
      return false;
    }

    this->flags=flags;
    nodes.resize(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      uint32_t latValue;
      uint32_t lonValue;

      scanner.Read(nodes[i].id);
      scanner.Read(latValue);
      scanner.Read(lonValue);

      nodes[i].lat=latValue/conversionFactor-180.0;
      nodes[i].lon=lonValue/conversionFactor-90.0;
    }

    if (flags & hasName) {
      scanner.Read(name);
    }

    if (flags & hasRef) {
      scanner.Read(ref);
    }

    if (flags & hasLayer) {
      scanner.Read(layer);
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

    if (flags & hasRestrictions) {
      uint32_t restrictionCount;

      scanner.ReadNumber(restrictionCount);
      if (scanner.HasError()) {
        return false;
      }

      restrictions.resize(restrictionCount);

      for (size_t i=0; i<restrictionCount; i++) {
        uint32_t type;
        uint32_t memberCount;

        scanner.ReadNumber(type);
        scanner.ReadNumber(memberCount);

        if (scanner.HasError()) {
          return false;
        }


        restrictions[i].type=(Way::RestrictionType)type;
        restrictions[i].members.resize(memberCount);

        for (size_t j=0; j<memberCount; j++) {
          scanner.Read(restrictions[i].members[j]);
        }
      }
    }

    return !scanner.HasError();
  }

  bool Way::Write(FileWriter& writer) const
  {
    writer.Write(id);
    writer.WriteNumber((unsigned long)type);
    writer.Write((uint32_t)flags);

    writer.WriteNumber((unsigned long)nodes.size());

    for (size_t i=0; i<nodes.size(); i++) {
      uint32_t latValue=(uint32_t)round((nodes[i].lat+180.0)*conversionFactor);
      uint32_t lonValue=(uint32_t)round((nodes[i].lon+90.0)*conversionFactor);

      writer.Write(nodes[i].id);
      writer.Write(latValue);
      writer.Write(lonValue);
    }

    if (flags & hasName) {
      writer.Write(name);
    }

    if (flags & hasRef) {
      writer.Write(ref);
    }

    if (flags & hasLayer) {
      writer.Write(layer);
    }

    if (flags & hasTags) {
      writer.WriteNumber((unsigned long)tags.size());

      for (size_t i=0; i<tags.size(); i++) {
        writer.WriteNumber((unsigned long)tags[i].key);
        writer.Write(tags[i].value);
      }
    }

    if (flags & hasRestrictions) {
      writer.WriteNumber((unsigned long)restrictions.size());

      for (size_t i=0; i<restrictions.size(); i++) {
        writer.WriteNumber((unsigned long)restrictions[i].type);

        writer.WriteNumber((unsigned long)restrictions[i].members.size());

        for (size_t j=0; j<restrictions[i].members.size(); j++) {
          writer.Write(restrictions[i].members[j]);
        }
      }
    }

    return !writer.HasError();
  }
}
