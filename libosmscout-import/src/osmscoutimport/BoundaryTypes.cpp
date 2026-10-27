/*
  This source is part of the libosmscout library
  Copyright (C) 2026  Tim Teulings

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

#include <osmscoutimport/private/BoundaryTypes.h>

#include <cassert>

#include <osmscout/util/Progress.h>

namespace osmscout {

  namespace {

    /**
     * The boundary types every type definition the pipeline supports declares. Missing one of them is a
     * defect of the type definition, not an older definition: without them the region tree of the
     * location index has no root.
     */
    const char* const requiredBoundaryTypeNames[] = {
      "boundary_country",
      "boundary_state",
      "boundary_county",
      "boundary_administrative"
    };

    /**
     * The boundary types a type definition may lack. A definition that predates them resolves their
     * admin levels to "boundary_administrative", which the set carries as well, so nothing is dropped.
     */
    const char* const optionalBoundaryTypeNames[] = {
      "boundary_municipality",
      "boundary_suburb"
    };
  }

  TypeInfoSet ResolveBoundaryTypes(const TypeConfig& typeConfig,
                                   Progress& progress)
  {
    TypeInfoSet boundaryTypes(typeConfig);

    for (const auto* name : requiredBoundaryTypeNames) {
      TypeInfoRef boundaryType=typeConfig.GetTypeInfo(name);

      assert(boundaryType);

      boundaryTypes.Set(boundaryType);
    }

    for (const auto* name : optionalBoundaryTypeNames) {
      TypeInfoRef boundaryType=typeConfig.GetTypeInfo(name);

      if (boundaryType) {
        boundaryTypes.Set(boundaryType);
      }
      else {
        progress.Debug(
          "Type definition does not declare \"{}\", its admin level is indexed as \"boundary_administrative\"",
          name);
      }
    }

    return boundaryTypes;
  }
}
