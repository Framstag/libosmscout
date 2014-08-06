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

#include <osmscout/import/RawNode.h>

#include <osmscout/system/Math.h>

#include <iostream>
namespace osmscout {

  RawNode::RawNode()
  : id(0)
  {
    // no code
  }

  RawNode::~RawNode()
  {
    // no code
  }

  void RawNode::SetId(OSMId id)
  {
    this->id=id;
  }

  void RawNode::SetType(const TypeInfoRef& type)
  {
    assert(type.Valid());

    featureValueBuffer.SetType(type);
  }

  void RawNode::SetCoords(double lon, double lat)
  {
    coords.Set(lat,lon);
  }

  void RawNode::UnsetFeature(size_t idx)
  {
    featureValueBuffer.FreeValue(idx);
  }

  void RawNode::Parse(Progress& progress,
                      const TypeConfig& typeConfig,
                      const std::map<TagId,std::string>& tags)
  {
    ObjectOSMRef object(id,
                        osmRefNode);

    featureValueBuffer.Parse(progress,
                             typeConfig,
                             object,
                             tags);
  }

  bool RawNode::Read(const TypeConfig& typeConfig,
                     FileScanner& scanner)
  {
    if (!scanner.ReadNumber(id)) {
      return false;
    }

    uint32_t tmpType;

    if (!scanner.ReadNumber(tmpType)) {
      return false;
    }

    TypeInfoRef type=typeConfig.GetTypeInfo((TypeId)tmpType);

    featureValueBuffer.SetType(type);

    if (!scanner.ReadCoord(coords)) {
      return false;
    }

    if (!featureValueBuffer.Read(scanner)) {
      return false;
    }

    return !scanner.HasError();
  }

  bool RawNode::Write(const TypeConfig& /*typeConfig*/,
                      FileWriter& writer) const
  {
    writer.WriteNumber(id);

    writer.WriteNumber(featureValueBuffer.GetTypeId());
    writer.WriteCoord(coords);

    if (!featureValueBuffer.Write(writer)) {
      return false;
    }

    return !writer.HasError();
  }
}

