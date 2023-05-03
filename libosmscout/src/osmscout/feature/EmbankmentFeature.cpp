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

#include <osmscout/feature/EmbankmentFeature.h>

namespace osmscout {

  const char* const EmbankmentFeature::NAME = "Embankment";

  void EmbankmentFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagEmbankment=tagRegistry.RegisterTag("embankment");
  }

  std::string EmbankmentFeature::GetName() const
  {
    return NAME;
  }

  void EmbankmentFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                const TagRegistry& /*tagRegistry*/,
                                const FeatureInstance& feature,
                                const ObjectOSMRef& /*object*/,
                                const TagMap& tags,
                                FeatureValueBuffer& buffer) const
  {
    auto embankment=tags.find(tagEmbankment);

    if (embankment!=tags.end() &&
        !(embankment->second=="no" ||
          embankment->second=="false" ||
          embankment->second=="0")) {
            buffer.AllocateValue(feature.GetIndex());
    }
  }
}
