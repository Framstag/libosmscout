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

#include <osmscout/feature/NetworkFeature.h>

namespace osmscout {

  void NetworkFeatureValue::Read(FileScanner& scanner)
  {
    network=scanner.ReadString();
  }

  void NetworkFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(network);
  }

  NetworkFeatureValue& NetworkFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const NetworkFeatureValue&>(other);

      network=otherValue.network;
    }

    return *this;
  }

  bool NetworkFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const NetworkFeatureValue&>(other);

    return network==otherValue.network;
  }

  const char* const NetworkFeature::NAME = "Network";
  const char* const NetworkFeature::NAME_LABEL = "name";
  const size_t      NetworkFeature::NAME_LABEL_INDEX = 0;

  NetworkFeature::NetworkFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,NAME_LABEL);
  }

  void NetworkFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagNetwork=tagRegistry.RegisterTag("network");
  }

  std::string NetworkFeature::GetName() const
  {
    return NAME;
  }

  size_t NetworkFeature::GetValueAlignment() const
  {
    return alignof(NetworkFeatureValue);
  }

  size_t NetworkFeature::GetValueSize() const
  {
    return sizeof(NetworkFeatureValue);
  }

  FeatureValue* NetworkFeature::AllocateValue(void* buffer)
  {
    return new (buffer) NetworkFeatureValue();
  }

  void NetworkFeature::Parse(TagErrorReporter& /*errorReporter*/,
                             const TagRegistry& /*tagRegistry*/,
                             const FeatureInstance& feature,
                             const ObjectOSMRef& /*object*/,
                             const TagMap& tags,
                             FeatureValueBuffer& buffer) const
  {
    auto network= tags.find(tagNetwork);

    if (network!=tags.end() && !network->second.empty()) {
      auto* value = static_cast<NetworkFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

      value->SetNetwork(network->second);
    }
  }
}
