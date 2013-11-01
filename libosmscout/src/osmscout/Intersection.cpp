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

#include <osmscout/Intersection.h>

namespace osmscout {

  Intersection::Intersection()
  : nodeId(0)
  {
    // no code
  }

  Intersection::~Intersection()
  {
    // no code
  }

  bool Intersection::Read(FileScanner& scanner)
  {
    if (!scanner.ReadNumber(nodeId)) {
      return false;
    }

    uint32_t objectCount;

    if (!scanner.ReadNumber(objectCount)) {
      return false;
    }

    objects.resize(objectCount);

    Id previousFileOffset=0;

    for (size_t i=0; i<objectCount; i++) {
      uint8_t    type;
      FileOffset fileOffset;

      if (!scanner.Read(type)) {
        return false;
      }

      if (!scanner.ReadNumber(fileOffset)) {
        return false;
      }

      fileOffset+=previousFileOffset;

      objects[i].Set(fileOffset,(RefType)type);

      previousFileOffset=fileOffset;
    }

    return !scanner.HasError();
  }
}
