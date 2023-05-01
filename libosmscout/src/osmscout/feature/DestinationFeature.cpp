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

#include <osmscout/feature/DestinationFeature.h>

namespace osmscout {

  void DestinationFeatureValue::Read(FileScanner& scanner)
  {
    destination=scanner.ReadString();
  }

  void DestinationFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(destination);
  }

  DestinationFeatureValue& DestinationFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const DestinationFeatureValue&>(other);

      destination=otherValue.destination;
    }

    return *this;
  }

  bool DestinationFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const DestinationFeatureValue&>(other);

    return destination==otherValue.destination;
  }

  const char* const DestinationFeature::NAME             = "Destination";
  const char* const DestinationFeature::NAME_LABEL       = "label";
  const size_t      DestinationFeature::NAME_LABEL_INDEX = 0;

  DestinationFeature::DestinationFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void DestinationFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagDestination=tagRegistry.RegisterTag("destination");
    tagDestinationRef=tagRegistry.RegisterTag("destination:ref");
    tagDestinationForward=tagRegistry.RegisterTag("destination:forward");
  }

  std::string DestinationFeature::GetName() const
  {
    return NAME;
  }

  size_t DestinationFeature::GetValueAlignment() const
  {
    return alignof(DestinationFeatureValue);
  }

  size_t DestinationFeature::GetValueSize() const
  {
    return sizeof(DestinationFeatureValue);
  }

  FeatureValue* DestinationFeature::AllocateValue(void* buffer)
  {
    return new (buffer) DestinationFeatureValue();
  }

  void DestinationFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                 const TagRegistry& /*tagRegistry*/,
                                 const FeatureInstance& feature,
                                 const ObjectOSMRef& /*object*/,
                                 const TagMap& tags,
                                 FeatureValueBuffer& buffer) const
  {
    auto destination=tags.find(tagDestination);

    if (destination==tags.end()) {
      destination=tags.find(tagDestinationForward);
    }

    if (destination==tags.end()) {
      destination=tags.find(tagDestinationRef);
    }

    if (destination==tags.end()) {
      return;
    }

    if (!destination->second.empty()) {
      auto* value=static_cast<DestinationFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetDestination(destination->second);
    }
  }
}
