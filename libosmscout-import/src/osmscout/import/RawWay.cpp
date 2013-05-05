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

#include <osmscout/import/RawWay.h>

#include <limits>

#include <osmscout/system/Math.h>

namespace osmscout {

  void RawWay::SetId(OSMId id)
  {
    this->id=id;
  }

  void RawWay::SetType(TypeId type, bool area)
  {
    this->type=type;

    if (area) {
      this->flags|=isArea;
    }
    else {
      this->flags&=~isArea;
    }
  }

  void RawWay::SetTags(const std::vector<Tag>& tags)
  {
    this->tags=tags;
  }

  void RawWay::SetNodes(const std::vector<OSMId>& nodes)
  {
    this->nodes=nodes;
  }

  bool RawWay::Read(FileScanner& scanner)
  {
    if (!scanner.ReadNumber(id)) {
      return false;
    }

    if (!scanner.Read(flags)) {
      return false;
    }

    if ((flags & hasType)!=0) {
      uint32_t tmpType;

      if (!scanner.ReadNumber(tmpType)) {
        return false;
      }

      type=(TypeId)tmpType;
    }
    else {
      type=typeIgnore;
    }

    uint32_t nodeCount;

    if ((flags & hasTags)!=0) {
      uint32_t tagCount;

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
    }
    else {
      tags.clear();
    }

    if (!scanner.ReadNumber(nodeCount)) {
      return false;
    }

    nodes.resize(nodeCount);

    if (nodeCount>0) {
      OSMId minId;

      if (!scanner.ReadNumber(minId)) {
        return false;
      }

      for (size_t i=0; i<nodeCount; i++) {
        OSMId id;

        if (!scanner.ReadNumber(id)) {
          return false;
        }

        nodes[i]=minId+id;
      }
    }

    return !scanner.HasError();
  }

  bool RawWay::Write(FileWriter& writer) const
  {
    writer.WriteNumber(id);

    if (type!=typeIgnore) {
      flags|=hasType;
    }
    else {
      flags&=~hasType;
    }

    if (!tags.empty()) {
      flags|=hasTags;
    }
    else {
      flags&=~hasTags;
    }

    writer.Write(flags);

    if (type!=typeIgnore) {
      writer.WriteNumber(type);
    }

    if (!tags.empty()) {
      writer.WriteNumber((uint32_t)tags.size());
      for (size_t i=0; i<tags.size(); i++) {
        writer.WriteNumber(tags[i].key);
        writer.Write(tags[i].value);
      }
    }

    writer.WriteNumber((uint32_t)nodes.size());

    if (!nodes.empty()) {
      OSMId minId=std::numeric_limits<OSMId>::max();

      for (size_t i=0; i<nodes.size(); i++) {
        minId=std::min(minId,nodes[i]);
      }

      writer.WriteNumber(minId);
      for (size_t i=0; i<nodes.size(); i++) {
        writer.WriteNumber(nodes[i]-minId);
      }
    }

    return !writer.HasError();
  }
}

