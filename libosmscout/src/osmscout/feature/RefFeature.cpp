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

#include <osmscout/feature/RefFeature.h>

namespace osmscout {

  void RefFeatureValue::Read(FileScanner& scanner)
  {
    ref=scanner.ReadString();
  }

  void RefFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(ref);
  }

  RefFeatureValue& RefFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const RefFeatureValue&>(other);

      ref=otherValue.ref;
    }

    return *this;
  }

  bool RefFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const RefFeatureValue&>(other);

    return ref==otherValue.ref;
  }

  const char* const RefFeature::NAME             = "Ref";
  const char* const RefFeature::NAME_LABEL       = "name";
  const size_t      RefFeature::NAME_LABEL_INDEX = 0;

  RefFeature::RefFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void RefFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagRef=tagRegistry.RegisterTag("ref");
  }

  std::string RefFeature::GetName() const
  {
    return NAME;
  }

  size_t RefFeature::GetValueAlignment() const
  {
    return alignof(RefFeatureValue);
  }

  size_t RefFeature::GetValueSize() const
  {
    return sizeof(RefFeatureValue);
  }

  FeatureValue* RefFeature::AllocateValue(void* buffer)
  {
    return new (buffer) RefFeatureValue();
  }

  void RefFeature::Parse(TagErrorReporter& /*errorReporter*/,
                         const TagRegistry& /*tagRegistry*/,
                         const FeatureInstance& feature,
                         const ObjectOSMRef& /*object*/,
                         const TagMap& tags,
                         FeatureValueBuffer& buffer) const
  {
    auto ref=tags.find(tagRef);

    if (ref!=tags.end() &&
        !ref->second.empty()) {
      auto* value=static_cast<RefFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetRef(ref->second);
    }
  }
}
