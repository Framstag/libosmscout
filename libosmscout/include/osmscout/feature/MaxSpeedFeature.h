#ifndef OSMSCOUT_FEATURE_MAX_SPEED_FEATURE_H
#define OSMSCOUT_FEATURE_MAX_SPEED_FEATURE_H

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

  class OSMSCOUT_API MaxSpeedFeatureValue : public FeatureValue
  {
  private:
    uint8_t maxSpeed=0;

  public:
    MaxSpeedFeatureValue() = default;

    explicit MaxSpeedFeatureValue(uint8_t maxSpeed)
      : maxSpeed(maxSpeed)
    {
      // no code
    }

    void SetMaxSpeed(uint8_t maxSpeed)
    {
      this->maxSpeed=maxSpeed;
    }

    uint8_t GetMaxSpeed() const
    {
      return maxSpeed;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    MaxSpeedFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API MaxSpeedFeature : public Feature
  {
  private:
    TagId tagMaxSpeed;
    TagId tagMaxSpeedForward;
    TagId tagMaxSpeedBackward;

  public:
    /** Name of this feature */
    static const char* const NAME;

  private:
    bool GetTagValue(TagErrorReporter& errorReporter,
                     const TagRegistry& tagRegistry,
                     const ObjectOSMRef& object,
                     const TagMap& tags,
                     const std::string& input,
                     uint8_t& speed) const;

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

  using MaxSpeedFeatureValueReader = FeatureValueReader<MaxSpeedFeature, MaxSpeedFeatureValue>;

}

#endif
