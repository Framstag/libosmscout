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

#include <osmscout/feature/ConstructionYearFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void ConstructionYearFeatureValue::Read(FileScanner& scanner)
  {
    startYear=scanner.ReadInt32();
    endYear=scanner.ReadInt32();
  }

  void ConstructionYearFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(startYear);
    writer.Write(endYear);
  }

  ConstructionYearFeatureValue& ConstructionYearFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const ConstructionYearFeatureValue&>(other);

      startYear=otherValue.startYear;
      endYear=otherValue.endYear;
    }

    return *this;
  }

  bool ConstructionYearFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const ConstructionYearFeatureValue&>(other);

    return startYear==otherValue.startYear &&
      endYear==otherValue.endYear;
  }

  const char* const ConstructionYearFeature::NAME = "ConstructionYear";
  const char* const ConstructionYearFeature::YEAR_LABEL = "year";
  const size_t      ConstructionYearFeature::YEAR_LABEL_INDEX = 0;

  ConstructionYearFeature::ConstructionYearFeature()
  {
    RegisterLabel(YEAR_LABEL_INDEX,YEAR_LABEL);
  }

  void ConstructionYearFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagConstructionYear=tagRegistry.RegisterTag("year_of_construction");
    tagStartDate=tagRegistry.RegisterTag("start_date");
  }

  std::string ConstructionYearFeature::GetName() const
  {
    return NAME;
  }

  size_t ConstructionYearFeature::GetValueAlignment() const
  {
    return alignof(ConstructionYearFeatureValue);
  }

  size_t ConstructionYearFeature::GetValueSize() const
  {
    return sizeof(ConstructionYearFeatureValue);
  }

  FeatureValue* ConstructionYearFeature::AllocateValue(void* buffer)
  {
    return new (buffer) ConstructionYearFeatureValue();
  }

  void ConstructionYearFeature::Parse(TagErrorReporter& errorReporter,
                                      const TagRegistry& /*tagRegistry*/,
                                      const FeatureInstance& feature,
                                      const ObjectOSMRef& object,
                                      const TagMap& tags,
                                      FeatureValueBuffer& buffer) const
  {
    auto constructionYearTag=tags.find(tagConstructionYear);

    std::string strValue;

    if (constructionYearTag!=tags.end()) {
      strValue=constructionYearTag->second;
    }
    else {
      auto startDateTag=tags.find(tagStartDate);

      if (startDateTag!=tags.end()) {
        strValue=startDateTag->second;
      }
      else {
        return;
      }
    }

    int startYear;
    int endYear;

    if (strValue[0]=='~') {
      strValue=strValue.substr(1);
    }


    if (osmscout::StringToNumber(strValue,startYear)) {
      auto* value=dynamic_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetStartYear(startYear);
      value->SetEndYear(startYear);

      return;
    }
    auto pos=strValue.find('-');

    if (pos!=std::string::npos) {
      std::string startValue=strValue.substr(0,pos);
      std::string endValue=strValue.substr(pos+1);

      if (!startValue.empty() &&
          !endValue.empty() &&
        osmscout::StringToNumber(startValue,startYear) &&
        osmscout::StringToNumber(endValue,endYear)) {

        auto* value=static_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetStartYear(startYear);
        value->SetEndYear(endYear);
      }

      return;
    }

    pos=strValue.find('/');

    if (pos!=std::string::npos) {
      std::string startValue=strValue.substr(0,pos);
      std::string endValue=strValue.substr(pos+1);

      if (!startValue.empty() &&
          !endValue.empty() &&
          osmscout::StringToNumber(startValue,startYear) &&
          osmscout::StringToNumber(endValue,endYear)) {

        auto* value=static_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetStartYear(startYear);
        value->SetEndYear(endYear);
      }

      return;
    }

    pos=strValue.find("..");

    if (pos!=std::string::npos) {
      std::string startValue=strValue.substr(0,pos);
      std::string endValue=strValue.substr(pos+2);

      if (!startValue.empty() &&
          !endValue.empty() &&
          osmscout::StringToNumber(startValue,startYear) &&
          osmscout::StringToNumber(endValue,endYear)) {

        auto* value=static_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetStartYear(startYear);
        value->SetEndYear(endYear);
      }

      return;
    }

    if (strValue[0]=='C') {
      std::string startValue=strValue.substr(1);

      if (osmscout::StringToNumber(startValue,startYear)) {
        auto* value=static_cast<ConstructionYearFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetStartYear(startYear*100);
        value->SetEndYear((startYear+1)*100-1);

        return;
      }
    }

    errorReporter.ReportTag(object,tags,std::string("Construction startYear tag value '")+strValue+"' cannot be parsed to a startYear!");
  }
}
