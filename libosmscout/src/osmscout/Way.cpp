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

  void WayAttributes::SetType(TypeId type)
  {
    this->type=type;
  }

  void WayAttributes::SetFeatures(const TypeConfig& typeConfig,
                                  const FeatureValueBuffer& buffer)
  {
    flags=0;
    name.clear();
    nameAlt.clear();
    ref.clear();
    layer=0;
    width=0;
    maxSpeed=0;
    grade=1;
    access.SetAccess(buffer.GetType()->GetDefaultAccess());

    for (size_t i=0; i<buffer.GetFeatureCount(); i++) {
      if (buffer.HasValue(i)) {

        if (buffer.GetFeature(i).GetFeature()==typeConfig.featureName &&
          buffer.GetFeature(i).GetFeature()->HasValue()) {
          NameFeatureValue* value=dynamic_cast<NameFeatureValue*>(buffer.GetValue(i));

          name=value->GetName();
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureNameAlt &&
          buffer.GetFeature(i).GetFeature()->HasValue()) {
          NameAltFeatureValue* value=dynamic_cast<NameAltFeatureValue*>(buffer.GetValue(i));

          nameAlt=value->GetNameAlt();
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureRef &&
          buffer.GetFeature(i).GetFeature()->HasValue()) {
          RefFeatureValue* value=dynamic_cast<RefFeatureValue*>(buffer.GetValue(i));

          ref=value->GetRef();
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureLayer &&
          buffer.GetFeature(i).GetFeature()->HasValue()) {
          LayerFeatureValue* value=dynamic_cast<LayerFeatureValue*>(buffer.GetValue(i));

          layer=value->GetLayer();
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureWidth &&
          buffer.GetFeature(i).GetFeature()->HasValue()) {
          WidthFeatureValue* value=dynamic_cast<WidthFeatureValue*>(buffer.GetValue(i));

          width=value->GetWidth();
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureMaxSpeed &&
          buffer.GetFeature(i).GetFeature()->HasValue()) {
          MaxSpeedFeatureValue* value=dynamic_cast<MaxSpeedFeatureValue*>(buffer.GetValue(i));

          maxSpeed=value->GetMaxSpeed();
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureGrade &&
          buffer.GetFeature(i).GetFeature()->HasValue()) {
          GradeFeatureValue* value=dynamic_cast<GradeFeatureValue*>(buffer.GetValue(i));

          grade=value->GetGrade();
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureAccess &&
          buffer.GetFeature(i).GetFeature()->HasValue()) {
          AccessFeatureValue* value=dynamic_cast<AccessFeatureValue*>(buffer.GetValue(i));

          access.SetAccess(value->GetAccess());
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureBridge) {
          flags|=isBridge;
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureTunnel) {
          flags|=isTunnel;
        }
        else if (buffer.GetFeature(i).GetFeature()==typeConfig.featureRoundabout) {
          flags|=isRoundabout;
        }
      }
    }
  }

  void WayAttributes::SetLayer(int8_t layer)
  {
    this->layer=layer;
  }

  bool WayAttributes::Read(FileScanner& scanner)
  {
    uint16_t flags;

    scanner.ReadNumber(type);
    scanner.Read(flags);

    if (scanner.HasError()) {
      return false;
    }

    this->flags=flags;

    access.Read(scanner);

    if (flags & hasName) {
      scanner.Read(name);
    }

    if (flags & hasNameAlt) {
      scanner.Read(nameAlt);
    }

    if (flags & hasRef) {
      scanner.Read(ref);
    }

    if (flags & hasLayer) {
      scanner.Read(layer);
    }
    else {
      layer=0;
    }

    if (flags & hasWidth) {
      scanner.Read(width);
    }
    else {
      width=0;
    }

    if (flags & hasMaxSpeed) {
      scanner.Read(maxSpeed);
    }
    else {
      maxSpeed=0;
    }

    if (flags & hasGrade) {
      scanner.Read(grade);
    }
    else {
      grade=1;
    }

    if (flags & hasTags) {
      uint32_t tagCount;

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

    return !scanner.HasError();
  }

  bool WayAttributes::Write(FileWriter& writer) const
  {
    writer.WriteNumber(type);

    if (!name.empty()) {
      flags|=hasName;
    }
    else {
      flags&=~hasName;
    }

    if (!nameAlt.empty()) {
      flags|=hasNameAlt;
    }
    else {
      flags&=~hasNameAlt;
    }

    if (!ref.empty()) {
      flags|=hasRef;
    }
    else {
      flags&=~hasRef;
    }

    if (layer!=0) {
      flags|=hasLayer;
    }
    else {
      flags&=~hasLayer;
    }

    if (width!=0) {
      flags|=hasWidth;
    }
    else {
      flags&=~hasWidth;
    }

    if (maxSpeed!=0) {
      flags|=hasMaxSpeed;
    }
    else {
      flags&=~hasMaxSpeed;
    }

    if (grade!=1) {
      flags|=hasGrade;
    }
    else {
      flags&=~hasGrade;
    }

    if (!tags.empty()) {
      flags|=hasTags;
    }
    else {
      flags&=~hasTags;
    }

    writer.Write(flags);

    access.Write(writer);

    if (flags & hasName) {
      writer.Write(name);
    }

    if (flags & hasNameAlt) {
      writer.Write(nameAlt);
    }

    if (flags & hasRef) {
      writer.Write(ref);
    }

    if (flags & hasLayer) {
      writer.Write(layer);
    }

    if (flags & hasWidth) {
      writer.Write(width);
    }

    if (flags & hasMaxSpeed) {
      writer.Write(maxSpeed);
    }

    if (flags & hasGrade) {
      writer.Write(grade);
    }

    if (flags & hasTags) {
      writer.WriteNumber((uint32_t)tags.size());

      for (size_t i=0; i<tags.size(); i++) {
        writer.WriteNumber(tags[i].key);
        writer.Write(tags[i].value);
      }
    }

    return !writer.HasError();
  }

  bool WayAttributes::operator==(const WayAttributes& other) const
  {
    if (type!=other.type) {
      return false;
    }

    if ((flags & (isBridge | isTunnel | isRoundabout))!=
        (other.flags & (isBridge | isTunnel | isRoundabout))) {
      return false;
    }

    if (access!=other.access ||
        name!=other.name ||
        nameAlt!=other.nameAlt ||
        ref!=other.ref ||
        layer!=other.layer ||
        width!=other.width ||
        maxSpeed!=other.maxSpeed ||
        grade!=other.grade) {
      return false;
    }

    if (tags.empty() && other.tags.empty()) {
      return true;
    }

    if (tags.size()!=other.tags.size()) {
      return false;
    }

    for (size_t t=0; t<tags.size(); t++) {
      if (tags[t].key!=other.tags[t].key) {
        return false;
      }

      if (tags[t].value!=other.tags[t].value) {
        return false;
      }
    }

    return true;
  }

  bool WayAttributes::operator!=(const WayAttributes& other) const
  {
    return !this->operator==(other);
  }

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
    attributes.SetType(type);
  }

  void Way::SetFeatures(const TypeConfig& typeConfig,
                        const FeatureValueBuffer& buffer)
  {
    attributes.SetFeatures(typeConfig,
                           buffer);
  }

  void Way::SetLayerToMax()
  {
    attributes.SetLayer(std::numeric_limits<int8_t>::max());
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

  bool Way::Read(FileScanner& scanner)
  {
    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    if (!attributes.Read(scanner)) {
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

  bool Way::ReadOptimized(FileScanner& scanner)
  {
    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    if (!attributes.Read(scanner)) {
      return false;
    }

    if (!scanner.Read(nodes)) {
      return false;
    }

    return !scanner.HasError();
  }

  bool Way::Write(FileWriter& writer) const
  {
    FileOffset fileOffset;

    if (!writer.GetPos(fileOffset)) {
      return false;
    }

    assert(!nodes.empty());

    if (!attributes.Write(writer)) {
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

  bool Way::WriteOptimized(FileWriter& writer) const
  {
    assert(!nodes.empty());

    if (!attributes.Write(writer)) {
      return false;
    }

    if (!writer.Write(nodes)) {
      return false;
    }

    return !writer.HasError();
  }
}
