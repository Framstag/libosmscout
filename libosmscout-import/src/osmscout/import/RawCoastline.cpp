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

#include <algorithm>
#include <limits>

#include <osmscout/system/Math.h>

namespace osmscout {

  void RawCoastline::SetId(OSMId id)
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

  void RawCoastline::SetNodes(const std::vector<OSMId>& nodes)
  {
    this->nodes=nodes;
  }

  void RawCoastline::Read(FileScanner& scanner)
  {
    scanner.ReadNumber(id);

    scanner.Read(flags);

    uint32_t nodeCount;

    scanner.ReadNumber(nodeCount);

    nodes.resize(nodeCount);

    if (nodeCount>0) {
      OSMId minId;

      scanner.ReadNumber(minId);

      for (size_t i=0; i<nodeCount; i++) {
        OSMId id;

        scanner.ReadNumber(id);

        nodes[i]=minId+id;
      }
    }
  }

  void RawCoastline::Write(FileWriter& writer) const
  {
    writer.WriteNumber(id);

    writer.Write(flags);

    writer.WriteNumber((uint32_t)nodes.size());

    if (!nodes.empty()) {
      OSMId minId=std::numeric_limits<Id>::max();

      for (size_t i=0; i<nodes.size(); i++) {
        minId=std::min(minId,nodes[i]);
      }

      writer.WriteNumber(minId);
      for (size_t i=0; i<nodes.size(); i++) {
        writer.WriteNumber(nodes[i]-minId);
      }
    }
  }
}

