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

namespace osmscout {

  RawNode::RawNode()
  : id(0)
  {
    // no code
  }

  void RawNode::SetId(OSMId id)
  {
    this->id=id;
  }

  void RawNode::SetType(const TypeInfoRef& type)
  {
    assert(type);

    featureValueBuffer.SetType(type);
  }

  void RawNode::SetCoord(const GeoCoord& coord)
  {
    this->coord=coord;
  }

  void RawNode::UnsetFeature(size_t idx)
  {
    featureValueBuffer.FreeValue(idx);
  }

  void RawNode::Parse(TagErrorReporter& errorReporter,
                      const TypeConfig& typeConfig,
                      const TagMap& tags)
  {
    ObjectOSMRef object(id,
                        osmRefNode);

    featureValueBuffer.Parse(errorReporter,
                             typeConfig,
                             object,
                             tags);
  }

  /**
   * Reads the data from the given FileScanner
   *
   * @throws IOException
   */
  void RawNode::Read(const TypeConfig& typeConfig,
                     FileScanner& scanner)
  {
    scanner.ReadNumber(id);

    TypeId typeId;

    scanner.ReadTypeId(typeId,
                       typeConfig.GetNodeTypeIdBytes());

    TypeInfoRef type=typeConfig.GetNodeTypeInfo(typeId);

    featureValueBuffer.SetType(type);

    if (!type->GetIgnore()) {
      featureValueBuffer.Read(scanner);
    }

    scanner.ReadCoord(coord);
  }

  /**
   * Writes the data to the given FileWriter
   *
   * @throws IOException
   */
  void RawNode::Write(const TypeConfig& typeConfig,
                      FileWriter& writer) const
  {
    writer.WriteNumber(id);

    writer.WriteTypeId(featureValueBuffer.GetType()->GetNodeId(),
                       typeConfig.GetNodeTypeIdBytes());

    if (!featureValueBuffer.GetType()->GetIgnore()) {
      featureValueBuffer.Write(writer);
    }

    writer.WriteCoord(coord);
  }
}

