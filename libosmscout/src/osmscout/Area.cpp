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

#include <osmscout/system/Math.h>

namespace osmscout {

  bool AreaAttributes::SetTags(Progress& progress,
                               const TypeConfig& typeConfig,
                               std::vector<Tag>& tags)
  {
    uint32_t namePriority=0;
    uint32_t nameAltPriority=0;

    name.clear();
    houseNr.clear();

    flags=0;

    this->tags.clear();

    flags|=hasAccess;

    std::vector<Tag>::iterator tag=tags.begin();
    while (tag!=tags.end()) {
      uint32_t ntPrio;
      bool     isNameTag=typeConfig.IsNameTag(tag->key,ntPrio);
      uint32_t natPrio;
      bool     isNameAltTag=typeConfig.IsNameAltTag(tag->key,natPrio);

      if (isNameTag &&
          (name.empty() || ntPrio>namePriority)) {
        name=tag->value;
        namePriority=ntPrio;

        /*
        size_t i=0;
        while (postfixes[i]!=NULL) {
          size_t pos=name.rfind(postfixes[i]);
          if (pos!=std::string::npos &&
              pos==name.length()-strlen(postfixes[i])) {
            name=name.substr(0,pos);
            break;
          }

          i++;
        }*/
      }

      if (isNameAltTag &&
          (nameAlt.empty() || natPrio>nameAltPriority)) {
        nameAlt=tag->value;
        nameAltPriority=natPrio;
      }

      if (isNameTag || isNameAltTag) {
        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagHouseNr) {
        houseNr=tag->value;
        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccess) {
        if (tag->value=="no" ||
            tag->value=="private" ||
            tag->value=="destination" ||
            tag->value=="delivery") {
          flags&=~hasAccess;
        }

        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }

    this->tags=tags;

    return true;
  }

  bool AreaAttributes::Read(FileScanner& scanner)
  {
    uint8_t flags;

    scanner.ReadNumber(type);
    scanner.Read(flags);

    if (scanner.HasError()) {
      return false;
    }

    this->flags=flags;

    if (flags & hasName) {
      scanner.Read(name);
    }

    if (flags & hasNameAlt) {
      scanner.Read(nameAlt);
    }

    if (flags & hasHouseNr) {
      scanner.Read(houseNr);
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

  bool AreaAttributes::Write(FileWriter& writer) const
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

    if (!houseNr.empty()) {
      flags|=hasHouseNr;
    }
    else {
      flags&=~hasHouseNr;
    }

    if (!tags.empty()) {
      flags|=hasTags;
    }
    else {
      flags&=~hasTags;
    }

    writer.Write(flags);

    if (flags & hasName) {
      writer.Write(name);
    }

    if (flags & hasNameAlt) {
      writer.Write(nameAlt);
    }

    if (flags & hasHouseNr) {
      writer.Write(houseNr);
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

  bool AreaAttributes::operator==(const AreaAttributes& other) const
  {
    if (type!=other.type) {
      return false;
    }

    if (name!=other.name ||
        nameAlt!=other.nameAlt ||
        houseNr!=other.houseNr) {
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

  bool AreaAttributes::operator!=(const AreaAttributes& other) const
  {
    return !this->operator==(other);
  }

  bool Area::GetCenter(double& lat, double& lon) const
  {
    if (rings.empty()) {
      return false;
    }

    double minLat=0.0;
    double minLon=0.0;
    double maxLat=0.0;
    double maxLon=0.0;

    bool start=true;

    for (size_t i=0; i<rings.size(); i++) {
      for (size_t j=0; j<rings[i].nodes.size(); j++) {
        if (start) {
          minLat=rings[i].nodes[j].GetLat();
          minLon=rings[i].nodes[j].GetLon();
          maxLat=rings[i].nodes[j].GetLat();
          maxLon=rings[i].nodes[j].GetLon();

          start=false;
        }

        minLat=std::min(minLat,rings[i].nodes[j].GetLat());
        minLon=std::min(minLon,rings[i].nodes[j].GetLon());
        maxLat=std::max(maxLat,rings[i].nodes[j].GetLat());
        maxLon=std::max(maxLon,rings[i].nodes[j].GetLon());
      }
    }

    if (start) {
      return false;
    }

    lat=minLat+(maxLat-minLat)/2;
    lon=minLon+(maxLon-minLon)/2;

    return true;
  }

  void Area::GetBoundingBox(double& minLon,
                                double& maxLon,
                                double& minLat,
                                double& maxLat) const
  {
    assert(!rings.empty());
    assert(!rings[0].nodes.empty());

    minLon=rings[0].nodes[0].GetLon();
    maxLon=rings[0].nodes[0].GetLon();
    minLat=rings[0].nodes[0].GetLat();
    maxLat=rings[0].nodes[0].GetLat();

    for (std::vector<Area::Ring>::const_iterator role=rings.begin();
         role!=rings.end();
         ++role) {
      for (size_t i=0; i<role->nodes.size(); i++) {
        minLon=std::min(minLon,role->nodes[i].GetLon());
        maxLon=std::max(maxLon,role->nodes[i].GetLon());
        minLat=std::min(minLat,role->nodes[i].GetLat());
        maxLat=std::max(maxLat,role->nodes[i].GetLat());
      }
    }
  }

  void Area::SetType(TypeId type)
  {
    attributes.type=type;
  }

  bool Area::Read(FileScanner& scanner)
  {
    uint32_t roleCount;

    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    if (!attributes.Read(scanner)) {
      return false;
    }

    scanner.ReadNumber(roleCount);
    if (scanner.HasError()) {
      return false;
    }

    rings.resize(roleCount);
    for (size_t i=0; i<roleCount; i++) {
      uint32_t nodesCount;

      if (!rings[i].attributes.Read(scanner)) {
        return false;
      }

      scanner.Read(rings[i].ring);

      scanner.ReadNumber(nodesCount);

      if (nodesCount>0) {
        Id       minId;
        uint32_t minLat;
        uint32_t minLon;

        rings[i].ids.resize(nodesCount);
        rings[i].nodes.resize(nodesCount);

        scanner.ReadNumber(minId);
        for (size_t j=0; j<nodesCount; j++) {
          Id id;

          scanner.ReadNumber(id);

          rings[i].ids[j]=minId+id;
        }

        scanner.Read(minLat);
        scanner.Read(minLon);

        for (size_t j=0; j<nodesCount; j++) {
          uint32_t latValue;
          uint32_t lonValue;

          scanner.ReadNumber(latValue);
          scanner.ReadNumber(lonValue);

          rings[i].nodes[j].Set((minLat+latValue)/conversionFactor-90.0,
                                (minLon+lonValue)/conversionFactor-180.0);
        }
      }
    }

    return !scanner.HasError();
  }

  bool Area::ReadOptimized(FileScanner& scanner)
  {
    uint32_t roleCount;

    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    if (!attributes.Read(scanner)) {
      return false;
    }

    scanner.ReadNumber(roleCount);
    if (scanner.HasError()) {
      return false;
    }

    rings.resize(roleCount);
    for (size_t i=0; i<roleCount; i++) {
      uint32_t nodesCount;

      if (!rings[i].attributes.Read(scanner)) {
        return false;
      }

      scanner.Read(rings[i].ring);

      scanner.ReadNumber(nodesCount);

      if (nodesCount>0) {
        uint32_t minLat;
        uint32_t minLon;

        rings[i].nodes.resize(nodesCount);

        scanner.Read(minLat);
        scanner.Read(minLon);

        for (size_t j=0; j<nodesCount; j++) {
          uint32_t latValue;
          uint32_t lonValue;

          scanner.ReadNumber(latValue);
          scanner.ReadNumber(lonValue);

          rings[i].nodes[j].Set((minLat+latValue)/conversionFactor-90.0,
                                (minLon+lonValue)/conversionFactor-180.0);
        }
      }
    }

    return !scanner.HasError();
  }

  bool Area::Write(FileWriter& writer) const
  {
    if (!attributes.Write(writer)) {
      return false;
    }

    writer.WriteNumber((uint32_t)rings.size());
    for (size_t i=0; i<rings.size(); i++) {
      if (!rings[i].attributes.Write(writer)) {
        return false;
      }

      writer.Write(rings[i].ring);

      writer.WriteNumber((uint32_t)rings[i].nodes.size());

      if (!rings[i].nodes.empty()) {
        Id       minId=std::numeric_limits<Id>::max();
        uint32_t minLat=std::numeric_limits<uint32_t>::max();
        uint32_t minLon=std::numeric_limits<uint32_t>::max();

        for (size_t j=0; j<rings[i].ids.size(); j++) {
          minId=std::min(minId,rings[i].ids[j]);
        }

        writer.WriteNumber(minId);

        for (size_t j=0; j<rings[i].nodes.size(); j++) {
          writer.WriteNumber(rings[i].ids[j]-minId);
        }

        for (size_t j=0; j<rings[i].nodes.size(); j++) {
          minLat=std::min(minLat,(uint32_t)round((rings[i].nodes[j].GetLat()+90.0)*conversionFactor));
          minLon=std::min(minLon,(uint32_t)round((rings[i].nodes[j].GetLon()+180.0)*conversionFactor));
        }

        writer.Write(minLat);
        writer.Write(minLon);

        for (size_t j=0; j<rings[i].nodes.size(); j++) {
          uint32_t latValue=(uint32_t)round((rings[i].nodes[j].GetLat()+90.0)*conversionFactor);
          uint32_t lonValue=(uint32_t)round((rings[i].nodes[j].GetLon()+180.0)*conversionFactor);

          writer.WriteNumber(latValue-minLat);
          writer.WriteNumber(lonValue-minLon);
        }
      }
    }

    return !writer.HasError();
  }

  bool Area::WriteOptimized(FileWriter& writer) const
  {
    if (!attributes.Write(writer)) {
      return false;
    }

    writer.WriteNumber((uint32_t)rings.size());
    for (size_t i=0; i<rings.size(); i++) {
      if (!rings[i].attributes.Write(writer)) {
        return false;
      }

      writer.Write(rings[i].ring);

      writer.WriteNumber((uint32_t)rings[i].nodes.size());

      if (!rings[i].nodes.empty()) {
        uint32_t minLat=std::numeric_limits<uint32_t>::max();
        uint32_t minLon=std::numeric_limits<uint32_t>::max();

        for (size_t j=0; j<rings[i].nodes.size(); j++) {
          minLat=std::min(minLat,(uint32_t)round((rings[i].nodes[j].GetLat()+90.0)*conversionFactor));
          minLon=std::min(minLon,(uint32_t)round((rings[i].nodes[j].GetLon()+180.0)*conversionFactor));
        }

        writer.Write(minLat);
        writer.Write(minLon);

        for (size_t j=0; j<rings[i].nodes.size(); j++) {
          uint32_t latValue=(uint32_t)round((rings[i].nodes[j].GetLat()+90.0)*conversionFactor);
          uint32_t lonValue=(uint32_t)round((rings[i].nodes[j].GetLon()+180.0)*conversionFactor);

          writer.WriteNumber(latValue-minLat);
          writer.WriteNumber(lonValue-minLon);
        }
      }
    }

    return !writer.HasError();
  }
}

