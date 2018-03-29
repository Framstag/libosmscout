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

#include <osmscout/import/RawWay.h>

#include <algorithm>
#include <limits>

#include <osmscout/TypeFeatures.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  bool RawWay::IsOneway() const
  {
    for (size_t i=0; i<featureValueBuffer.GetFeatureCount(); i++) {
      if (featureValueBuffer.HasFeature(i) &&
          featureValueBuffer.GetFeature(i).GetFeature()->HasValue()) {
        auto* value=dynamic_cast<AccessFeatureValue*>(featureValueBuffer.GetValue(i));

        if (value!=nullptr) {
          return value->IsOneway();
        }
      }
    }

    return false;
  }

  void RawWay::SetId(OSMId id)
  {
    this->id=id;
  }

  void RawWay::SetType(const TypeInfoRef& type,
                       bool area)
  {
    this->isArea=area;
    featureValueBuffer.SetType(type);
  }

  void RawWay::SetNodes(const std::vector<OSMId>& nodes)
  {
    this->nodes=nodes;
  }

  void RawWay::Parse(TagErrorReporter& errorReporter,
                     const TypeConfig& typeConfig,
                     const TagMap& tags)
  {
    ObjectOSMRef object(id,
                        osmRefWay);

    featureValueBuffer.Parse(errorReporter,
                             typeConfig,
                             object,
                             tags);
  }

  /**
   * Reads data from the given FileScanner
   *
   * @throws IOException
   */
  void RawWay::Read(const TypeConfig& typeConfig,
                    FileScanner& scanner)
  {
    scanner.ReadNumber(id);

    TypeId tmpType;

    scanner.ReadNumber(tmpType);

    if (tmpType>typeConfig.GetMaxTypeId()) {
      isArea=true;
      tmpType=tmpType-typeConfig.GetMaxTypeId()-1;
    }
    else {
      isArea=false;
    }

    TypeInfoRef type;

    if (isArea) {
      type=typeConfig.GetAreaTypeInfo((TypeId)tmpType);
    }
    else {
      type=typeConfig.GetWayTypeInfo((TypeId)tmpType);
    }

    featureValueBuffer.SetType(type);

    if (!type->GetIgnore()) {
      featureValueBuffer.Read(scanner);
    }

    uint32_t nodeCount;

    scanner.ReadNumber(nodeCount);

    nodes.resize(nodeCount);

    if (nodeCount>0) {
      OSMId minId;

      scanner.ReadNumber(minId);

      for (size_t i=0; i<nodeCount; i++) {
        OSMId id;

        scanner.ReadNumber(id);

        nodes[i]=minId+id;
      }
    }
  }

  /**
   * Writes data to the given FileWriter
   *
   * @throws IOException
   */
  void RawWay::Write(const TypeConfig& typeConfig,
                     FileWriter& writer) const
  {
    writer.WriteNumber(id);

    if (isArea) {
      TypeId type=typeConfig.GetMaxTypeId()+1+
                  featureValueBuffer.GetType()->GetAreaId();
      writer.WriteNumber(type);
    }
    else {
      writer.WriteNumber(featureValueBuffer.GetType()->GetWayId());
    }

    if (!featureValueBuffer.GetType()->GetIgnore()) {
      featureValueBuffer.Write(writer);
    }

    writer.WriteNumber((uint32_t)nodes.size());

    if (!nodes.empty()) {
      OSMId minId=std::numeric_limits<OSMId>::max();

      for (OSMId node : nodes) {
        minId=std::min(minId,
                       node);
      }

      writer.WriteNumber(minId);
      for (OSMId node : nodes) {
        writer.WriteNumber(node-minId);
      }
    }
  }
}

