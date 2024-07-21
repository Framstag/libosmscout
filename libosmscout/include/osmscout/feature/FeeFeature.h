#ifndef OSMSCOUT_FEATURE_FEE_FEATURE_H
#define OSMSCOUT_FEATURE_FEE_FEATURE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2024  Tim Teulings

  This library is Fee software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Fee Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Fee Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <unordered_map>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeature.h>

#include <osmscout/FeatureReader.h>

namespace osmscout {

  class OSMSCOUT_API FeeFeatureValue : public FeatureValue
  {
   public:
    enum class Fee : uint8_t
    {
      Unknown  = 0,
      Yes      = 1,
      No       = 2,
      Donation = 3
    };

  private:
    Fee         fee=Fee::Unknown;
    std::string condition;

  public:
    FeeFeatureValue() = default;

    explicit FeeFeatureValue(Fee fee)
      : fee(fee)
    {
      // no code
    }

    void SetValue(Fee fee)
    {
      this->fee=fee;
    }

    Fee GetValue() const
    {
      return fee;
    }

    void SetCondition(const std::string_view& condition)
    {
      this->condition=condition;
    }

    bool HasCondition() const
    {
      return !condition.empty();
    }

    std::string GetCondition() const
    {
      return condition;
    }

    void Read(FileScanner& scanner) override;
    void Write(FileWriter& writer) override;

    FeeFeatureValue& operator=(const FeatureValue& other) override;
    bool operator==(const FeatureValue& other) const override;
  };

  class OSMSCOUT_API FeeFeature : public Feature
  {
  private:
    TagId tagFee;
    TagId tagFeeCondition;
    std::unordered_map<std::string,FeeFeatureValue::Fee> feeMap;


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

  extern OSMSCOUT_API const char* EnumToString(const FeeFeatureValue::Fee& value);

  using FeeFeatureValueReader = FeatureValueReader<FeeFeature, FeeFeatureValue>;
}

#endif
