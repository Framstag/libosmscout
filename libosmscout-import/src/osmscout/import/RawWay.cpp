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

#include <osmscout/private/Math.h>

namespace osmscout {

  void RawWay::SetId(Id id)
  {
    this->id=id;
  }

  void RawWay::SetType(TypeId type, bool isArea)
  {
    this->type=type;
    this->isArea=isArea;
  }

  void RawWay::SetTags(const std::vector<Tag>& tags)
  {
    this->tags=tags;
  }

  void RawWay::SetNodes(const std::vector<Id>& nodes)
  {
    this->nodes=nodes;
  }

  bool RawWay::Read(FileScanner& scanner)
  {
    if (!scanner.Read(id)) {
      return false;
    }

    uint32_t tmpType;

    if (!scanner.ReadNumber(tmpType)) {
      return false;
    }

    type=(TypeId)tmpType;

    if (!scanner.Read(isArea)) {
      return false;
    }

    uint32_t tagCount;
    uint32_t nodeCount;

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

    if (!scanner.ReadNumber(nodeCount)) {
      return false;
    }

    nodes.resize(nodeCount);

    if (nodeCount>0) {
      Id minId;

      if (!scanner.Read(minId)) {
        return false;
      }

      for (size_t i=0; i<nodeCount; i++) {
        Id id;

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
    writer.Write(id);
    writer.WriteNumber(type);
    writer.Write(isArea);

    writer.WriteNumber((uint32_t)tags.size());
    for (size_t i=0; i<tags.size(); i++) {
      writer.WriteNumber(tags[i].key);
      writer.Write(tags[i].value);
    }

    writer.WriteNumber((uint32_t)nodes.size());

    if (!nodes.empty()) {
      Id minId=std::numeric_limits<Id>::max();

      for (size_t i=0; i<nodes.size(); i++) {
        minId=std::min(minId,nodes[i]);
      }

      writer.Write(minId);
      for (size_t i=0; i<nodes.size(); i++) {
        writer.WriteNumber(nodes[i]-minId);
      }
    }

    return !writer.HasError();
  }
}

