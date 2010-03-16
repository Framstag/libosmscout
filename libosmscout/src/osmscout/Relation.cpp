/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/Relation.h>

#include <cmath>

namespace osmscout {

  static double conversionFactor=10000000.0;

  bool Relation::Read(FileScanner& scanner)
  {
    unsigned long tagCount;
    unsigned long roleCount;

    scanner.Read(id);
    scanner.ReadNumber(type);

    scanner.ReadNumber(tagCount);

    if (scanner.HasError()) {
      return false;
    }

    tags.resize(tagCount);
    for (size_t i=0; i<tagCount; i++) {
      scanner.ReadNumber(tags[i].key);
      scanner.Read(tags[i].value);
    }

    scanner.ReadNumber(roleCount);

    if (scanner.HasError()) {
      return false;
    }

    roles.resize(roleCount);
    for (size_t i=0; i<roleCount; i++) {
      unsigned long nodesCount;

      scanner.Read(roles[i].role);
      scanner.ReadNumber(nodesCount);

      for (size_t j=0; j<nodesCount; j++) {
        unsigned long latValue;
        unsigned long lonValue;

        scanner.Read(roles[i].nodes[j].id);
        scanner.Read(latValue);
        scanner.Read(lonValue);

        roles[i].nodes[j].lat=latValue/conversionFactor-180.0;
        roles[i].nodes[j].lon=lonValue/conversionFactor-90.0;
      }
    }

    return !scanner.HasError();
  }

  bool Relation::Write(FileWriter& writer) const
  {
    writer.Write(id);
    writer.WriteNumber(type);

    writer.WriteNumber(tags.size());
    for (size_t i=0; i<tags.size(); i++) {
      writer.WriteNumber(tags[i].key);
      writer.Write(tags[i].value);
    }

    writer.WriteNumber(roles.size());
    for (size_t i=0; i<roles.size(); i++) {
      writer.Write(roles[i].role);
      writer.WriteNumber(roles[i].nodes.size());

      for (size_t j=0; j<roles[i].nodes.size(); j++) {
        unsigned long latValue=(unsigned long)round((roles[i].nodes[j].lat+180.0)*conversionFactor);
        unsigned long lonValue=(unsigned long)round((roles[i].nodes[j].lon+90.0)*conversionFactor);

        writer.Write(roles[i].nodes[j].id);
        writer.Write(latValue);
        writer.Write(lonValue);
      }
    }

    return !writer.HasError();
  }
}

