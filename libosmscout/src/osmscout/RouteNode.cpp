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

  uint32_t RouteNode::AddObject(const ObjectFileRef& object)
  {
    uint32_t index=0;

    while (index<objects.size() && objects[index]!=object) {
      index++;
    }

    if (index<objects.size()) {
      return index;
    }

    objects.push_back(object);

    return index;
  }


  bool RouteNode::Read(FileScanner& scanner)
  {
    uint32_t objectCount;
    uint32_t pathCount;
    uint32_t excludesCount;
    uint32_t minLat;
    uint32_t minLon;

    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    scanner.ReadNumber(id);

    scanner.ReadNumber(objectCount);
    scanner.ReadNumber(pathCount);
    scanner.ReadNumber(excludesCount);

    scanner.Read(minLat);
    scanner.Read(minLon);

    if (scanner.HasError()) {
      return false;
    }

    objects.resize(objectCount);

    Id previousFileOffset=0;

    for (size_t i=0; i<objectCount; i++) {
      uint8_t    type;
      FileOffset fileOffset;

      if (!scanner.Read(type)) {
        return false;
      }

      if (!scanner.ReadNumber(fileOffset)) {
        return false;
      }

      fileOffset+=previousFileOffset;

      objects[i].Set(fileOffset,(RefType)type);

      previousFileOffset=fileOffset;
    }

    paths.resize(pathCount);
    for (size_t i=0; i<pathCount; i++) {
      uint32_t latValue;
      uint32_t lonValue;
      uint32_t distanceValue;

      scanner.ReadFileOffset(paths[i].offset);
      scanner.ReadNumber(paths[i].objectIndex);
      scanner.ReadNumber(paths[i].type);
      scanner.Read(paths[i].maxSpeed);
      scanner.Read(paths[i].grade);
      //scanner.Read(paths[i].bearing);
      scanner.Read(paths[i].flags);
      scanner.ReadNumber(distanceValue);
      scanner.ReadNumber(latValue);
      scanner.ReadNumber(lonValue);

      paths[i].distance=distanceValue/(1000.0*100.0);
      paths[i].lat=(latValue+minLat)/conversionFactor-90.0;
      paths[i].lon=(lonValue+minLon)/conversionFactor-180.0;
    }

    excludes.resize(excludesCount);
    for (size_t i=0; i<excludesCount; i++) {
      FileOffset fileOffset;
      uint8_t    typeByte;

      scanner.Read(typeByte);
      scanner.ReadFileOffset(fileOffset);
      scanner.ReadNumber(excludes[i].targetIndex);

      excludes[i].source.Set(fileOffset,(RefType)typeByte);;
    }

    return !scanner.HasError();
  }

  bool RouteNode::Write(FileWriter& writer) const
  {
    writer.WriteNumber(id);

    writer.WriteNumber((uint32_t)objects.size());
    writer.WriteNumber((uint32_t)paths.size());
    writer.WriteNumber((uint32_t)excludes.size());

    uint32_t minLat=std::numeric_limits<uint32_t>::max();
    uint32_t minLon=std::numeric_limits<uint32_t>::max();

    for (size_t i=0; i<paths.size(); i++) {
      minLat=std::min(minLat,(uint32_t)floor((paths[i].lat+90.0)*conversionFactor+0.5));
      minLon=std::min(minLon,(uint32_t)floor((paths[i].lon+180.0)*conversionFactor+0.5));
    }

    writer.Write(minLat);
    writer.Write(minLon);

    Id lastFileOffset=0;

    for (std::vector<ObjectFileRef>::const_iterator object=objects.begin();
        object!=objects.end();
        ++object) {
      writer.Write((uint8_t)object->GetType());
      writer.WriteNumber(object->GetFileOffset()-lastFileOffset);

      lastFileOffset=object->GetFileOffset();
    }

    for (size_t i=0; i<paths.size(); i++) {
      uint32_t latValue=(uint32_t)floor((paths[i].lat+90.0)*conversionFactor+0.5);
      uint32_t lonValue=(uint32_t)floor((paths[i].lon+180.0)*conversionFactor+0.5);
      uint32_t distanceValue=(uint32_t)floor(paths[i].distance*(1000.0*100.0)+0.5);

      writer.WriteFileOffset(paths[i].offset);
      writer.WriteNumber(paths[i].objectIndex);
      writer.WriteNumber(paths[i].type);
      writer.Write(paths[i].maxSpeed);
      writer.Write(paths[i].grade);
      //writer.Write(paths[i].bearing);
      writer.Write(paths[i].flags);
      writer.WriteNumber(distanceValue);
      writer.WriteNumber(latValue-minLat);
      writer.WriteNumber(lonValue-minLon);
    }

    for (size_t i=0; i<excludes.size(); i++) {
      writer.Write((uint8_t)excludes[i].source.GetType());
      writer.WriteFileOffset(excludes[i].source.GetFileOffset());
      writer.WriteNumber(excludes[i].targetIndex);
    }

    return !writer.HasError();
  }
}
