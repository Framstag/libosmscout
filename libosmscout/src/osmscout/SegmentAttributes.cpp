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

    if (isArea) {
      flags|=SegmentAttributes::isArea;
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
        if (!StringToNumber(tag->value.c_str(),layer)) {
          progress.Warning(std::string("Layer tag value '")+tag->value+"' for "+NumberToString(id)+" is not numeric!");
        }
        tag=tags.erase(tag);
      }
      else if (!IsArea() && tag->key==typeConfig.tagBridge) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=SegmentAttributes::isBridge;
        }
        tag=tags.erase(tag);
      }
      else if (!IsArea() && tag->key==typeConfig.tagTunnel) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=SegmentAttributes::isTunnel;
        }
        tag=tags.erase(tag);
      }
      else if (!IsArea() && tag->key==typeConfig.tagOneway) {
        if (tag->value=="-1") {
          reverseNodes=true;
          flags|=SegmentAttributes::isOneway;
        }
        else if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=SegmentAttributes::isOneway;
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
