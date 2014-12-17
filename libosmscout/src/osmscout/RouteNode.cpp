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

#include <osmscout/RouteNode.h>

#include <limits>

#include <osmscout/system/Math.h>

namespace osmscout {

  uint32_t RouteNode::AddObject(const ObjectFileRef& object,
                                TypeId type,
                                uint8_t maxSpeed,
                                uint8_t grade)
  {
    uint32_t index=0;

    while (index<objects.size() &&
           objects[index].object!=object) {
      index++;
    }

    if (index<objects.size()) {
      return index;
    }

    ObjectData data;

    data.object=object;
    data.type=type;
    data.maxSpeed=maxSpeed;
    data.grade=grade;

    objects.push_back(data);

    return index;
  }


  bool RouteNode::Read(FileScanner& scanner)
  {
    uint32_t objectCount;
    uint32_t pathCount;
    uint32_t excludesCount;

    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    scanner.ReadNumber(id);

    scanner.ReadNumber(objectCount);
    scanner.ReadNumber(pathCount);
    scanner.ReadNumber(excludesCount);

    if (scanner.HasError()) {
      return false;
    }

    objects.resize(objectCount);

    Id previousFileOffset=0;

    for (size_t i=0; i<objectCount; i++) {
      RefType    type;
      FileOffset fileOffset;

      if (!scanner.ReadNumber(fileOffset)) {
        return false;
      }

      if (fileOffset % 2==0) {
        type=refWay;
      }
      else {
        type=refArea;
      }

      fileOffset=fileOffset/2;

      fileOffset+=previousFileOffset;

      objects[i].object.Set(fileOffset,type);

      scanner.ReadNumber(objects[i].type);
      scanner.Read(objects[i].maxSpeed);
      scanner.Read(objects[i].grade);

      previousFileOffset=fileOffset;
    }

    if (pathCount>0) {
      GeoCoord minCoord;

      if (!scanner.ReadCoord(minCoord)) {
        return false;
      }

      paths.resize(pathCount);

      for (size_t i=0; i<pathCount; i++) {
        uint32_t latValue;
        uint32_t lonValue;
        uint32_t distanceValue;

        scanner.ReadFileOffset(paths[i].offset);
        scanner.ReadNumber(paths[i].objectIndex);
        //scanner.Read(paths[i].bearing);
        scanner.Read(paths[i].flags);
        scanner.ReadNumber(distanceValue);
        scanner.ReadNumber(latValue);
        scanner.ReadNumber(lonValue);

        paths[i].distance=distanceValue/(1000.0*100.0);
        paths[i].lat=minCoord.GetLat()+latValue/latConversionFactor;
        paths[i].lon=minCoord.GetLon()+lonValue/lonConversionFactor;
      }
    }

    excludes.resize(excludesCount);
    for (size_t i=0; i<excludesCount; i++) {
      scanner.Read(excludes[i].source);
      scanner.ReadNumber(excludes[i].targetIndex);
    }

    return !scanner.HasError();
  }

  bool RouteNode::Read(const TypeConfig& /*typeConfig*/,
                       FileScanner& scanner)
  {
    return Read(scanner);
  }

  bool RouteNode::Write(FileWriter& writer) const
  {
    writer.WriteNumber(id);

    writer.WriteNumber((uint32_t)objects.size());
    writer.WriteNumber((uint32_t)paths.size());
    writer.WriteNumber((uint32_t)excludes.size());

    Id lastFileOffset=0;

    for (const auto& object : objects) {
      FileOffset offset=object.object.GetFileOffset()-lastFileOffset;

      if (object.object.GetType()==refWay) {
        offset=offset*2;
      }
      else if (object.object.GetType()==refArea) {
        offset=offset*2+1;
      }
      else {
        assert(false);
      }

      writer.WriteNumber(offset);
      writer.WriteNumber(object.type);
      writer.Write(object.maxSpeed);
      writer.Write(object.grade);

      lastFileOffset=object.object.GetFileOffset();
    }

    if (!paths.empty()) {
      GeoCoord minCoord(paths[0].lat,paths[0].lon);

      for (size_t i=1; i<paths.size(); i++) {
        minCoord.Set(std::min(minCoord.GetLat(),paths[i].lat),
                     std::min(minCoord.GetLon(),paths[i].lon));
      }

      writer.WriteCoord(minCoord);

      for (const auto& path : paths) {
        writer.WriteFileOffset(path.offset);
        writer.WriteNumber(path.objectIndex);
        //writer.Write(paths[i].bearing);
        writer.Write(path.flags);
        writer.WriteNumber((uint32_t)floor(path.distance*(1000.0*100.0)+0.5));
        writer.WriteNumber((uint32_t)round((path.lat-minCoord.GetLat())*latConversionFactor));
        writer.WriteNumber((uint32_t)round((path.lon-minCoord.GetLon())*lonConversionFactor));
      }
    }

    for (const auto& exclude : excludes) {
      writer.Write(exclude.source);
      writer.WriteNumber(exclude.targetIndex);
    }

    return !writer.HasError();
  }
}
