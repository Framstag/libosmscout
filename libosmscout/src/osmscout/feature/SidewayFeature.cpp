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

#include <osmscout/feature/SidewayFeature.h>

namespace osmscout {

  void SidewayFeatureValue::Read(FileScanner& scanner)
  {
    featureSet=scanner.ReadUInt8();
  }

  void SidewayFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(featureSet);
  }

  SidewayFeatureValue& SidewayFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const SidewayFeatureValue&>(other);

      featureSet=otherValue.featureSet;
    }

    return *this;
  }

  bool SidewayFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const SidewayFeatureValue&>(other);

    return featureSet==otherValue.featureSet;
  }

  const char* const SidewayFeature::NAME = "Sideway";

  SidewayFeature::SidewayFeature()
  {
    RegisterFlag((size_t)FeatureFlags::sidewalkTrackLeft,"sidewalkTrackLeft");
    RegisterFlag((size_t)FeatureFlags::sidewalkTrackRight,"sidewalkTrackRight");

    RegisterFlag((size_t)FeatureFlags::cyclewayLaneLeft,"cyclewayLaneLeft");
    RegisterFlag((size_t)FeatureFlags::cyclewayLaneRight,"cyclewayLaneRight");
    RegisterFlag((size_t)FeatureFlags::cyclewayTrackLeft,"cyclewayTrackLeft");
    RegisterFlag((size_t)FeatureFlags::cyclewayTrackRight,"cyclewayTrackRight");
  }

  void SidewayFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagSidewalk=tagRegistry.RegisterTag("sidewalk");
    tagCyclewayLeft=tagRegistry.RegisterTag("cycleway:left");
    tagCyclewayLeftSegregated=tagRegistry.RegisterTag("cycleway:left:segregated");
    tagCyclewayRight=tagRegistry.RegisterTag("cycleway:right");
    tagCyclewayRightSegregated=tagRegistry.RegisterTag("cycleway:right:segregated");
  }

  std::string SidewayFeature::GetName() const
  {
    return NAME;
  }

  size_t SidewayFeature::GetValueAlignment() const
  {
    return alignof(SidewayFeatureValue);
  }

  size_t SidewayFeature::GetValueSize() const
  {
    return sizeof(SidewayFeatureValue);
  }

  FeatureValue* SidewayFeature::AllocateValue(void* buffer)
  {
    return new (buffer) SidewayFeatureValue();
  }

  void SidewayFeature::Parse(TagErrorReporter& /*errorReporter*/,
                             const TagRegistry& /*tagRegistry*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    uint8_t featureSet=0;
    auto sidewalkTag=tags.find(tagSidewalk);
    auto cyclewayLeftTag=tags.find(tagCyclewayLeft);
    auto cyclewayRightTag=tags.find(tagCyclewayRight);
    auto cyclewayLeftSegregatedTag=tags.find(tagCyclewayLeftSegregated);
    auto cyclewayRightSegregatedTag=tags.find(tagCyclewayRightSegregated);

    bool hasSidewalkTrackLeft=sidewalkTag!=tags.end() &&
                              (sidewalkTag->second=="left" ||
                               sidewalkTag->second=="both");

    bool hasSidewalkTrackRight=sidewalkTag!=tags.end() &&
                               (sidewalkTag->second=="right" ||
                                sidewalkTag->second=="both");

    bool hasCyclewayLaneLeft=cyclewayLeftTag!=tags.end() &&
                             (cyclewayLeftTag->second=="lane" ||
                              cyclewayLeftTag->second=="shared_lane");

    bool hasCyclewayLaneRight=cyclewayRightTag!=tags.end() &&
                              (cyclewayRightTag->second=="lane" ||
                               cyclewayRightTag->second=="shared_lane");

    bool hasCyclewayTrackLeft=cyclewayLeftTag!=tags.end() &&
                              cyclewayLeftTag->second=="track";

    bool hasCyclewayTrackRight=cyclewayRightTag!=tags.end() &&
                               cyclewayRightTag->second=="track";

    bool hasCyclewayLeftSegregated=cyclewayLeftSegregatedTag!=tags.end();
    bool hasCyclewayRightSegregated=cyclewayRightSegregatedTag!=tags.end();

    if (hasSidewalkTrackLeft) {
      featureSet|=SidewayFeatureValue::sidewalkTrackLeft;
    }
    if (hasSidewalkTrackRight) {
      featureSet|=SidewayFeatureValue::sidewalkTrackRight;
    }

    if (hasCyclewayLaneLeft) {
      featureSet|=SidewayFeatureValue::cyclewayLaneLeft;
    }
    if (hasCyclewayLaneRight) {
      featureSet|=SidewayFeatureValue::cyclewayLaneRight;
    }

    if (hasCyclewayTrackLeft) {
      featureSet|=SidewayFeatureValue::cyclewayTrackLeft;

      if (hasCyclewayLeftSegregated) {
        featureSet|=SidewayFeatureValue::sidewalkTrackLeft;
      }
    }
    if (hasCyclewayTrackRight) {
      featureSet|=SidewayFeatureValue::cyclewayTrackRight;

      if (hasCyclewayRightSegregated) {
        featureSet|=SidewayFeatureValue::sidewalkTrackRight;
      }
    }

    if (featureSet!=0) {
      auto* value=static_cast<SidewayFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetFeatureSet(featureSet);
    }
  }
}
