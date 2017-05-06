/*
  This source is part of the libosmscout library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscout/LabelProvider.h>

#include <osmscout/system/Assert.h>

namespace osmscout {

  LabelProvider::~LabelProvider()
  {
    // No code
  }

  LabelProviderFactory::~LabelProviderFactory()
  {
    // no code
  }

  DynamicFeatureLabelReader::DynamicFeatureLabelReader(const TypeConfig& typeConfig,
                                                       const std::string& featureName,
                                                       const std::string& labelName)
  {
    FeatureRef feature=typeConfig.GetFeature(featureName);
    size_t     labelIndex;

    assert(feature);
    assert(feature->HasLabel());

    feature->GetLabelIndex(labelName,
                           labelIndex);

    this->featureName=featureName;
    this->labelName=labelName;
    this->labelIndex=labelIndex;

    lookupTable.resize(typeConfig.GetTypeCount(),
                       std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(featureName,
                          index)) {
        lookupTable[type->GetIndex()]=index;
      }
    }
  }

  INameLabelProviderFactory::INameLabelProvider::INameLabelProvider(const TypeConfig& typeConfig)
  {
    nameLookupTable.resize(typeConfig.GetTypeCount(),
                           std::numeric_limits<size_t>::max());
    nameAltLookupTable.resize(typeConfig.GetTypeCount(),
                              std::numeric_limits<size_t>::max());

    for (const auto &type : typeConfig.GetTypes()) {
      size_t index;

      if (type->GetFeature(NameFeature::NAME,
                          index)) {
        nameLookupTable[type->GetIndex()]=index;
      }

      if (type->GetFeature(NameAltFeature::NAME,
                          index)) {
        nameAltLookupTable[type->GetIndex()]=index;
      }
    }
  }

  std::string INameLabelProviderFactory::INameLabelProvider::GetLabel(const MapParameter& parameter,
                                                                      const FeatureValueBuffer& buffer) const
  {
    if (parameter.GetShowAltLanguage()) {
      size_t index=nameAltLookupTable[buffer.GetType()->GetIndex()];

      if (index!=std::numeric_limits<size_t>::max() &&
          buffer.HasFeature(index)) {
        FeatureValue *value=buffer.GetValue(index);

        if (value!=NULL) {
          return value->GetLabel();
        }
      }

      index=nameLookupTable[buffer.GetType()->GetIndex()];

      if (index!=std::numeric_limits<size_t>::max() &&
          buffer.HasFeature(index)) {
        FeatureValue *value=buffer.GetValue(index);

        if (value!=NULL) {
          return value->GetLabel();
        }
      }

      return "";
    }
    else {
      size_t index=nameLookupTable[buffer.GetType()->GetIndex()];

      if (index!=std::numeric_limits<size_t>::max() &&
          buffer.HasFeature(index)) {
        FeatureValue *value=buffer.GetValue(index);

        if (value!=NULL) {
          return value->GetLabel();
        }
      }

      return "";
    }

  }

  LabelProviderRef INameLabelProviderFactory::Create(const TypeConfig& typeConfig) const
  {
    if (!instance) {
      instance=std::make_shared<INameLabelProvider>(typeConfig);
    }

    return instance;
  }

  std::string DynamicFeatureLabelReader::GetLabel(const MapParameter& /*parameter*/,
                                                  const FeatureValueBuffer& buffer) const
  {
    size_t index=lookupTable[buffer.GetType()->GetIndex()];

    if (index!=std::numeric_limits<size_t>::max() &&
        buffer.HasFeature(index)) {
      FeatureValue *value=buffer.GetValue(index);

      if (value!=NULL) {
        return value->GetLabel();
      }
    }

    return "";
  }
}

