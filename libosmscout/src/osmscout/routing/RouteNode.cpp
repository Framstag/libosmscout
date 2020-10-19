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
    uint32_t typeIndex=scanner.ReadUInt32Number();

    type=typeConfig.GetTypeInfo(typeIndex);

    maxSpeed=scanner.ReadUInt8();
    grade=scanner.ReadUInt8();
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

  uint8_t RouteNode::AddObject(const ObjectFileRef& object,
                                uint16_t objectVariantIndex)
  {
    size_t index=0;

    while (index<objects.size() &&
           objects[index].object!=object) {
      index++;
    }

    assert(index<=std::numeric_limits<uint8_t>::max());

    if (index<objects.size()) {
      return (uint8_t)index;
    }

    ObjectData data;

    data.object=object;
    data.objectVariantIndex=objectVariantIndex;

    objects.push_back(data);

    return (uint8_t)index;
  }


  /**
   * Read data from the given FileScanner
   *
   * @throws IOException
   */
  void RouteNode::Read(FileScanner& scanner)
  {
    fileOffset=scanner.GetPos();

    uint8_t  serial=scanner.ReadUInt8();
    GeoCoord coord=scanner.ReadCoord();

    point.Set(serial,coord);

    uint8_t objectCount=scanner.ReadUInt8();
    uint8_t pathCount=scanner.ReadUInt8();
    uint8_t excludesCount=scanner.ReadUInt8();

    objects.resize(objectCount);

    Id previousFileOffset=0;

    for (auto& object : objects) {
      RefType    type;
      FileOffset objectFileOffset=scanner.ReadUInt64Number();

      if (objectFileOffset%2==0) {
        type=refWay;
      }
      else {
        type=refArea;
      }

      objectFileOffset=objectFileOffset/2;

      objectFileOffset+=previousFileOffset;

      object.object.Set(objectFileOffset,type);

      object.objectVariantIndex=scanner.ReadUInt16();

      previousFileOffset=objectFileOffset;
    }

    paths.resize(pathCount);

    for (auto& path : paths) {
      path.id=scanner.ReadUInt64();
      path.objectIndex=scanner.ReadUInt8();
      //scanner.Read(paths[i].bearing);
      path.flags=scanner.ReadUInt8();

      uint32_t distanceValue=scanner.ReadUInt32Number();

      path.distance=Distance::Of<Kilometer>(distanceValue/(1000.0*100.0));
    }

    excludes.resize(excludesCount);
    for (auto& exclude: excludes) {
      exclude.source=scanner.ReadObjectFileRef();
      exclude.targetIndex=scanner.ReadUInt8();
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

    assert(paths.size()<=std::numeric_limits<uint8_t>::max());
    assert(excludes.size()<=std::numeric_limits<uint8_t>::max());

    writer.Write((uint8_t)objects.size());
    writer.Write((uint8_t)paths.size());
    writer.Write((uint8_t)excludes.size());

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

    for (const auto& path : paths) {
      writer.Write(path.id);
      writer.Write(path.objectIndex);
      //writer.Write(paths[i].bearing);
      writer.Write(path.flags);
      writer.WriteNumber((uint32_t)floor(path.distance.As<Kilometer>()*(1000.0*100.0)+0.5));
    }

    for (const auto& exclude : excludes) {
      writer.Write(exclude.source);
      writer.Write(exclude.targetIndex);
    }
  }
}
