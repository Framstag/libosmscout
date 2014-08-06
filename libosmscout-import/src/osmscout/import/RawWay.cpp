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

#include <limits>

#include <osmscout/system/Math.h>
#include <iostream>
namespace osmscout {

  bool RawWay::IsOneway() const
  {
    for (size_t i=0; i<featureValueBuffer.GetFeatureCount(); i++) {
      if (featureValueBuffer.HasValue(i) &&
          featureValueBuffer.GetFeature(i).GetFeature()->HasValue()) {
        AccessFeatureValue* value=dynamic_cast<AccessFeatureValue*>(featureValueBuffer.GetValue(i));

        if (value!=NULL) {
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

  void RawWay::Parse(Progress& progress,
                     const TypeConfig& typeConfig,
                     const std::map<TagId,std::string>& tags)
  {
    ObjectOSMRef object(id,
                        osmRefWay);

    featureValueBuffer.Parse(progress,
                             typeConfig,
                             object,
                             tags);
  }

  bool RawWay::Read(const TypeConfig& typeConfig,
                    FileScanner& scanner)
  {
    if (!scanner.ReadNumber(id)) {
      return false;
    }

    TypeId tmpType;

    if (!scanner.ReadNumber(tmpType)) {
      return false;
    }

    if (tmpType>typeConfig.GetMaxTypeId()) {
      isArea=true;
      tmpType=tmpType-typeConfig.GetMaxTypeId();
    }
    else {
      isArea=false;
    }

    TypeInfoRef type=typeConfig.GetTypeInfo((TypeId)tmpType);

    featureValueBuffer.SetType(type);

    uint32_t nodeCount;

    if (!scanner.ReadNumber(nodeCount)) {
      return false;
    }

    nodes.resize(nodeCount);

    if (nodeCount>0) {
      OSMId minId;

      if (!scanner.ReadNumber(minId)) {
        return false;
      }

      for (size_t i=0; i<nodeCount; i++) {
        OSMId id;

        if (!scanner.ReadNumber(id)) {
          return false;
        }

        nodes[i]=minId+id;
      }
    }

    if (!featureValueBuffer.Read(scanner)) {
      return false;
    }

    return !scanner.HasError();
  }

  bool RawWay::Write(const TypeConfig& typeConfig,
                     FileWriter& writer) const
  {
    writer.WriteNumber(id);

    if (isArea) {
      writer.WriteNumber((TypeId)(typeConfig.GetMaxTypeId()+
                                 featureValueBuffer.GetTypeId()));
    }
    else {
      writer.WriteNumber(featureValueBuffer.GetTypeId());
    }

    writer.WriteNumber((uint32_t)nodes.size());

    if (!nodes.empty()) {
      OSMId minId=std::numeric_limits<OSMId>::max();

      for (size_t i=0; i<nodes.size(); i++) {
        minId=std::min(minId,nodes[i]);
      }

      writer.WriteNumber(minId);
      for (size_t i=0; i<nodes.size(); i++) {
        writer.WriteNumber(nodes[i]-minId);
      }
    }

    if (!featureValueBuffer.Write(writer)) {
      return false;
    }

    return !writer.HasError();
  }
}

