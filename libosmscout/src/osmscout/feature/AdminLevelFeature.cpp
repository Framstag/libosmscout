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

#include <osmscout/feature/AdminLevelFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void AdminLevelFeatureValue::Read(FileScanner& scanner)
  {
    adminLevel=scanner.ReadUInt8();
    isIn=scanner.ReadString();
  }

  void AdminLevelFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(adminLevel);
    writer.Write(isIn);
  }

  AdminLevelFeatureValue& AdminLevelFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const AdminLevelFeatureValue&>(other);

      adminLevel=otherValue.adminLevel;
      isIn=otherValue.isIn;
    }

    return *this;
  }

  bool AdminLevelFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const AdminLevelFeatureValue&>(other);

    return adminLevel==otherValue.adminLevel && isIn==otherValue.isIn;
  }

  const char* const AdminLevelFeature::NAME = "AdminLevel";

  void AdminLevelFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagAdminLevel=tagRegistry.RegisterTag("admin_level");
    tagIsIn=tagRegistry.RegisterTag("is_in");
  }

  std::string AdminLevelFeature::GetName() const
  {
    return NAME;
  }

  size_t AdminLevelFeature::GetValueAlignment() const
  {
    return alignof(AdminLevelFeatureValue);
  }

  size_t AdminLevelFeature::GetValueSize() const
  {
    return sizeof(AdminLevelFeatureValue);
  }

  FeatureValue* AdminLevelFeature::AllocateValue(void* buffer)
  {
    return new (buffer) AdminLevelFeatureValue();
  }

  void AdminLevelFeature::Parse(TagErrorReporter& errorReporter,
                                const TagRegistry& /*tagRegistry*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& object,
                                const TagMap& tags,
                                FeatureValueBuffer& buffer) const
  {
    auto adminLevel=tags.find(tagAdminLevel);

    if (adminLevel!=tags.end()) {
      uint8_t adminLevelValue;

      if (StringToNumber(adminLevel->second,
                         adminLevelValue)) {
        auto* value=static_cast<AdminLevelFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

        value->SetAdminLevel(adminLevelValue);

        auto isIn=tags.find(tagIsIn);

        if (isIn!=tags.end()) {
          value->SetIsIn(isIn->second);
        }
      }
      else {
        errorReporter.ReportTag(object,tags,std::string("Admin level is not numeric '")+adminLevel->second+"'!");
      }
    }
  }
}
