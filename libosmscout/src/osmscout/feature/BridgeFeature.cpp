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

#include <osmscout/feature/BridgeFeature.h>

namespace osmscout {

  const char* const BridgeFeature::NAME = "Bridge";

  void BridgeFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagBridge=tagRegistry.RegisterTag("bridge");
  }

  std::string BridgeFeature::GetName() const
  {
    return NAME;
  }

  void BridgeFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TagRegistry& /*tagRegistry*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const TagMap& tags,
                            FeatureValueBuffer& buffer) const
  {
    auto bridge=tags.find(tagBridge);

    if (bridge!=tags.end() &&
        !(bridge->second=="no" ||
          bridge->second=="false" ||
          bridge->second=="0")) {
      buffer.AllocateValue(feature.GetIndex());
    }
  }
}
