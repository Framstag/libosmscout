#ifndef OSMSCOUT_IMPORT_BOUNDARYTYPES_H
#define OSMSCOUT_IMPORT_BOUNDARYTYPES_H

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

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>

#include <osmscoutimport/ImportImportExport.h>

namespace osmscout {

  class Progress;

  /**
   * \ingroup Import
   *
   * Resolves the types the import pipeline treats as administrative boundaries: the location index
   * builds its region tree from them and the relation-to-area step composes a boundary relation from
   * its members because of them.
   *
   * The type definition decides which admin level resolves to which type, and it may split a level out
   * of the generic boundary type at any time, so the set is resolved from the declared names in one
   * place instead of being enumerated by every step that needs it.
   *
   * A type definition that predates a boundary type the pipeline knows resolves the admin level of that
   * type to the generic boundary type, which the set carries as well, so the boundary is still indexed.
   * The absence is reported through debug output, not as a warning: an older type definition is a
   * supported input.
   *
   * A type definition that misses one of the types every map has to declare cannot build a region tree;
   * the resolver asserts on it, as its callers did before.
   */
  OSMSCOUT_IMPORT_API TypeInfoSet ResolveBoundaryTypes(const TypeConfig& typeConfig,
                                                       Progress& progress);
}

#endif // OSMSCOUT_IMPORT_BOUNDARYTYPES_H
