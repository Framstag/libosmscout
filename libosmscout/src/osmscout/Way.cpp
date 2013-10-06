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

  bool WayAttributes::SetTags(Progress& progress,
                              const TypeConfig& typeConfig,
                              Id id,
                              std::vector<Tag>& tags)
  {
    uint32_t namePriority=0;
    uint32_t nameAltPriority=0;
    bool     hasGrade=false;


    flags=0;
    name.clear();
    nameAlt.clear();
    ref.clear();
    houseNr.clear();
    layer=0;
    width=0;
    maxSpeed=0;
    grade=1;

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
      else if (tag->key==typeConfig.tagRef) {
        ref=tag->value;
        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagHouseNr) {
        houseNr=tag->value;
        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagLayer) {
        if (!StringToNumber(tag->value,layer)) {
          progress.Warning(std::string("Layer tag value '")+tag->value+"' for "+NumberToString(id)+" is not numeric!");
        }
        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagMaxSpeed) {
        std::string valueString=tag->value;
        size_t      value;
        bool        isMph=false;

        if (valueString=="signals") {
          tag=tags.erase(tag);
          continue;
        }

        if (valueString=="none") {
          tag=tags.erase(tag);
          continue;
        }

        // "walk" should not be used, but we provide an estimation anyway,
        // since it is likely still better than the default
        if (valueString=="walk") {
          maxSpeed=10;

          tag=tags.erase(tag);
          continue;
        }

        size_t pos;

        pos=valueString.rfind("mph");
        if (pos!=std::string::npos) {
          valueString.erase(pos);
          isMph=true;
        }

        while (valueString.length()>0 && valueString[valueString.length()-1]==' ') {
          valueString.erase(valueString.length()-1);
        }

        if (StringToNumber(valueString,value)) {
          if (isMph) {
            if (value>std::numeric_limits<uint8_t>::max()/1.609+0.5) {
              maxSpeed=std::numeric_limits<uint8_t>::max();
            }
            else {
              maxSpeed=(uint8_t)(value*1.609+0.5);
            }
          }
          else {
            if (value>std::numeric_limits<uint8_t>::max()) {
              maxSpeed=std::numeric_limits<uint8_t>::max();
            }
            else {
              maxSpeed=value;
            }
          }
        }
        else {
          progress.Warning(std::string("Max speed tag value '")+tag->value+"' for "+NumberToString(id)+" is not numeric!");
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagSurface) {
        if (!hasGrade) {
          size_t grade;

          if (typeConfig.GetGradeForSurface(tag->value,
                                            grade)) {
            this->grade=(uint8_t)grade;
          }
          else {
            progress.Warning(std::string("Unknown surface type '")+tag->value+"' for "+NumberToString(id)+"!");
          }
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagTracktype) {
        if (tag->value=="grade1") {
          grade=1;
          hasGrade=true;
        }
        else if (tag->value=="grade2") {
          grade=2;
          hasGrade=true;
        }
        else if (tag->value=="grade3") {
          grade=3;
          hasGrade=true;
        }
        else if (tag->value=="grade4") {
          grade=4;
          hasGrade=true;
        }
        else if (tag->value=="grade5") {
          grade=5;
          hasGrade=true;
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagBridge) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=isBridge;
        }
        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagTunnel) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=isTunnel;
        }
        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagAccess) {
        if (tag->value=="no" ||
            tag->value=="private" ||
            tag->value=="destination" ||
            tag->value=="delivery") {
          flags&=~hasAccess;
        }

        ++tag;
        //tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagJunction) {
        if (tag->value=="roundabout") {
          flags|=isRoundabout;
          // If it is a roundabout is cannot be a area
        }

        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagWidth) {
        double w;
        size_t pos=0;
        size_t count=0;

        // We expect that float values use '.' as separator, but many values use ',' instead.
        // Try try fix this if string looks reasonable
        for (size_t i=0; i<tag->value.length() && count<=1; i++) {
          if (tag->value[i]==',') {
            pos=i;
            count++;
          }
        }

        if (count==1) {
          tag->value[pos]='.';
        }

        // Some width tagvalues add an 'm' to hint that the unit is meter, remove it.
        if (tag->value.length()>=2) {
          if (tag->value[tag->value.length()-1]=='m' &&
              ((tag->value[tag->value.length()-2]>='0' &&
                tag->value[tag->value.length()-2]<='9') ||
                tag->value[tag->value.length()-2]<=' ')) {
            tag->value.erase(tag->value.length()-1);
          }

          // Trim possible trailing spaces
          while (tag->value.length()>0 &&
                 tag->value[tag->value.length()-1]==' ') {
            tag->value.erase(tag->value.length()-1);
          }
        }

        if (!StringToNumber(tag->value,w)) {
          progress.Warning(std::string("Width tag value '")+tag->value+"' for "+NumberToString(id)+" is no double!");
        }
        else if (w<0 && w>255.5) {
          progress.Warning(std::string("Width tag value '")+tag->value+"' for "+NumberToString(id)+" value is too small or too big!");
        }
        else {
          width=(uint8_t)floor(w+0.5);
        }

        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }

    access.Parse(progress,
                 typeConfig,
                 type,
                 id,
                 tags);

    this->tags=tags;

    return true;
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

    if (flags & hasHouseNr) {
      scanner.Read(houseNr);
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

    if (!houseNr.empty()) {
      flags|=hasHouseNr;
    }
    else {
      flags&=~hasHouseNr;
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

    if (flags & hasHouseNr) {
      writer.Write(houseNr);
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

    if ((flags & (hasAccess | isBridge | isTunnel | isRoundabout))!=
        (other.flags & (hasAccess | isBridge | isTunnel | isRoundabout))) {
      return false;
    }

    if (access!=other.access ||
        name!=other.name ||
        nameAlt!=other.nameAlt ||
        ref!=other.ref ||
        houseNr!=other.houseNr ||
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
    attributes.type=type;
  }

  bool Way::SetTags(Progress& progress,
                    const TypeConfig& typeConfig,
                    Id id,
                    std::vector<Tag>& tags)
  {
    return attributes.SetTags(progress,
                              typeConfig,
                              id,
                              tags);
  }

  void Way::SetLayerToMax()
  {
    attributes.SetLayer(std::numeric_limits<int8_t>::max());
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

    uint32_t minLat;
    uint32_t minLon;

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

    ids.resize(nodeCount,0);

    uint32_t idCount;

    scanner.ReadNumber(idCount);

    if (idCount>0) {
      Id  minId;

      scanner.ReadNumber(minId);

      for (size_t i=1; i<=idCount; i++) {
        uint32_t index;
        Id       id;

        scanner.ReadNumber(index);
        scanner.ReadNumber(id);

        ids[index]=id+minId;
      }
    }

    return !scanner.HasError();
  }

  bool Way::ReadOptimized(FileScanner& scanner)
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

    uint32_t minLat;
    uint32_t minLon;

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

    double   minLat=nodes[0].GetLat();
    double   minLon=nodes[0].GetLon();
    uint32_t minLatValue;
    uint32_t minLonValue;

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

    uint32_t idCount=0;
    Id       minId;

    for (size_t i=0; i<ids.size(); i++) {
      if (ids[i]!=0) {
        if (idCount==0) {
          minId=ids[i];
        }
        else {
          minId=std::min(minId,ids[i]);
        }

        idCount++;
      }
    }

    writer.WriteNumber(idCount);

    if (idCount>0) {
      writer.WriteNumber(minId);

      for (size_t i=0; i<ids.size(); i++) {
        if (ids[i]!=0) {
          writer.WriteNumber((uint32_t)i);
          writer.WriteNumber(ids[i]-minId);
        }
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

    writer.WriteNumber((uint32_t)nodes.size());

    double minLat=nodes[0].GetLat();
    double minLon=nodes[0].GetLon();

    for (size_t i=1; i<nodes.size(); i++) {
      minLat=std::min(minLat,nodes[i].GetLat());
      minLon=std::min(minLon,nodes[i].GetLon());
    }

    uint32_t minLatValue=(uint32_t)round((minLat+90.0)*conversionFactor);
    uint32_t minLonValue=(uint32_t)round((minLon+180.0)*conversionFactor);

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
