/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <osmscout/import/RawCoastline.h>

#include <limits>

#include <osmscout/private/Math.h>

namespace osmscout {

  void RawCoastline::SetId(Id id)
  {
    this->id=id;
  }

  void RawCoastline::SetType(bool area)
  {
    if (area) {
      this->flags|=isArea;
    }
    else {
      this->flags&=~isArea;
    }
  }

  void RawCoastline::SetNodes(const std::vector<Id>& nodes)
  {
    this->nodes=nodes;
  }

  bool RawCoastline::Read(FileScanner& scanner)
  {
    if (!scanner.ReadNumber(id)) {
      return false;
    }

    if (!scanner.Read(flags)) {
      return false;
    }

    uint32_t nodeCount;

    if (!scanner.ReadNumber(nodeCount)) {
      return false;
    }

    nodes.resize(nodeCount);

    if (nodeCount>0) {
      Id minId;

      if (!scanner.ReadNumber(minId)) {
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

  bool RawCoastline::Write(FileWriter& writer) const
  {
    writer.WriteNumber(id);

    writer.Write(flags);

    writer.WriteNumber((uint32_t)nodes.size());

    if (!nodes.empty()) {
      Id minId=std::numeric_limits<Id>::max();

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

