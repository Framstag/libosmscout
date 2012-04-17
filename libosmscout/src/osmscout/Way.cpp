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

#include <osmscout/Way.h>

#include <cassert>
#include <limits>

#include <osmscout/private/Math.h>

namespace osmscout {

  bool Way::GetCenter(double& lat, double& lon) const
  {
    if (nodes.empty()) {
      return false;
    }

    double minLat=nodes[0].GetLat();
    double minLon=nodes[0].GetLon();
    double maxLat=nodes[0].GetLat();
    double maxLon=nodes[0].GetLon();

    for (size_t i=1; i<nodes.size(); i++) {
      minLat=std::min(minLat,nodes[i].GetLat());
      minLon=std::min(minLon,nodes[i].GetLon());
      maxLat=std::max(maxLat,nodes[i].GetLat());
      maxLon=std::max(maxLon,nodes[i].GetLon());
    }

    lat=minLat+(maxLat-minLat)/2;
    lon=minLon+(maxLon-minLon)/2;

    return true;
  }

  void Way::SetId(Id id)
  {
    this->id=id;
  }

  void Way::SetType(TypeId type)
  {
    attributes.type=type;
  }

  bool Way::SetTags(Progress& progress,
                    const TypeConfig& typeConfig,
                    bool isArea,
                    std::vector<Tag>& tags,
                    bool& reverseNodes)
  {
    return attributes.SetTags(progress,
                              typeConfig,
                              id,
                              isArea,
                              tags,
                              reverseNodes);
  }

  void Way::SetStartIsJoint(bool isJoint)
  {
    if (isJoint) {
      attributes.flags|=SegmentAttributes::startIsJoint;
    }
  }

  void Way::SetEndIsJoint(bool isJoint)
  {
    if (isJoint) {
      attributes.flags|=SegmentAttributes::endIsJoint;
    }
  }

  void Way::GetBoundingBox(double& minLon,
                           double& maxLon,
                           double& minLat,
                           double& maxLat) const
  {
    assert(!nodes.empty());

    minLon=nodes[0].GetLon();
    maxLon=nodes[0].GetLon();
    minLat=nodes[0].GetLat();
    maxLat=nodes[0].GetLat();

    for (size_t i=1; i<nodes.size(); i++) {
      minLon=std::min(minLon,nodes[i].GetLon());
      maxLon=std::max(maxLon,nodes[i].GetLon());
      minLat=std::min(minLat,nodes[i].GetLat());
      maxLat=std::max(maxLat,nodes[i].GetLat());
    }
  }

  bool Way::GetCoordinates(Id nodeId,
                           double& lat,
                           double& lon) const
  {
    for (size_t i=0; i<nodes.size(); i++) {
      if (nodes[i].GetId()==nodeId) {
        lat=nodes[i].GetLat();
        lon=nodes[i].GetLon();

        return true;
      }
    }

    return false;
  }

  bool Way::Read(FileScanner& scanner)
  {
    uint32_t nodeCount;

    scanner.ReadNumber(id);

    if (!attributes.Read(scanner)) {
      return false;
    }

    scanner.ReadNumber(nodeCount);

    if (scanner.HasError()) {
      return false;
    }

    if (nodeCount>0) {
      Id       minId;
      uint32_t minLat;
      uint32_t minLon;

      scanner.Read(minId),
      scanner.Read(minLat);
      scanner.Read(minLon);

      nodes.resize(nodeCount);
      for (size_t i=0; i<nodeCount; i++) {
        Id       id;
        uint32_t latValue;
        uint32_t lonValue;

        scanner.ReadNumber(id);
        scanner.ReadNumber(latValue);
        scanner.ReadNumber(lonValue);

        nodes[i].Set(minId+id,
                     (minLat+latValue)/conversionFactor-90.0,
                     (minLon+lonValue)/conversionFactor-180.0);
      }
    }

    return !scanner.HasError();
  }

  bool Way::Write(FileWriter& writer) const
  {
    writer.WriteNumber(id);

    if (!attributes.Write(writer)) {
      return false;
    }

    writer.WriteNumber((uint32_t)nodes.size());

    if (!nodes.empty()) {
      Id       minId=std::numeric_limits<Id>::max();
      uint32_t minLat=std::numeric_limits<uint32_t>::max();
      uint32_t minLon=std::numeric_limits<uint32_t>::max();

      for (size_t i=0; i<nodes.size(); i++) {
        minId=std::min(minId,nodes[i].GetId());
        minLat=std::min(minLat,(uint32_t)floor((nodes[i].GetLat()+90.0)*conversionFactor+0.5));
        minLon=std::min(minLon,(uint32_t)floor((nodes[i].GetLon()+180.0)*conversionFactor+0.5));
      }

      writer.Write(minId);
      writer.Write(minLat);
      writer.Write(minLon);

      for (size_t i=0; i<nodes.size(); i++) {
        uint32_t latValue=(uint32_t)floor((nodes[i].GetLat()+90.0)*conversionFactor+0.5);
        uint32_t lonValue=(uint32_t)floor((nodes[i].GetLon()+180.0)*conversionFactor+0.5);

        writer.WriteNumber(nodes[i].GetId()-minId);
        writer.WriteNumber(latValue-minLat);
        writer.WriteNumber(lonValue-minLon);
      }
    }

    return !writer.HasError();
  }
}
