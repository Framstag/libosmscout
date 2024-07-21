/*
This source is part of the libosmscout library
Copyright (C) 2024  Tim Teulings

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

#include <osmscout/feature/FeeFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void FeeFeatureValue::Read(FileScanner& scanner)
  {
    fee=static_cast<Fee>(scanner.ReadUInt8());
    condition=scanner.ReadString();
  }

  void FeeFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(static_cast<uint8_t>(fee));
    writer.Write(condition);
  }

  FeeFeatureValue& FeeFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const FeeFeatureValue&>(other);

      fee=otherValue.fee;
      condition=otherValue.condition;
    }

    return *this;
  }

  bool FeeFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const FeeFeatureValue&>(other);

    return fee==otherValue.fee &&
           condition==otherValue.condition;
  }

  const char* const FeeFeature::NAME = "Fee";

  void FeeFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagFee=tagRegistry.RegisterTag("fee");
    tagFeeCondition=tagRegistry.RegisterTag("fee:condition");

    feeMap={
            {"no", FeeFeatureValue::Fee::No},
            {"yes", FeeFeatureValue::Fee::Yes},
            {"unknown", FeeFeatureValue::Fee::Unknown},
            {"donation", FeeFeatureValue::Fee::Donation},
    };
  }

  std::string FeeFeature::GetName() const
  {
    return NAME;
  }

  size_t FeeFeature::GetValueAlignment() const
  {
    return alignof(FeeFeatureValue);
  }

  size_t FeeFeature::GetValueSize() const
  {
    return sizeof(FeeFeatureValue);
  }

  FeatureValue* FeeFeature::AllocateValue(void* buffer)
  {
    return new (buffer) FeeFeatureValue();
  }

  void FeeFeature::Parse(TagErrorReporter& errorReporter,
                           const TagRegistry& /*tagRegistry*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    auto feeTag=tags.find(tagFee);

    if (feeTag!=tags.end() &&
        !feeTag->second.empty()) {
      auto feeEnumValue=feeMap.find(feeTag->second);

      if (feeEnumValue==feeMap.end()) {
        errorReporter.ReportTag(object,tags,"Unknown value for fee");
        return;
      }

      auto* value=static_cast<FeeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetValue(feeEnumValue->second);

      auto feeTagConditionTag=tags.find(tagFeeCondition);

      if (feeTagConditionTag!=tags.end() &&
          !feeTagConditionTag->second.empty()) {
        value->SetCondition(feeTagConditionTag->second);
      }
    }
  }

  const char* EnumToString(const FeeFeatureValue::Fee& value)
  {
    switch (value) {
      case FeeFeatureValue::Fee::Unknown:
        return "Unknown";
      case FeeFeatureValue::Fee::Yes:
        return "Yes";
      case FeeFeatureValue::Fee::No:
        return "No";
      case FeeFeatureValue::Fee::Donation:
        return "Donation";
    }

    return "???";
  }

}
