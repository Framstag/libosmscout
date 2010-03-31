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

  bool Relation::GetCenter(double& lat, double& lon) const
  {
    if (roles.size()==0) {
      return false;
    }

    double minLat=0.0;
    double minLon=0.0;
    double maxLat=0.0;
    double maxLon=0.0;

    bool start=true;

    for (size_t i=0; i<roles.size(); i++) {
      for (size_t j=0; j<roles[i].nodes.size(); j++) {
        if (start) {
          minLat=roles[i].nodes[j].lat;
          minLon=roles[i].nodes[j].lon;
          maxLat=roles[i].nodes[j].lat;
          maxLon=roles[i].nodes[j].lon;

          start=false;
        }

        minLat=std::min(minLat,roles[i].nodes[j].lat);
        minLon=std::min(minLon,roles[i].nodes[j].lon);
        maxLat=std::max(maxLat,roles[i].nodes[j].lat);
        maxLon=std::max(maxLon,roles[i].nodes[j].lon);
      }
    }

    if (start) {
      return false;
    }

    lat=minLat+(maxLat-minLat)/2;
    lon=minLon+(maxLon-minLon)/2;

    return true;
  }

  bool Relation::Read(FileScanner& scanner)
  {
    uint32_t tagCount;
    uint32_t roleCount;

    scanner.Read(id);
    scanner.ReadNumber(type);
    scanner.Read(relType);

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
      uint32_t nodesCount;
      uint32_t typeValue;

      scanner.Read(typeValue);
      roles[i].type=(TypeId)typeValue;
      scanner.Read(roles[i].role);
      scanner.ReadNumber(nodesCount);

      roles[i].nodes.resize(nodesCount);
      for (size_t j=0; j<nodesCount; j++) {
        uint32_t latValue;
        uint32_t lonValue;

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
    writer.WriteNumber((unsigned long)type);
    writer.Write(relType);

    writer.WriteNumber((unsigned long)tags.size());
    for (size_t i=0; i<tags.size(); i++) {
      writer.WriteNumber((unsigned long)tags[i].key);
      writer.Write(tags[i].value);
    }

    writer.WriteNumber((unsigned long)roles.size());
    for (size_t i=0; i<roles.size(); i++) {
      writer.Write(roles[i].type);
      writer.Write(roles[i].role);
      writer.WriteNumber((unsigned long)roles[i].nodes.size());

      for (size_t j=0; j<roles[i].nodes.size(); j++) {
        uint32_t latValue=(uint32_t)round((roles[i].nodes[j].lat+180.0)*conversionFactor);
        uint32_t lonValue=(uint32_t)round((roles[i].nodes[j].lon+90.0)*conversionFactor);

        writer.Write(roles[i].nodes[j].id);
        writer.Write(latValue);
        writer.Write(lonValue);
      }
    }

    return !writer.HasError();
  }
}

