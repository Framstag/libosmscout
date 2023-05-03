#ifndef OSMSCOUT_MAP_STYLEPROCESSOR_H
#define OSMSCOUT_MAP_STYLEPROCESSOR_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2017  Tim Teulings

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

#include <osmscoutmap/MapImportExport.h>

#include <osmscout/TypeConfig.h>

#include <osmscoutmap/Styles.h>

namespace osmscout {
  class OSMSCOUT_MAP_API FillStyleProcessor
  {
  public:
    virtual ~FillStyleProcessor() = default;

    virtual FillStyleRef Process(const FeatureValueBuffer& features,
                                 const FillStyleRef& fillStyle) const = 0;
  };

  using FillStyleProcessorRef = std::shared_ptr<FillStyleProcessor>;
}

#endif
