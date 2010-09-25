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
#include <cmath>
#include <limits>

#include <osmscout/Util.h>

namespace osmscout {

  bool SegmentAttributes::Assign(Progress& progress,
                                 Id id,
                                 TypeId type,
                                 bool isArea,
                                 std::vector<Tag>& tags,
                                 bool& reverseNodes)
  {
    bool isOneway=false;

    this->type=type;
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
      if (tag->key==tagName) {
        name=tag->value;
        if (!name.empty()) {
          flags|=SegmentAttributes::hasName;
        }
        tag=tags.erase(tag);
      }
      else if (tag->key==tagRef) {
        ref=tag->value;
        if (!ref.empty()) {
          flags|=SegmentAttributes::hasRef;
        }
        tag=tags.erase(tag);
      }
      else if (tag->key==tagLayer) {
        if (!StringToNumber(tag->value.c_str(),layer)) {
          progress.Warning(std::string("Layer tag value '")+tag->value+"' for "+NumberToString(id)+" is not numeric!");
        }
        else if (layer!=0) {
          flags|=SegmentAttributes::hasLayer;
        }
        tag=tags.erase(tag);
      }
      else if (!isArea && tag->key==tagBridge) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=SegmentAttributes::isBridge;
        }
        tag=tags.erase(tag);
      }
      else if (!isArea && tag->key==tagTunnel) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=SegmentAttributes::isTunnel;
        }
        tag=tags.erase(tag);
      }
      else if (isArea && tag->key==tagBuilding) {
        if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=SegmentAttributes::isBuilding;
        }

        tag=tags.erase(tag);
      }
      else if (!isArea && tag->key==tagOneway) {
        if (tag->value=="-1") {
          isOneway=true;
          reverseNodes=true;
          flags|=SegmentAttributes::isOneway;
        }
        else if (!(tag->value=="no" || tag->value=="false" || tag->value=="0")) {
          flags|=SegmentAttributes::isOneway;
        }

        tag=tags.erase(tag);
      }
      else if (!isArea && tag->key==tagWidth) {
        double w;

        if (!StringToNumber(tag->value,w)) {
          progress.Warning(std::string("Width tag value '")+tag->value+"' for "+NumberToString(id)+" is no double!");
        }
        else if (w<0 && w>255.5) {
          progress.Warning(std::string("Width tag value '")+tag->value+"' for "+NumberToString(id)+" value is too small or too big!");
        }
        else {
          width=(uint8_t)floor(w+0.5);
          flags|=SegmentAttributes::hasWidth;
        }
        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }

    // tags

    this->tags=tags;
    if (this->tags.size()>0) {
      flags|=SegmentAttributes::hasTags;
    }

    return true;
  }

  bool SegmentAttributes::Read(FileScanner& scanner)
  {
    uint32_t flags;

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
    writer.Write((uint32_t)flags);

    if (flags & hasName) {
      writer.Write(name);
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
