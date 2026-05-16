#ifndef OSMSCOUT_FEATURE_HIGHWAY_MILESTONE_FEATURE_H
#define OSMSCOUT_FEATURE_HIGHWAY_MILESTONE_FEATURE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2025  Tim Teulings

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

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeature.h>

#include <osmscout/FeatureReader.h>

namespace osmscout {

  class OSMSCOUT_API HighwayMilestoneFeatureValue : public FeatureValue
  {
  private:
    uint32_t  distance=0;
    std::string ref;
    std::string carriagewayRef;
    std::string marker;

  public:
    HighwayMilestoneFeatureValue() = default;
    HighwayMilestoneFeatureValue(const HighwayMilestoneFeatureValue& featureValue) = default;

    explicit HighwayMilestoneFeatureValue(uint32_t distance,
                                          const std::string& ref)
      : distance(distance),
        ref(ref)
    {
      // no code
    }

    void SetDistance(uint32_t distance)
    {
      this->distance=distance;
    }

    uint32_t GetDistance() const
    {
      return distance;
    }

    void SetRef(const std::string_view& ref)
    {
      this->ref=ref;
    }

    std::string GetRef() const
    {
      return ref;
    }

    void SetCarriagewayRef(const std::string_view& carriagewayRef)
    {
      this->carriagewayRef=carriagewayRef;
    }

    std::string GetCarriagewayRef() const
    {
      return carriagewayRef;
    }

    void SetMarker(const std::string_view& marker)
    {
      this->marker=marker;
    }

    std::string GetMarker() const
    {
      return marker;
    }

    std::string GetLabel(const Locale &locale, size_t labelIndex) const override;

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    HighwayMilestoneFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  /**
   * The highway milestone feature stores OSM highway=milestone data (distance, ref,
   * carriageway_ref, marker) for node objects.
   */
  class OSMSCOUT_API HighwayMilestoneFeature : public Feature
  {
  private:
    TagId tagDistance=0;
    TagId tagRef=0;
    TagId tagCarriagewayRef=0;
    TagId tagMarker=0;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    void Initialize(TagRegistry& tagRegistry) override;

    std::string GetName() const override;

    size_t GetValueAlignment() const override;
    size_t GetValueSize() const override;
    FeatureValue* AllocateValue(void* buffer) override;

    void Parse(TagErrorReporter& reporter,
               const TagRegistry& tagRegistry,
               const FeatureInstance& feature,
               const ObjectOSMRef& object,
               const TagMap& tags,
               FeatureValueBuffer& buffer) const override;
  };

  using HighwayMilestoneFeatureValueReader = FeatureValueReader<HighwayMilestoneFeature, HighwayMilestoneFeatureValue>;
}

#endif
