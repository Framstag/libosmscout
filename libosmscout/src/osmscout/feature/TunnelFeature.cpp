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

#include <osmscout/feature/TunnelFeature.h>

namespace osmscout {

  const char* const TunnelFeature::NAME = "Tunnel";

  void TunnelFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagTunnel=tagRegistry.RegisterTag("tunnel");
  }

  std::string TunnelFeature::GetName() const
  {
    return NAME;
  }

  void TunnelFeature::Parse(TagErrorReporter& /*errorReporter*/,
                            const TagRegistry& /*tagRegistry*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& /*object*/,
                            const TagMap& tags,
                            FeatureValueBuffer& buffer) const
  {
    auto tunnel=tags.find(tagTunnel);

    if (tunnel!=tags.end() &&
        !(tunnel->second=="no" ||
          tunnel->second=="false" ||
          tunnel->second=="0")) {
      buffer.AllocateValue(feature.GetIndex());
    }
  }

}
