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

#include <osmscout/feature/EleFeature.h>

#include <sstream>

#include <osmscout/util/String.h>

namespace osmscout {

  void EleFeatureValue::Read(FileScanner& scanner)
  {
    ele=scanner.ReadInt16Number();
  }

  void EleFeatureValue::Write(FileWriter& writer)
  {
    writer.WriteNumber(ele);
  }

  EleFeatureValue& EleFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const EleFeatureValue&>(other);

      ele=otherValue.ele;
    }

    return *this;
  }

  std::string EleFeatureValue::GetLabel(const Locale &locale, size_t labelIndex) const
  {
    DistanceUnitSystem units;
    if (labelIndex==EleFeature::IN_LOCALE_UNIT_LABEL_INDEX){
      units=locale.GetDistanceUnits();
    } else if (labelIndex==EleFeature::IN_METER_LABEL_INDEX){
      units=DistanceUnitSystem::Metrics;
    } else {
      assert(labelIndex==EleFeature::IN_FEET_LABEL_INDEX);
      units=DistanceUnitSystem::Imperial;
    }

    int value;
    std::string unitsStr;
    if (units == DistanceUnitSystem::Imperial){
      value=std::round(Meters(ele).As<Feet>());
      unitsStr="ft";
    }else{
      value=ele;
      unitsStr="m";
    }

    std::stringstream ss;
    ss << NumberToString(value, locale);
    ss << locale.GetUnitsSeparator();
    ss << unitsStr;
    return ss.str();
  }

  bool EleFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const EleFeatureValue&>(other);

    return ele==otherValue.ele;
  }

  const char* const EleFeature::NAME = "Ele";

  const char* const EleFeature::IN_METER_LABEL       = "inMeter";
  const size_t      EleFeature::IN_METER_LABEL_INDEX = 0;

  const char* const EleFeature::IN_FEET_LABEL       = "inFeet";
  const size_t      EleFeature::IN_FEET_LABEL_INDEX = 1;

  const char* const EleFeature::IN_LOCALE_UNIT_LABEL       = "inLocaleUnit";
  const size_t      EleFeature::IN_LOCALE_UNIT_LABEL_INDEX = 2;

  EleFeature::EleFeature()
  {
    RegisterLabel(IN_METER_LABEL_INDEX,
                  IN_METER_LABEL);
    RegisterLabel(IN_FEET_LABEL_INDEX,
                  IN_FEET_LABEL);
    RegisterLabel(IN_LOCALE_UNIT_LABEL_INDEX,
                  IN_LOCALE_UNIT_LABEL);
  }

  void EleFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagEle=tagRegistry.RegisterTag("ele");
  }

  std::string EleFeature::GetName() const
  {
    return NAME;
  }

  size_t EleFeature::GetValueAlignment() const
  {
    return alignof(EleFeatureValue);
  }

  size_t EleFeature::GetValueSize() const
  {
    return sizeof(EleFeatureValue);
  }

  FeatureValue* EleFeature::AllocateValue(void* buffer)
  {
    return new (buffer) EleFeatureValue();
  }

  void EleFeature::Parse(TagErrorReporter& errorReporter,
                         const TagRegistry& /*tagRegistry*/,
                         const FeatureInstance& feature,
                         const ObjectOSMRef& object,
                         const TagMap& tags,
                         FeatureValueBuffer& buffer) const
  {
    auto ele=tags.find(tagEle);

    if (ele==tags.end()) {
      return;
    }

    std::string eleString=ele->second;
    double      e;
    size_t      pos=0;
    size_t      count=0;

    // We expect that float values use '.' as separator, but many values use ',' instead.
    // Try to fix this if string looks reasonable
    for (size_t i=0; i<eleString.length() && count<=1; i++) {
      if (eleString[i]==',') {
        pos=i;
        count++;
      }
    }

    if (count==1) {
      eleString[pos]='.';
    }

    // Some ele tag values add an 'm' to hint that the unit is meter, remove it.
    if (eleString.length()>=2) {
      if (eleString[eleString.length()-1]=='m' &&
          ((eleString[eleString.length()-2]>='0' &&
            eleString[eleString.length()-2]<='9') ||
            eleString[eleString.length()-2]<=' ')) {
        eleString.erase(eleString.length()-1);
      }

      eleString=Trim(eleString);
    }

    if (!StringToNumber(eleString,e)) {
      errorReporter.ReportTag(object,tags,std::string("Ele tag value '")+ele->second+"' is no double!");
    }
    else if (e<std::numeric_limits<int16_t>::min() || e>std::numeric_limits<int16_t>::max()) {
      errorReporter.ReportTag(object,tags,std::string("Ele tag value '")+ele->second+"' value is too small or too big!");
    }
    else {
      auto* value=static_cast<EleFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetEle(int16_t(floor(e+0.5)));
    }
  }
}
