#ifndef OSMSCOUT_FEATURE_ELE_FEATURE_H
#define OSMSCOUT_FEATURE_ELE_FEATURE_H

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

  class OSMSCOUT_API EleFeatureValue : public FeatureValue
  {
  private:
    int16_t ele=0;

  public:
    EleFeatureValue() = default;

    explicit EleFeatureValue(int16_t ele)
    : ele(ele)
    {
      // no code
    }

    void SetEle(int16_t ele)
    {
      this->ele=ele;
    }

    int16_t GetEle() const
    {
      return ele;
    }

    std::string GetLabel(const Locale &locale, size_t labelIndex) const override;

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    EleFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API EleFeature : public Feature
  {
  private:
    TagId tagEle=0;

  public:
    /** Name of this feature */
    static const char* const NAME;

    /** Name of the "inMeter" label */
    static const char* const IN_METER_LABEL;

    /** Index of the 'inMeter' label */
    static const size_t      IN_METER_LABEL_INDEX;

    /** Name of the "inFeet" label */
    static const char* const IN_FEET_LABEL;

    /** Index of the 'inFeet' label */
    static const size_t      IN_FEET_LABEL_INDEX;

    /** Name of the "inLocaleUnit" label */
    static const char* const IN_LOCALE_UNIT_LABEL;

    /** Index of the 'inLocaleUnit' label */
    static const size_t      IN_LOCALE_UNIT_LABEL_INDEX;

  public:
    EleFeature();
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

  using EleFeatureValueReader = FeatureValueReader<EleFeature, EleFeatureValue>;

}

#endif
