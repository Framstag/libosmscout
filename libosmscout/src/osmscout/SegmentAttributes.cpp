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

#include <osmscout/SegmentAttributes.h>

#include <cassert>
#include <limits>

#include <osmscout/util/String.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  bool SegmentAttributes::SetTags(Progress& progress,
                                  const TypeConfig& typeConfig,
                                  Id id,
                                  bool isArea,
                                  std::vector<Tag>& tags,
                                  bool& reverseNodes)
  {
    name.clear();
    ref.clear();
    flags=0;
    layer=0;
    width=0;
    reverseNodes=false;

    flags|=hasAccess;

    if (isArea) {
      flags|=isArea;
    }

    std::vector<Tag>::iterator tag=tags.begin();
    while (tag!=tags.end()) {
      if (tag->key==typeConfig.tagName) {
        name=tag->value;
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
            maxSpeed=(uint8_t)(value*1.609+0.5);
          }

          if (value>std::numeric_limits<uint8_t>::max()) {
            maxSpeed=std::numeric_limits<uint8_t>::max();
          }
          else {
            maxSpeed=(uint8_t)value;
          }
        }
        else {
          progress.Warning(std::string("Max speed tag value '")+tag->value+"' for "+NumberToString(id)+" is not numeric!");
        }

        tag=tags.erase(tag);
      }
      else if (!IsArea() && tag->key==typeConfig.tagBridge) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=isBridge;
        }
        tag=tags.erase(tag);
      }
      else if (!IsArea() && tag->key==typeConfig.tagTunnel) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=isTunnel;
        }
        tag=tags.erase(tag);
      }
      else if (!IsArea() && tag->key==typeConfig.tagOneway) {
        if (tag->value=="-1") {
          reverseNodes=true;
          flags|=isOneway;
        }
        else if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=isOneway;
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

        tag=tags.erase(tag);
      }
      else if (!IsArea() && tag->key==typeConfig.tagJunction) {
        if (tag->value=="roundabout") {
          flags|=isOneway;
          flags|=isRoundabout;
          // If it is a roundabout is cannot be a area
          flags&=~isArea;
        }

        tag=tags.erase(tag);
      }
      else if (!IsArea() && tag->key==typeConfig.tagWidth) {
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

    this->tags=tags;

    return true;
  }

  bool SegmentAttributes::Read(FileScanner& scanner)
  {
    uint16_t flags;

    scanner.ReadNumber(type);
    scanner.Read(flags);

    if (scanner.HasError()) {
      return false;
    }

    this->flags=flags;

    if (flags & hasName) {
      scanner.Read(name);
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

  bool SegmentAttributes::Write(FileWriter& writer) const
  {
    writer.WriteNumber(type);

    if (!name.empty()) {
      flags|=hasName;
    }
    else {
      flags&=~hasName;
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

    if (flags & hasTags) {
      writer.WriteNumber((uint32_t)tags.size());

      for (size_t i=0; i<tags.size(); i++) {
        writer.WriteNumber(tags[i].key);
        writer.Write(tags[i].value);
      }
    }

    return !writer.HasError();
  }
}
