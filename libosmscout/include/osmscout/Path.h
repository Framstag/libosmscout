#ifndef OSMSCOUT_PATH_H
#define OSMSCOUT_PATH_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <osmscout/lib/CoreImportExport.h>

#include <osmscout/OSMScoutTypes.h>

#include <osmscout/ObjectRef.h>

namespace osmscout {

  /**
   * \ingroup Geometry
   * A path is defined by the way to be used and the node id of a node on this way
   * which is the target to reach.
   */
  class OSMSCOUT_API Path CLASS_FINAL
  {
  private:
    ObjectFileRef object;
    size_t        targetNodeIndex;
    bool          traversable;

  public:
    Path(const ObjectFileRef& Object,
         size_t targetNodeIndex);
    Path(const ObjectFileRef& object,
         size_t targetNodeIndex,
         bool traversable);
    Path(const Path& other);

    ObjectFileRef GetObject() const
    {
      return object;
    }

    size_t GetTargetNodeIndex() const
    {
      return targetNodeIndex;
    }

    bool IsTraversable() const
    {
      return traversable;
    }
  };
}

#endif
