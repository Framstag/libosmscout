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

#include <osmscout/feature/OpeningHoursFeature.h>

namespace osmscout {

  void OpeningHoursFeatureValue::Read(FileScanner& scanner)
  {
    value=scanner.ReadString();
  }

  void OpeningHoursFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(value);
  }

  OpeningHoursFeatureValue& OpeningHoursFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const OpeningHoursFeatureValue&>(other);

      value=otherValue.value;
    }

    return *this;
  }

  bool OpeningHoursFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const OpeningHoursFeatureValue&>(other);

    return value==otherValue.value;
  }

  const char* const OpeningHoursFeature::NAME = "OpeningHours";
  const char* const OpeningHoursFeature::LABEL = "opening hours";
  const size_t      OpeningHoursFeature::LABEL_INDEX = 0;

  OpeningHoursFeature::OpeningHoursFeature()
  {
    RegisterLabel(LABEL_INDEX,LABEL);
  }

  void OpeningHoursFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagOpeningHours=tagRegistry.RegisterTag("opening_hours");
  }

  std::string OpeningHoursFeature::GetName() const
  {
    return NAME;
  }

  size_t OpeningHoursFeature::GetValueAlignment() const
  {
    return alignof(OpeningHoursFeatureValue);
  }

  size_t OpeningHoursFeature::GetValueSize() const
  {
    return sizeof(OpeningHoursFeatureValue);
  }

  FeatureValue* OpeningHoursFeature::AllocateValue(void* buffer)
  {
    return new (buffer) OpeningHoursFeatureValue();
  }

  void OpeningHoursFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                  const TagRegistry& /*tagRegistry*/,
                                  const FeatureInstance& feature,
                                  const ObjectOSMRef& /*object*/,
                                  const TagMap& tags,
                                  FeatureValueBuffer& buffer) const
  {
    using namespace std::string_literals;

    std::string colorString;
    if (auto v=tags.find(tagOpeningHours);
        v!=tags.end() && !v->second.empty()) {

      size_t idx = feature.GetIndex();
      FeatureValue* fv = buffer.AllocateValue(idx);
      auto* value = static_cast<OpeningHoursFeatureValue*>(fv);

      value->SetValue(v->second);
    }
  }
}
