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
  : id(0),
    featureBits(NULL),
    featureValues(NULL)
  {
    // no code
  }

  RawNode::~RawNode()
  {
    DeleteFeatureData();
  }

  void RawNode::DeleteFeatureData()
  {
    if (type.Valid()) {
      if (featureValues!=NULL) {
        for (size_t i=0; i<type->GetFeatureCount(); i++) {
          delete featureValues[i];
        }
      }

      delete [] featureValues;
      delete [] featureBits;
    }
  }

  void RawNode::AllocateFeatureData()
  {
    featureBits=new uint8_t[type->GetFeatureBytes()];

    for (size_t i=0; i<type->GetFeatureBytes(); i++) {
      featureBits[i]=0;
    }

    featureValues=new FeatureValue*[type->GetFeatureCount()];

    for (size_t i=0; i<type->GetFeatureCount(); i++) {
      featureValues[i]=NULL;
    }
  }

  void RawNode::SetId(OSMId id)
  {
    this->id=id;
  }

  void RawNode::SetType(const TypeInfoRef& type)
  {
    assert(type.Valid());

    DeleteFeatureData();

    this->type=type;

    AllocateFeatureData();
  }

  void RawNode::SetCoords(double lon, double lat)
  {
    coords.Set(lat,lon);
  }

  void RawNode::SetFeature(size_t idx,
                           FeatureValue* value)
  {
    size_t byteIdx=idx/8;

    featureBits[byteIdx]=featureBits[byteIdx] | (1 << idx%8);

    delete featureValues[idx];
    featureValues[idx]=value;
  }

  void RawNode::UnsetFeature(size_t idx)
  {
    size_t byteIdx=idx/8;

    featureBits[byteIdx]=featureBits[byteIdx] & ~(1 << idx%8);

    delete featureValues[idx];
    featureValues[idx]=NULL;
  }

  bool RawNode::Read(const TypeConfig& typeConfig,
                     FileScanner& scanner)
  {
    DeleteFeatureData();

    if (!scanner.ReadNumber(id)) {
      return false;
    }

    uint32_t tmpType;

    if (!scanner.ReadNumber(tmpType)) {
      return false;
    }

    type=typeConfig.GetTypeInfo((TypeId)tmpType);

    AllocateFeatureData();

    if (!scanner.ReadCoord(coords)) {
      return false;
    }

    for (size_t i=0; i<type->GetFeatureBytes(); i++) {
      if (!scanner.Read(featureBits[i])) {
        return false;
      }
    }

    size_t featureIdx=0;
    for (auto feature : type->GetFeatures()) {
      if (HashFeature(featureIdx)) {
        FeatureValue* value=NULL;

        if (!feature->Read(scanner,
                           value)) {
          return false;
        }

        featureValues[featureIdx]=value;
      }

      featureIdx++;
    }

    return !scanner.HasError();
  }

  bool RawNode::Write(const TypeConfig& /*typeConfig*/,
                      FileWriter& writer) const
  {
    writer.WriteNumber(id);

    writer.WriteNumber(type->GetId());
    writer.WriteCoord(coords);

    for (size_t i=0; i<type->GetFeatureBytes(); i++) {
      if (!writer.Write(featureBits[i])) {
        return false;
      }
    }

    size_t featureIdx=0;
    for (auto feature : type->GetFeatures()) {
      if (HashFeature(featureIdx)) {
        if (!feature->Write(writer,
                            GetFeatureValue(featureIdx))) {
          return false;
        }
      }

      featureIdx++;
    }

    return !writer.HasError();
  }
}

