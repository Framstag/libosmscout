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

#include <osmscout/Node.h>

namespace osmscout {

  void Node::SetType(const TypeInfoRef& type)
  {
    featureValueBuffer.SetType(type);
  }

  void Node::SetCoords(const GeoCoord& coords)
  {
    this->coords=coords;
  }

  void Node::SetFeatures(const FeatureValueBuffer& buffer)
  {
    featureValueBuffer.Set(buffer);
  }

  bool Node::Read(const TypeConfig& typeConfig,
                  FileScanner& scanner)
  {

    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    TypeId typeId;

    scanner.ReadTypeId(typeId,
                       typeConfig.GetNodeTypeIdBytes());

    TypeInfoRef type=typeConfig.GetNodeTypeInfo(typeId);

    featureValueBuffer.SetType(type);

    if (!featureValueBuffer.Read(scanner)) {
      return false;
    }

    scanner.ReadCoord(coords);

    return !scanner.HasError();
  }

  bool Node::Write(const TypeConfig& typeConfig,
                   FileWriter& writer) const
  {
    writer.WriteTypeId(featureValueBuffer.GetType()->GetNodeId(),
                       typeConfig.GetNodeTypeIdBytes());

    if (!featureValueBuffer.Write(writer)) {
      return false;
    }

    writer.WriteCoord(coords);

    return !writer.HasError();
  }
}

