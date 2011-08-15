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
#include <limits>

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

  void Relation::GetBoundingBox(double& minLon,
                                double& maxLon,
                                double& minLat,
                                double& maxLat) const
  {
    assert(roles.size()>0);
    assert(roles[0].nodes.size()>0);

    minLon=roles[0].nodes[0].lon;
    maxLon=roles[0].nodes[0].lon;
    minLat=roles[0].nodes[0].lat;
    maxLat=roles[0].nodes[0].lat;

    for (std::vector<Relation::Role>::const_iterator role=roles.begin();
         role!=roles.end();
         ++role) {
      for (size_t i=0; i<role->nodes.size(); i++) {
        minLon=std::min(minLon,role->nodes[i].lon);
        maxLon=std::max(maxLon,role->nodes[i].lon);
        minLat=std::min(minLat,role->nodes[i].lat);
        maxLat=std::max(maxLat,role->nodes[i].lat);
      }
    }
  }

  void Relation::SetId(Id id)
  {
    this->id=id;
  }

  void Relation::SetType(TypeId type)
  {
    this->type=type;
  }

  void Relation::SetRelType(const std::string& relType)
  {
    this->relType=relType;
  }

  bool Relation::Read(FileScanner& scanner)
  {
    uint32_t tagCount;
    uint32_t roleCount;

    scanner.Read(id);
    scanner.ReadNumber(type);
    scanner.Read(relType);
    scanner.Read(flags);

    if (flags & hasTags) {
      scanner.ReadNumber(tagCount);
      if (scanner.HasError()) {
        return false;
      }

      tags.resize(tagCount);
      for (size_t i=0; i<tagCount; i++) {
        scanner.ReadNumber(tags[i].key);
        scanner.Read(tags[i].value);
      }
    }

    scanner.ReadNumber(roleCount);
    if (scanner.HasError()) {
      return false;
    }

    roles.resize(roleCount);
    for (size_t i=0; i<roleCount; i++) {
      uint32_t nodesCount;

      if (!roles[i].attributes.Read(scanner)) {
        return false;
      }

      scanner.Read(roles[i].role);

      scanner.ReadNumber(nodesCount);

      if (nodesCount>0) {
        roles[i].nodes.resize(nodesCount);

        Id       minId;
        uint32_t minLat;
        uint32_t minLon;

        scanner.Read(minId);
        scanner.Read(minLat);
        scanner.Read(minLon);

        for (size_t j=0; j<nodesCount; j++) {
          Id       id;
          uint32_t latValue;
          uint32_t lonValue;

          scanner.ReadNumber(id);
          scanner.ReadNumber(latValue);
          scanner.ReadNumber(lonValue);

          roles[i].nodes[j].id=minId+id;
          roles[i].nodes[j].lat=(minLat+latValue)/conversionFactor-90.0;
          roles[i].nodes[j].lon=(minLon+lonValue)/conversionFactor-180.0;
        }
      }
    }

    return !scanner.HasError();
  }

  bool Relation::Write(FileWriter& writer) const
  {
    writer.Write(id);
    writer.WriteNumber(type);
    writer.Write(relType);
    writer.Write(flags);

    if (flags & hasTags) {
      writer.WriteNumber((uint32_t)tags.size());
      for (size_t i=0; i<tags.size(); i++) {
        writer.WriteNumber(tags[i].key);
        writer.Write(tags[i].value);
      }
    }

    writer.WriteNumber((uint32_t)roles.size());
    for (size_t i=0; i<roles.size(); i++) {
      if (!roles[i].attributes.Write(writer)) {
        return false;
      }

      writer.Write(roles[i].role);

      writer.WriteNumber((uint32_t)roles[i].nodes.size());

      if (roles[i].nodes.size()>0) {
        Id       minId=std::numeric_limits<Id>::max();
        uint32_t minLat=std::numeric_limits<uint32_t>::max();
        uint32_t minLon=std::numeric_limits<uint32_t>::max();

        for (size_t j=0; j<roles[i].nodes.size(); j++) {
          minId=std::min(minId,roles[i].nodes[j].GetId());
          minLat=std::min(minLat,(uint32_t)floor((roles[i].nodes[j].lat+90.0)*conversionFactor+0.5));
          minLon=std::min(minLon,(uint32_t)floor((roles[i].nodes[j].lon+180.0)*conversionFactor+0.5));
        }

        writer.Write(minId);
        writer.Write(minLat);
        writer.Write(minLon);

        for (size_t j=0; j<roles[i].nodes.size(); j++) {
          uint32_t latValue=(uint32_t)floor((roles[i].nodes[j].lat+90.0)*conversionFactor+0.5);
          uint32_t lonValue=(uint32_t)floor((roles[i].nodes[j].lon+180.0)*conversionFactor+0.5);

          writer.WriteNumber(roles[i].nodes[j].id-minId);
          writer.WriteNumber(latValue-minLat);
          writer.WriteNumber(lonValue-minLon);
        }
      }
    }

    return !writer.HasError();
  }
}

