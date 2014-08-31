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

#include <limits>

#include <osmscout/util/String.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

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

  void Way::SetLayerToMax()
  {
    // TODO
    // attributes.SetLayer(std::numeric_limits<int8_t>::max());
  }

  void Way::GetCoordinates(size_t nodeIndex,
                           double& lat,
                           double& lon) const
  {
    assert(nodeIndex<nodes.size());

    lat=nodes[nodeIndex].GetLat();
    lon=nodes[nodeIndex].GetLon();
  }

  bool Way::GetNodeIndexByNodeId(Id id,
                                 size_t& index) const
  {
    for (size_t i=0; i<ids.size(); i++) {
      if (ids[i]==id) {
        index=i;

        return true;
      }
    }

    return false;
  }

  bool Way::Read(const TypeConfig& typeConfig,
                 FileScanner& scanner)
  {
    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    uint32_t tmpType;

    scanner.ReadNumber(tmpType);

    TypeInfoRef type=typeConfig.GetTypeInfo((TypeId)tmpType);

    featureValueBuffer.SetType(type);

    if (!featureValueBuffer.Read(scanner)) {
      return false;
    }

    if (!scanner.Read(nodes)) {
      return false;
    }

    ids.resize(nodes.size());

    Id minId;

    scanner.ReadNumber(minId);

    if (minId>0) {
      size_t idCurrent=0;

      while (idCurrent<ids.size()) {
        uint8_t bitset;
        size_t  bitmask=1;

        scanner.Read(bitset);

        for (size_t i=0; i<8 && idCurrent<ids.size(); i++) {
          if (bitset & bitmask) {
            scanner.ReadNumber(ids[idCurrent]);

            ids[idCurrent]+=minId;
          }
          else {
            ids[idCurrent]=0;
          }

          bitmask*=2;
          idCurrent++;
        }
      }
    }

    return !scanner.HasError();
  }

  bool Way::ReadOptimized(const TypeConfig& typeConfig,
                          FileScanner& scanner)
  {
    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    uint32_t tmpType;

    scanner.ReadNumber(tmpType);

    TypeInfoRef type=typeConfig.GetTypeInfo((TypeId)tmpType);

    featureValueBuffer.SetType(type);

    if (!featureValueBuffer.Read(scanner)) {
      return false;
    }

    if (!scanner.Read(nodes)) {
      return false;
    }

    return !scanner.HasError();
  }

  bool Way::Write(const TypeConfig& /*typeConfig*/,
                  FileWriter& writer) const
  {
    assert(!nodes.empty());

    writer.WriteNumber(featureValueBuffer.GetType()->GetId());

    if (!featureValueBuffer.Write(writer)) {
      return false;
    }

    if (!writer.Write(nodes)) {
      return false;
    }

    Id minId=0;

    for (size_t i=0; i<ids.size(); i++) {
      if (ids[i]!=0) {
        if (minId==0) {
          minId=ids[i];
        }
        else {
          minId=std::min(minId,ids[i]);
        }
      }
    }

    writer.WriteNumber(minId);

    if (minId>0) {
      size_t idCurrent=0;

      while (idCurrent<ids.size()) {
        uint8_t bitset=0;
        size_t  bitMask=1;
        size_t  idEnd=std::min(idCurrent+8,ids.size());

        for (size_t i=idCurrent; i<idEnd; i++) {
          if (ids[i]!=0) {
            bitset=bitset | bitMask;
          }

          bitMask*=2;
        }

        writer.Write(bitset);

        for (size_t i=idCurrent; i<idEnd; i++) {
          if (ids[i]!=0) {
            writer.WriteNumber(ids[i]-minId);
          }

          bitMask=bitMask*2;
        }

        idCurrent+=8;
      }
    }

    return !writer.HasError();
  }

  bool Way::WriteOptimized(const TypeConfig& /*typeConfig*/,
                           FileWriter& writer) const
  {
    assert(!nodes.empty());

    writer.WriteNumber(featureValueBuffer.GetType()->GetId());

    if (!featureValueBuffer.Write(writer)) {
      return false;
    }

    if (!writer.Write(nodes)) {
      return false;
    }

    return !writer.HasError();
  }
}
