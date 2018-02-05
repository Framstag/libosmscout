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

#include <algorithm>
#include <limits>

#include <osmscout/util/String.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  const uint8_t Area::masterRingId=0;
  const uint8_t Area::outerRingId=1;

  bool Area::Ring::HasAnyFeaturesSet() const
  {
    for (size_t f=0; f<featureValueBuffer.GetType()->GetFeatureCount(); f++) {
      if (featureValueBuffer.HasFeature(f)) {
        return true;
      }
    }

    return false;
  }

  bool Area::Ring::GetNodeIndexByNodeId(Id id,
                                        size_t& index) const
  {
    for (size_t i=0; i<nodes.size(); i++) {
      if (nodes[i].GetId()==id) {
        index=i;

        return true;
      }
    }

    return false;
  }

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
      if (ring.IsOuterRing()) {
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

    for (const auto& ring : rings) {
      if (ring.IsOuterRing()) {
        if (!boundingBox.IsValid()) {
          ring.GetBoundingBox(boundingBox);
        }
        else {
          GeoBox ringBoundingBox;

          ring.GetBoundingBox(ringBoundingBox);

          boundingBox.Include(ringBoundingBox);
        }
      }
    }
  }

  /**
   * Reads data from the given Filescanner. Node ids will only be read
   * if not thought to be required for this area.
   *
   * @throws IOException
   */
  void Area::Read(const TypeConfig& typeConfig,
                  FileScanner& scanner)
  {
    TypeId             ringType;
    bool               multipleRings;
    bool               hasMaster;
    uint32_t           ringCount=1;
    FeatureValueBuffer featureValueBuffer;

    fileOffset=scanner.GetPos();

    scanner.ReadTypeId(ringType,
                       typeConfig.GetAreaTypeIdBytes());

    TypeInfoRef type=typeConfig.GetAreaTypeInfo(ringType);

    featureValueBuffer.SetType(type);

    featureValueBuffer.Read(scanner,
                            multipleRings,
                            hasMaster);

    if (multipleRings) {
      scanner.ReadNumber(ringCount);

      ringCount++;
    }

    rings.resize(ringCount);

    rings[0].featureValueBuffer=std::move(featureValueBuffer);

    if (hasMaster) {
      rings[0].MarkAsMasterRing();
    }
    else {
      rings[0].MarkAsOuterRing();
    }

    scanner.Read(rings[0].nodes,
                 rings[0].GetType()->CanRoute());

    for (size_t i=1; i<ringCount; i++) {
      scanner.ReadTypeId(ringType,
                         typeConfig.GetAreaTypeIdBytes());

      type=typeConfig.GetAreaTypeInfo(ringType);

      rings[i].SetType(type);

      if (rings[i].GetType()->GetAreaId()!=typeIgnore) {
        rings[i].featureValueBuffer.Read(scanner);
      }

      scanner.Read(rings[i].ring);
      scanner.Read(rings[i].nodes,
                   rings[i].GetType()->GetAreaId()!=typeIgnore &&
                   rings[i].GetType()->CanRoute());
    }
    nextFileOffset=scanner.GetPos();
  }

  /**
   * Reads data from the given FileScanner. All data available will be read.
   *
   * @throws IOException
   */
  void Area::ReadImport(const TypeConfig& typeConfig,
                        FileScanner& scanner)
  {
    TypeId             ringType;
    bool               multipleRings;
    bool               hasMaster;
    uint32_t           ringCount=1;
    FeatureValueBuffer featureValueBuffer;

    fileOffset=scanner.GetPos();

    scanner.ReadTypeId(ringType,
                       typeConfig.GetAreaTypeIdBytes());

    TypeInfoRef type=typeConfig.GetAreaTypeInfo(ringType);

    featureValueBuffer.SetType(type);

    featureValueBuffer.Read(scanner,
                            multipleRings,
                            hasMaster);

    if (multipleRings) {
      scanner.ReadNumber(ringCount);

      ringCount++;
    }

    rings.resize(ringCount);

    rings[0].featureValueBuffer=featureValueBuffer;

    if (hasMaster) {
      rings[0].MarkAsMasterRing();
    }
    else {
      rings[0].MarkAsOuterRing();
    }

    scanner.Read(rings[0].nodes,
                 true);

    for (size_t i=1; i<ringCount; i++) {
      scanner.ReadTypeId(ringType,
                         typeConfig.GetAreaTypeIdBytes());

      type=typeConfig.GetAreaTypeInfo(ringType);

      rings[i].SetType(type);

      if (rings[i].GetType()->GetAreaId()!=typeIgnore) {
        rings[i].featureValueBuffer.Read(scanner);
      }

      scanner.Read(rings[i].ring);
      scanner.Read(rings[i].nodes,
                   rings[i].GetType()->GetAreaId()!=typeIgnore ||
                   rings[i].ring==outerRingId);
    }
    nextFileOffset=scanner.GetPos();
  }

  /**
   * Reads data to the given FileScanner. No node ids will be read.
   *
   * @throws IOException
   */
  void Area::ReadOptimized(const TypeConfig& typeConfig,
                           FileScanner& scanner)
  {
    TypeId             ringType;
    bool               multipleRings;
    bool               hasMaster;
    uint32_t           ringCount=1;
    FeatureValueBuffer featureValueBuffer;

    fileOffset=scanner.GetPos();

    scanner.ReadTypeId(ringType,
                       typeConfig.GetAreaTypeIdBytes());

    TypeInfoRef type=typeConfig.GetAreaTypeInfo(ringType);

    featureValueBuffer.SetType(type);

    featureValueBuffer.Read(scanner,
                            multipleRings,
                            hasMaster);

    if (multipleRings) {
      scanner.ReadNumber(ringCount);

      ringCount++;
    }

    rings.resize(ringCount);

    rings[0].featureValueBuffer=featureValueBuffer;

    if (hasMaster) {
      rings[0].MarkAsMasterRing();
    }
    else {
      rings[0].MarkAsOuterRing();
    }

    scanner.Read(rings[0].nodes,
                 false);

    for (size_t i=1; i<ringCount; i++) {
      scanner.ReadTypeId(ringType,
                         typeConfig.GetAreaTypeIdBytes());

      type=typeConfig.GetAreaTypeInfo(ringType);

      rings[i].SetType(type);

      if (rings[i].featureValueBuffer.GetType()->GetAreaId()!=typeIgnore) {
        rings[i].featureValueBuffer.Read(scanner);
      }

      scanner.Read(rings[i].ring);
      scanner.Read(rings[i].nodes,
                   false);
    }
    nextFileOffset=scanner.GetPos();
  }

  /**
   * Writes data to the given FileWriter. Node ids will only be written
   * if not thought to be required for this area.
   *
   * @throws IOException
   */
  void Area::Write(const TypeConfig& typeConfig,
                   FileWriter& writer) const
  {
    std::vector<Ring>::const_iterator ring=rings.begin();
    bool                              multipleRings=rings.size()>1;
    bool                              hasMaster=rings[0].IsMasterRing();

    // TODO: We would like to have a bit flag here, if we have a simple area,
    // an area with one master (and multiple rings) or an area with
    // multiple outer but no master
    //
    // Also for each ring we would like to have a bit flag, if
    // we stor eids or not

    // Outer ring

    writer.WriteTypeId(ring->GetType()->GetAreaId(),
                       typeConfig.GetAreaTypeIdBytes());

    ring->featureValueBuffer.Write(writer,
                                   multipleRings,
                                   hasMaster);

    if (multipleRings) {
      writer.WriteNumber((uint32_t)(rings.size()-1));
    }

    writer.Write(ring->nodes,
                 ring->GetType()->CanRoute());

    ++ring;

    // Potential additional rings

    while (ring!=rings.end()) {
      writer.WriteTypeId(ring->GetType()->GetAreaId(),
                         typeConfig.GetAreaTypeIdBytes());

      if (ring->GetType()->GetAreaId()!=typeIgnore) {
        ring->featureValueBuffer.Write(writer);
      }

      writer.Write(ring->ring);
      writer.Write(ring->nodes,
                   ring->GetType()->GetAreaId()!=typeIgnore &&
                   ring->GetType()->CanRoute());

      ++ring;
    }
  }

  /**
   * Writes data to the given FileWriter. All data available will be written.
   *
   * @throws IOException
   */
  void Area::WriteImport(const TypeConfig& typeConfig,
                         FileWriter& writer) const
  {
    std::vector<Ring>::const_iterator ring=rings.begin();
    bool                              multipleRings=rings.size()>1;
    bool                              hasMaster=ring->IsMasterRing();

    // Master/Outer ring

    writer.WriteTypeId(ring->GetType()->GetAreaId(),
                       typeConfig.GetAreaTypeIdBytes());

    ring->featureValueBuffer.Write(writer,
                                   multipleRings,
                                   hasMaster);

    if (multipleRings) {
      writer.WriteNumber((uint32_t)(rings.size()-1));
    }

    writer.Write(ring->nodes,
                 true);

    ++ring;

    // Potential additional rings

    while (ring!=rings.end()) {
      writer.WriteTypeId(ring->GetType()->GetAreaId(),
                         typeConfig.GetAreaTypeIdBytes());

      if (ring->GetType()->GetAreaId()!=typeIgnore) {
        ring->featureValueBuffer.Write(writer);
      }

      writer.Write(ring->ring);
      writer.Write(ring->nodes,
                   ring->GetType()->GetAreaId()!=typeIgnore ||
                   ring->ring==outerRingId);

      ++ring;
    }
  }

  /**
   * Writes data to the given FileWriter. No node ids will be written.
   *
   * @throws IOException
   */
  void Area::WriteOptimized(const TypeConfig& typeConfig,
                            FileWriter& writer) const
  {
    std::vector<Ring>::const_iterator ring=rings.begin();
    bool                              multipleRings=rings.size()>1;
    bool                              hasMaster=rings[0].IsMasterRing();

    // Outer ring

    writer.WriteTypeId(ring->GetType()->GetAreaId(),
                       typeConfig.GetAreaTypeIdBytes());

    ring->featureValueBuffer.Write(writer,
                                   multipleRings,
                                   hasMaster);

    if (multipleRings) {
      writer.WriteNumber((uint32_t)(rings.size()-1));
    }

    writer.Write(ring->nodes,
                 false);

    ++ring;

    // Potential additional rings

    while (ring!=rings.end()) {
      writer.WriteTypeId(ring->GetType()->GetAreaId(),
                         typeConfig.GetAreaTypeIdBytes());

      if (ring->GetType()->GetAreaId()!=typeIgnore) {
        ring->featureValueBuffer.Write(writer);
      }

      writer.Write(ring->ring);
      writer.Write(ring->nodes,
                   false);

      ++ring;
    }
  }
}

