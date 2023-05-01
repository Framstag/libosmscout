#ifndef OSMSCOUT_FEATURE_SIDEWAY_FEATURE_H
#define OSMSCOUT_FEATURE_SIDEWAY_FEATURE_H

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

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeature.h>

#include <osmscout/FeatureReader.h>

namespace osmscout {

  class OSMSCOUT_API SidewayFeatureValue : public FeatureValue
  {
  public:
    enum Feature : uint8_t {
      sidewalkTrackLeft  = 1u << 0u,
      sidewalkTrackRight = 1u << 1u,
      cyclewayLaneLeft   = 1u << 2u,
      cyclewayLaneRight  = 1u << 3u,
      cyclewayTrackLeft  = 1u << 4u,
      cyclewayTrackRight = 1u << 5u,
    };

  private:
    uint8_t featureSet=0;

  public:
    SidewayFeatureValue() = default;

    bool IsFlagSet(size_t flagIndex) const override
    {
      return (featureSet & (1<< flagIndex))!=0;
    }

    void SetFeatureSet(uint8_t featureSet)
    {
      this->featureSet=featureSet;
    }

    bool HasSidewalkTrackLeft() const
    {
      return (featureSet & sidewalkTrackLeft)!=0;
    }

    bool HasSidewalkTrackRight() const
    {
      return (featureSet & sidewalkTrackRight)!=0;
    }

    bool HasCyclewayLaneLeft() const
    {
      return (featureSet & cyclewayLaneLeft)!=0;
    }

    bool HasCyclewayLaneRight() const
    {
      return (featureSet & cyclewayLaneRight)!=0;
    }

    bool HasCyclewayTrackLeft() const
    {
      return (featureSet & cyclewayTrackLeft)!=0;
    }

    bool HasCyclewayTrackRight() const
    {
      return (featureSet & cyclewayTrackRight)!=0;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    SidewayFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API SidewayFeature : public Feature
  {
  private:
    enum class FeatureFlags: uint8_t {
      sidewalkTrackLeft  = 0,
      sidewalkTrackRight = 1,
      cyclewayLaneLeft   = 2,
      cyclewayLaneRight  = 3,
      cyclewayTrackLeft  = 4,
      cyclewayTrackRight = 5
    };

  private:
    TagId tagSidewalk;
    TagId tagCyclewayLeft;
    TagId tagCyclewayLeftSegregated;
    TagId tagCyclewayRight;
    TagId tagCyclewayRightSegregated;

  public:
    /** Name of this feature */
    static const char* const NAME;

  public:
    SidewayFeature();
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
}

#endif
