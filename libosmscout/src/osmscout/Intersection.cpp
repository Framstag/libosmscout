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

#include <osmscout/util/Logger.h>

namespace osmscout {

  Intersection::Intersection()
  : nodeId(0)
  {
    // no code
  }

  bool Intersection::Read(FileScanner& scanner)
  {
    try {
      nodeId=scanner.ReadUInt64Number();

      uint32_t objectCount=scanner.ReadUInt32Number();

      objects.resize(objectCount);

      ObjectFileRefStreamReader objectFileRefReader(scanner);

      for (size_t i=0; i<objectCount; i++) {
        objectFileRefReader.Read(objects[i]);
      }
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();

      return false;
    }

    return true;
  }

  bool Intersection::Read(const TypeConfig& /*typeConfig*/,
                          FileScanner& scanner)
  {
    return Read(scanner);
  }
}
