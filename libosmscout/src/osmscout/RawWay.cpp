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

#include <osmscout/RawWay.h>

#include <cmath>

namespace osmscout {

  bool RawWay::Read(FileScanner& scanner)
  {
    scanner.Read(id);
    if (scanner.HasError()) {
      return false;
    }

    scanner.ReadNumber(type);
    scanner.Read(isArea);

    uint32_t tagCount;
    uint32_t nodeCount;

    scanner.ReadNumber(tagCount);
    if (scanner.HasError()) {
      return false;
    }

    tags.resize(tagCount);
    for (size_t i=0; i<tagCount; i++) {
      scanner.ReadNumber(tags[i].key);
      scanner.Read(tags[i].value);
    }

    scanner.ReadNumber(nodeCount);
    if (scanner.HasError()) {
      return false;
    }

    nodes.resize(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      scanner.Read(nodes[i]);
    }

    return scanner.HasError();
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
    for (size_t i=0; i<nodes.size(); i++) {
      writer.Write(nodes[i]);
    }

    return !writer.HasError();
  }
}

