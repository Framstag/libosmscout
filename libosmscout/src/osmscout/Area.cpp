/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/Area.h>

#include <limits>

#include <osmscout/util/String.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  bool Area::Ring::GetCenter(GeoCoord& center) const
  {
    double minLat=0.0;
    double minLon=0.0;
    double maxLat=0.0;
    double maxLon=0.0;

    bool start=true;

    for (size_t j=0; j<nodes.size(); j++) {
      if (start) {
        minLat=nodes[j].GetLat();
        minLon=nodes[j].GetLon();
        maxLat=nodes[j].GetLat();
        maxLon=nodes[j].GetLon();

        start=false;
      }
      else {
        minLat=std::min(minLat,nodes[j].GetLat());
        minLon=std::min(minLon,nodes[j].GetLon());
        maxLat=std::max(maxLat,nodes[j].GetLat());
        maxLon=std::max(maxLon,nodes[j].GetLon());
      }
    }

    if (start) {
      return false;
    }

    center.Set(minLat+(maxLat-minLat)/2,
               minLon+(maxLon-minLon)/2);

    return true;
  }

  void Area::Ring::GetBoundingBox(GeoBox& boundingBox) const
  {
    assert(!nodes.empty());

    double minLon=nodes[0].GetLon();
    double maxLon=minLon;
    double minLat=nodes[0].GetLat();
    double maxLat=minLat;

    for (size_t i=1; i<nodes.size(); i++) {
      minLon=std::min(minLon,nodes[i].GetLon());
      maxLon=std::max(maxLon,nodes[i].GetLon());
      minLat=std::min(minLat,nodes[i].GetLat());
      maxLat=std::max(maxLat,nodes[i].GetLat());
    }

    boundingBox.Set(GeoCoord(minLat,minLon),
                    GeoCoord(maxLat,maxLon));
  }

  bool Area::GetCenter(GeoCoord& center) const
  {
    assert(!rings.empty());

    double minLat=0.0;
    double minLon=0.0;
    double maxLat=0.0;
    double maxLon=0.0;

    bool start=true;

    for (const auto& ring : rings) {
      if (ring.ring==Area::outerRingId) {
        for (size_t j=0; j<ring.nodes.size(); j++) {
          if (start) {
            minLat=ring.nodes[j].GetLat();
            maxLat=minLat;
            minLon=ring.nodes[j].GetLon();
            maxLon=minLon;

            start=false;
          }
          else {
            minLat=std::min(minLat,ring.nodes[j].GetLat());
            minLon=std::min(minLon,ring.nodes[j].GetLon());
            maxLat=std::max(maxLat,ring.nodes[j].GetLat());
            maxLon=std::max(maxLon,ring.nodes[j].GetLon());
          }
        }
      }
    }

    assert(!start);

    if (start) {
      return false;
    }

    center.Set(minLat+(maxLat-minLat)/2,
               minLon+(maxLon-minLon)/2);

    return true;
  }

  void Area::GetBoundingBox(GeoBox& boundingBox) const
  {
    boundingBox.Invalidate();

    for (const auto& role : rings) {
      if (role.ring==Area::outerRingId) {
        if (!boundingBox.IsValid()) {
          role.GetBoundingBox(boundingBox);
        }
        else {
          GeoBox ringBoundingBox;

          role.GetBoundingBox(ringBoundingBox);

          boundingBox.Include(ringBoundingBox);
        }
      }
    }
  }

  bool Area::ReadIds(FileScanner& scanner,
                     uint32_t nodesCount,
                     std::vector<Id>& ids)
  {
    ids.resize(nodesCount);

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

  bool Area::Read(const TypeConfig& typeConfig,
                  FileScanner& scanner)
  {
    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    TypeId   ringType;
    bool     multipleRings;
    uint32_t ringCount=1;
    uint32_t nodesCount;
    FeatureValueBuffer featureValueBuffer;

    scanner.ReadTypeId(ringType,
                       typeConfig.GetAreaTypeIdBytes());

    TypeInfoRef type=typeConfig.GetAreaTypeInfo(ringType);

    featureValueBuffer.SetType(type);

    if (!featureValueBuffer.Read(scanner,
                                 multipleRings)) {
      return false;
    }

    if (multipleRings) {
      if (!scanner.ReadNumber(ringCount)) {
        return false;
      }

      ringCount++;
    }

    rings.resize(ringCount);

    rings[0].featureValueBuffer=featureValueBuffer;

    if (ringCount>1) {
      rings[0].ring=masterRingId;
    }
    else {
      rings[0].ring=outerRingId;
    }

    if (!scanner.ReadNumber(nodesCount)) {
      return false;
    }

    if (nodesCount>0) {
      if (!ReadIds(scanner,
                   nodesCount,
                   rings[0].ids)) {
        return false;
      }

      if (!scanner.Read(rings[0].nodes,
                        nodesCount)) {
        return false;
      }
    }

    for (size_t i=1; i<ringCount; i++) {
      scanner.ReadTypeId(ringType,
                         typeConfig.GetAreaTypeIdBytes());

      type=typeConfig.GetAreaTypeInfo(ringType);

      rings[i].SetType(type);

      if (rings[i].featureValueBuffer.GetType()->GetAreaId()!=typeIgnore) {
        if (!rings[i].featureValueBuffer.Read(scanner)) {
          return false;
        }
      }

      scanner.Read(rings[i].ring);

      scanner.ReadNumber(nodesCount);

      if (nodesCount>0 &&
          rings[i].GetType()->GetAreaId()!=typeIgnore) {
        if (!ReadIds(scanner,
                     nodesCount,
                     rings[i].ids)) {
          return false;
        }
      }

      if (nodesCount>0) {
        if (!scanner.Read(rings[i].nodes,
                          nodesCount)) {
          return false;
        }
      }
    }

    return !scanner.HasError();
  }

  bool Area::ReadOptimized(const TypeConfig& typeConfig,
                           FileScanner& scanner)
  {
    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    TypeId   ringType;
    bool     multipleRings;
    uint32_t ringCount=1;
    uint32_t nodesCount;
    FeatureValueBuffer featureValueBuffer;

    scanner.ReadTypeId(ringType,
                       typeConfig.GetAreaTypeIdBytes());

    TypeInfoRef type=typeConfig.GetAreaTypeInfo(ringType);

    featureValueBuffer.SetType(type);

    if (!featureValueBuffer.Read(scanner,
                                 multipleRings)) {
      return false;
    }

    if (multipleRings) {
      if (!scanner.ReadNumber(ringCount)) {
        return false;
      }

      ringCount++;
    }

    rings.resize(ringCount);

    rings[0].featureValueBuffer=featureValueBuffer;

    if (ringCount>1) {
      rings[0].ring=masterRingId;
    }
    else {
      rings[0].ring=outerRingId;
    }

    if (!scanner.ReadNumber(nodesCount)) {
      return false;
    }

    if (nodesCount>0) {
      if (!scanner.Read(rings[0].nodes,
                        nodesCount)) {
        return false;
      }
    }

    for (size_t i=1; i<ringCount; i++) {
      scanner.ReadTypeId(ringType,
                         typeConfig.GetAreaTypeIdBytes());

      type=typeConfig.GetAreaTypeInfo(ringType);

      rings[i].SetType(type);

      if (rings[i].featureValueBuffer.GetType()->GetAreaId()!=typeIgnore) {
        if (!rings[i].featureValueBuffer.Read(scanner)) {
          return false;
        }
      }

      scanner.Read(rings[i].ring);

      scanner.ReadNumber(nodesCount);

      if (nodesCount>0) {
        if (!scanner.Read(rings[i].nodes,
                          nodesCount)) {
          return false;
        }
      }
    }

    return !scanner.HasError();
  }

  bool Area::WriteIds(FileWriter& writer,
                      const std::vector<Id>& ids) const
  {
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
        uint8_t bitMask=1;
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

  bool Area::Write(const TypeConfig& typeConfig,
                   FileWriter& writer) const
  {
    std::vector<Ring>::const_iterator ring=rings.begin();
    bool                              multipleRings=rings.size()>1;

    // Outer ring

    writer.WriteTypeId(ring->GetType()->GetAreaId(),
                       typeConfig.GetAreaTypeIdBytes());

    if (!ring->featureValueBuffer.Write(writer,
                                        multipleRings)) {
      return false;
    }

    if (multipleRings) {
      writer.WriteNumber((uint32_t)(rings.size()-1));
    }

    writer.WriteNumber((uint32_t)ring->nodes.size());

    if (!ring->nodes.empty()) {
      if (!WriteIds(writer,
                    ring->ids)) {
        return false;
      }

      if (!writer.Write(ring->nodes,
                        ring->nodes.size())) {
        return false;
      }
    }

    ++ring;

    // Potential additional rings

    while (ring!=rings.end()) {
      writer.WriteTypeId(ring->GetType()->GetAreaId(),
                         typeConfig.GetAreaTypeIdBytes());

      if (ring->GetType()->GetAreaId()!=typeIgnore) {
        if (!ring->featureValueBuffer.Write(writer)) {
          return false;
        }
      }

      writer.Write(ring->ring);

      writer.WriteNumber((uint32_t)ring->nodes.size());

      if (!ring->nodes.empty() &&
          ring->featureValueBuffer.GetType()->GetAreaId()!=typeIgnore) {
        if (!WriteIds(writer,
                      ring->ids)) {
          return false;
        }
      }

      if (!ring->nodes.empty()) {
        if (!writer.Write(ring->nodes,
                          ring->nodes.size())) {
          return false;
        }
      }

      ++ring;
    }

    return !writer.HasError();
  }

  bool Area::WriteOptimized(const TypeConfig& typeConfig,
                            FileWriter& writer) const
  {
    std::vector<Ring>::const_iterator ring=rings.begin();
    bool                              multipleRings=rings.size()>1;

    // Outer ring

    writer.WriteTypeId(ring->GetType()->GetAreaId(),
                       typeConfig.GetAreaTypeIdBytes());

    if (!ring->featureValueBuffer.Write(writer,
                                        multipleRings)) {
      return false;
    }

    if (multipleRings) {
      writer.WriteNumber((uint32_t)(rings.size()-1));
    }

    writer.WriteNumber((uint32_t)ring->nodes.size());

    if (!ring->nodes.empty()) {
      if (!writer.Write(ring->nodes,
                        ring->nodes.size())) {
        return false;
      }
    }

    ++ring;

    // Potential additional rings

    while (ring!=rings.end()) {
      writer.WriteTypeId(ring->GetType()->GetAreaId(),
                         typeConfig.GetAreaTypeIdBytes());

      if (ring->GetType()->GetAreaId()!=typeIgnore) {
        if (!ring->featureValueBuffer.Write(writer)) {
          return false;
        }
      }

      writer.Write(ring->ring);

      writer.WriteNumber((uint32_t)ring->nodes.size());

      if (!ring->nodes.empty()) {
        if (!writer.Write(ring->nodes,
                          ring->nodes.size())) {
          return false;
        }
      }

      ++ring;
    }

    return !writer.HasError();
  }
}

