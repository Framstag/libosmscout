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

#include <osmscout/routing/RouteNode.h>

#include <limits>

#include <osmscout/system/Math.h>

namespace osmscout {

  bool ObjectVariantData::operator==(const ObjectVariantData& other) const
  {
    return type==other.type && maxSpeed==other.maxSpeed && grade==other.grade;
  }

  bool ObjectVariantData::operator<(const ObjectVariantData& other) const
  {
    if (type->GetIndex()!=other.type->GetIndex()) {
      return type->GetIndex()<other.type->GetIndex();
    }

    if (maxSpeed!=other.maxSpeed) {
      return maxSpeed<other.maxSpeed;
    }

    return grade<other.grade;
  }

  void ObjectVariantData::Read(const TypeConfig& typeConfig,
                               FileScanner& scanner)
  {
    uint32_t typeIndex;

    scanner.ReadNumber(typeIndex);

    type=typeConfig.GetTypeInfo(typeIndex);

    scanner.Read(maxSpeed);
    scanner.Read(grade);
  }

  /**
   * Write the data to the given FileWriter.
   *
   * @throws IOException
   */
  void ObjectVariantData::Write(FileWriter& writer) const
  {
    writer.WriteNumber((uint32_t)type->GetIndex());
    writer.Write(maxSpeed);
    writer.Write(grade);
  }

  uint32_t RouteNode::AddObject(const ObjectFileRef& object,
                                uint16_t objectVariantIndex)
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
    data.objectVariantIndex=objectVariantIndex;

    objects.push_back(data);

    return index;
  }


  /**
   * Read data from the given FileScanner
   *
   * @throws IOException
   */
  void RouteNode::Read(FileScanner& scanner)
  {
    uint8_t  serial;
    GeoCoord coord;
    uint32_t objectCount;
    uint32_t pathCount;
    uint32_t excludesCount;

    fileOffset=scanner.GetPos();

    scanner.Read(serial);
    scanner.ReadCoord(coord);

    point.Set(serial,coord);

    scanner.ReadNumber(objectCount);
    scanner.ReadNumber(pathCount);
    scanner.ReadNumber(excludesCount);

    objects.resize(objectCount);

    Id previousFileOffset=0;

    for (size_t i=0; i<objectCount; i++) {
      RefType    type;
      FileOffset fileOffset;

      scanner.ReadNumber(fileOffset);

      if (fileOffset % 2==0) {
        type=refWay;
      }
      else {
        type=refArea;
      }

      fileOffset=fileOffset/2;

      fileOffset+=previousFileOffset;

      objects[i].object.Set(fileOffset,type);

      scanner.Read(objects[i].objectVariantIndex);

      previousFileOffset=fileOffset;
    }

    if (pathCount>0) {
      paths.resize(pathCount);

      for (size_t i=0; i<pathCount; i++) {
        uint32_t distanceValue;

        scanner.ReadFileOffset(paths[i].offset);
        scanner.ReadNumber(paths[i].objectIndex);
        //scanner.Read(paths[i].bearing);
        scanner.Read(paths[i].flags);
        scanner.ReadNumber(distanceValue);

        paths[i].distance=distanceValue/(1000.0*100.0);
      }
    }

    excludes.resize(excludesCount);
    for (size_t i=0; i<excludesCount; i++) {
      scanner.Read(excludes[i].source);
      scanner.ReadNumber(excludes[i].targetIndex);
    }
  }

  /**
   * Read data from the given FileScanner
   *
   * @throws IOException
   */
  void RouteNode::Read(const TypeConfig& /*typeConfig*/,
                       FileScanner& scanner)
  {
    Read(scanner);
  }

  /**
   * Write data to the given FileWriter
   *
   * @throws IOException
   */
  void RouteNode::Write(FileWriter& writer) const
  {
    writer.Write(point.GetSerial());
    writer.WriteCoord(point.GetCoord());

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
      writer.Write(object.objectVariantIndex);

      lastFileOffset=object.object.GetFileOffset();
    }

    if (!paths.empty()) {

      for (const auto& path : paths) {
        writer.WriteFileOffset(path.offset);
        writer.WriteNumber(path.objectIndex);
        //writer.Write(paths[i].bearing);
        writer.Write(path.flags);
        writer.WriteNumber((uint32_t)floor(path.distance*(1000.0*100.0)+0.5));
      }
    }

    for (const auto& exclude : excludes) {
      writer.Write(exclude.source);
      writer.WriteNumber(exclude.targetIndex);
    }
  }
}
