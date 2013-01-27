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

  void Way::SetType(TypeId type)
  {
    attributes.type=type;
  }

  bool Way::SetTags(Progress& progress,
                    const TypeConfig& typeConfig,
                    Id id,
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
      if (ids[i]==nodeId) {
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

    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    if (!attributes.Read(scanner)) {
      return false;
    }

    if (!scanner.ReadNumber(nodeCount)) {
      return false;
    }

    Id       minId;
    uint32_t minLat;
    uint32_t minLon;

    scanner.ReadNumber(minId);

    ids.resize(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      Id id;

      scanner.ReadNumber(id);

      ids[i]=minId+id;
    }

    scanner.Read(minLat);
    scanner.Read(minLon);

    nodes.resize(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      uint32_t latValue;
      uint32_t lonValue;

      scanner.ReadNumber(latValue);
      scanner.ReadNumber(lonValue);

      nodes[i].Set((minLat+latValue)/conversionFactor-90.0,
                   (minLon+lonValue)/conversionFactor-180.0);
    }

    return !scanner.HasError();
  }

  bool Way::Write(FileWriter& writer) const
  {
    assert(!nodes.empty());

    if (!attributes.Write(writer)) {
      return false;
    }

    writer.WriteNumber((uint32_t)nodes.size());

    Id       minId=ids[0];
    double   minLat=nodes[0].GetLat();
    double   minLon=nodes[0].GetLon();
    uint32_t minLatValue;
    uint32_t minLonValue;

    for (size_t i=1; i<ids.size(); i++) {
      minId=std::min(minId,ids[i]);
    }

    writer.WriteNumber(minId);

    for (size_t i=0; i<nodes.size(); i++) {
      writer.WriteNumber(ids[i]-minId);
    }

    for (size_t i=1; i<nodes.size(); i++) {
      minLat=std::min(minLat,nodes[i].GetLat());
      minLon=std::min(minLon,nodes[i].GetLon());
    }

    minLatValue=(uint32_t)round((minLat+90.0)*conversionFactor);
    minLonValue=(uint32_t)round((minLon+180.0)*conversionFactor);

    writer.Write(minLatValue);
    writer.Write(minLonValue);

    for (size_t i=0; i<nodes.size(); i++) {
      uint32_t latValue=(uint32_t)round((nodes[i].GetLat()-minLat)*conversionFactor);
      uint32_t lonValue=(uint32_t)round((nodes[i].GetLon()-minLon)*conversionFactor);

      writer.WriteNumber(latValue);
      writer.WriteNumber(lonValue);
    }

    return !writer.HasError();
  }
}
