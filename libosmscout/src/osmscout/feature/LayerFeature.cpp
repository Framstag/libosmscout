/*
This source is part of the libosmscout library
Copyright (C) 2014  Tim Teulings

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

#include <osmscout/feature/LayerFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void LayerFeatureValue::Read(FileScanner& scanner)
  {
    layer=scanner.ReadInt8();
  }

  void LayerFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(layer);
  }

  LayerFeatureValue& LayerFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const LayerFeatureValue&>(other);

      layer=otherValue.layer;
    }

    return *this;
  }

  bool LayerFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const LayerFeatureValue&>(other);

    return layer==otherValue.layer;
  }

  const char* const LayerFeature::NAME = "Layer";

  void LayerFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagLayer=tagRegistry.RegisterTag("layer");
  }

  std::string LayerFeature::GetName() const
  {
    return NAME;
  }

  size_t LayerFeature::GetValueAlignment() const
  {
    return alignof(LayerFeatureValue);
  }

  size_t LayerFeature::GetValueSize() const
  {
    return sizeof(LayerFeatureValue);
  }

  FeatureValue* LayerFeature::AllocateValue(void* buffer)
  {
    return new (buffer) LayerFeatureValue();
  }

  void LayerFeature::Parse(TagErrorReporter& errorReporter,
                           const TagRegistry& /*tagRegistry*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    auto layer=tags.find(tagLayer);

    if (layer!=tags.end()) {
      int8_t layerValue;

      if (StringToNumber(layer->second,layerValue)) {
        if (layerValue!=0) {
          auto* value=static_cast<LayerFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

          value->SetLayer(layerValue);
        }
      }
      else {
        errorReporter.ReportTag(object,tags,std::string("Layer tag value '")+layer->second+"' is not numeric!");
      }
    }
  }
}
