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

#include <osmscout/private/Math.h>

namespace osmscout {

  void Node::SetId(Id id)
  {
    this->id=id;
  }

  void Node::SetType(TypeId type)
  {
    this->type=type;
  }

  void Node::SetCoordinates(double lon, double lat)
  {
    this->lon=lon;
    this->lat=lat;
  }

  void Node::SetTags(const std::vector<Tag>& tags)
  {
    this->tags=tags;
  }

  bool Node::Read(FileScanner& scanner)
  {
    uint32_t tmpType;
    uint32_t tagCount;
    uint32_t latValue;
    uint32_t lonValue;

    scanner.Read(id);
    scanner.ReadNumber(tmpType);
    scanner.Read(latValue);
    scanner.Read(lonValue);

    type=(TypeId)tmpType;
    lat=latValue/conversionFactor-90.0;
    lon=lonValue/conversionFactor-180.0;

    scanner.ReadNumber(tagCount);

    if (scanner.HasError()) {
      return false;
    }

    tags.resize(tagCount);
    for (size_t i=0; i<tagCount; i++) {
      scanner.ReadNumber(tags[i].key);
      scanner.Read(tags[i].value);
    }

    return !scanner.HasError();
  }

  bool Node::Write(FileWriter& writer) const
  {
    uint32_t latValue=(uint32_t)floor((lat+90.0)*conversionFactor+0.5);
    uint32_t lonValue=(uint32_t)floor((lon+180.0)*conversionFactor+0.5);

    writer.Write(id);
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

