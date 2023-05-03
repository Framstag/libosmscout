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

#include <osmscout/feature/GradeFeature.h>

namespace osmscout {

  void GradeFeatureValue::Read(FileScanner& scanner)
  {
    grade=scanner.ReadUInt8();
  }

  void GradeFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(grade);
  }

  GradeFeatureValue& GradeFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const GradeFeatureValue&>(other);

      grade=otherValue.grade;
    }

    return *this;
  }

  bool GradeFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const GradeFeatureValue&>(other);

    return grade==otherValue.grade;
  }

  const char* const GradeFeature::NAME = "Grade";

  void GradeFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagSurface=tagRegistry.RegisterTag("surface");
    tagTrackType=tagRegistry.RegisterTag("tracktype");
  }

  std::string GradeFeature::GetName() const
  {
    return NAME;
  }

  size_t GradeFeature::GetValueAlignment() const
  {
    return alignof(GradeFeatureValue);
  }

  size_t GradeFeature::GetValueSize() const
  {
    return sizeof(GradeFeatureValue);
  }

  FeatureValue* GradeFeature::AllocateValue(void* buffer)
  {
    return new (buffer) GradeFeatureValue();
  }

  void GradeFeature::Parse(TagErrorReporter& errorReporter,
                           const TagRegistry& tagRegistry,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    auto tracktype=tags.find(tagTrackType);

    if (tracktype!=tags.end()) {
      if (tracktype->second=="grade1") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(1);

        return;
      }

      if (tracktype->second=="grade2") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(2);

        return;
      }

      if (tracktype->second=="grade3") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(3);

        return;
      }

      if (tracktype->second=="grade4") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(4);

        return;
      }

      if (tracktype->second=="grade5") {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade(5);

        return;
      }

      errorReporter.ReportTag(object,tags,std::string("Unsupported tracktype value '")+tracktype->second+"'");
    }

    auto surface=tags.find(tagSurface);

    if (surface!=tags.end()) {
      size_t grade;

      if (tagRegistry.GetGradeForSurface(surface->second,
                                         grade)) {
        auto* value=static_cast<GradeFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetGrade((uint8_t)grade);
      }
      else {
        errorReporter.ReportTag(object,tags,std::string("Unknown surface type '")+surface->second+"' !");
      }
    }
  }
}
