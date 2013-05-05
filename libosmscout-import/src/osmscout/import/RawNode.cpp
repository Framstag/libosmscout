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

#include <osmscout/import/RawNode.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  void RawNode::SetId(OSMId id)
  {
    this->id=id;
  }

  void RawNode::SetType(TypeId type)
  {
    this->type=type;
  }

  void RawNode::SetCoordinates(double lon, double lat)
  {
    this->lon=lon;
    this->lat=lat;
  }

  void RawNode::SetTags(const std::vector<Tag>& tags)
  {
    this->tags=tags;
  }

  bool RawNode::Read(FileScanner& scanner)
  {
    if (!scanner.ReadNumber(id)) {
      return false;
    }

    uint32_t tmpType;
    uint32_t tagCount;

    if (!scanner.ReadNumber(tmpType)) {
      return false;
    }

    type=(TypeId)tmpType;

    if (!scanner.ReadCoord(lat,lon)) {
      return false;
    }

    if (!scanner.ReadNumber(tagCount)) {
      return false;
    }

    tags.resize(tagCount);
    for (size_t i=0; i<tagCount; i++) {
      if (!scanner.ReadNumber(tags[i].key)) {
        return false;
      }

      if (!scanner.Read(tags[i].value)) {
        return false;
      }
    }

    return !scanner.HasError();
  }

  bool RawNode::Write(FileWriter& writer) const
  {
    uint32_t latValue=(uint32_t)round((lat+90.0)*conversionFactor);
    uint32_t lonValue=(uint32_t)round((lon+180.0)*conversionFactor);

    writer.WriteNumber(id);

    writer.WriteNumber(type);
    writer.Write(latValue);
    writer.Write(lonValue);

    writer.WriteNumber((uint32_t)tags.size());
    for (size_t i=0; i<tags.size(); i++) {
      writer.WriteNumber(tags[i].key);
      writer.Write(tags[i].value);
    }

    return !writer.HasError();
  }
}

