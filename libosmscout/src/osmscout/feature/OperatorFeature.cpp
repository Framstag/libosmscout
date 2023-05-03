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

#include <osmscout/feature/OperatorFeature.h>

namespace osmscout {

  void OperatorFeatureValue::Read(FileScanner& scanner)
  {
    op=scanner.ReadString();
  }

  void OperatorFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(op);
  }

  OperatorFeatureValue& OperatorFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const OperatorFeatureValue&>(other);

      op=otherValue.op;
    }

    return *this;
  }

  bool OperatorFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const OperatorFeatureValue&>(other);

    return op==otherValue.op;
  }

  const char* const OperatorFeature::NAME = "Operator";
  const char* const OperatorFeature::NAME_LABEL = "name";
  const size_t      OperatorFeature::NAME_LABEL_INDEX = 0;

  OperatorFeature::OperatorFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,NAME_LABEL);
  }

  void OperatorFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagOperator=tagRegistry.RegisterTag("operator");
  }

  std::string OperatorFeature::GetName() const
  {
    return NAME;
  }

  size_t OperatorFeature::GetValueAlignment() const
  {
    return alignof(OperatorFeatureValue);
  }

  size_t OperatorFeature::GetValueSize() const
  {
    return sizeof(OperatorFeatureValue);
  }

  FeatureValue* OperatorFeature::AllocateValue(void* buffer)
  {
    return new (buffer) OperatorFeatureValue();
  }

  void OperatorFeature::Parse(TagErrorReporter& /*errorReporter*/,
                              const TagRegistry& /*tagRegistry*/,
                              const FeatureInstance& feature,
                              const ObjectOSMRef& /*object*/,
                              const TagMap& tags,
                              FeatureValueBuffer& buffer) const
  {
    auto op = tags.find(tagOperator);

    if (op != tags.end() && !op->second.empty()) {
      auto* value = static_cast<OperatorFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetOperator(op->second);
    }
  }
}
