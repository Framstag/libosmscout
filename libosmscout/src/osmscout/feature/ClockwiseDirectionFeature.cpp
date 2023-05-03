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

#include <osmscout/feature/ClockwiseDirectionFeature.h>

namespace osmscout {

  const char* const ClockwiseDirectionFeature::NAME = "ClockwiseDirection";

  void ClockwiseDirectionFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagDirection=tagRegistry.RegisterTag("direction");
  }

  std::string ClockwiseDirectionFeature::GetName() const
  {
    return NAME;
  }

  void ClockwiseDirectionFeature::Parse(TagErrorReporter& /*errorReporter*/,
                                        const TagRegistry& /*tagRegistry*/,
                                        const FeatureInstance& feature,
                                        const ObjectOSMRef& /*object*/,
                                        const TagMap& tags,
                                        FeatureValueBuffer& buffer) const
  {
    auto junction=tags.find(tagDirection);

    if (junction!=tags.end() &&
        junction->second=="clockwise") {
      buffer.AllocateValue(feature.GetIndex());
    }
  }
}
