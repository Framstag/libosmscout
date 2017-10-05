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

#include <osmscout/Path.h>

namespace osmscout {

  Path::Path(const ObjectFileRef& object,
             size_t targetNodeIndex)
  : object(object),
    targetNodeIndex(targetNodeIndex),
    traversable(true)
  {
    // no code
  }

  Path::Path(const ObjectFileRef& object,
             size_t targetNodeIndex,
             bool traversable)
  : object(object),
    targetNodeIndex(targetNodeIndex),
    traversable(traversable)
  {
    // no code
  }

  Path::Path(const Path& other)
  {
    this->object=other.object;
    this->targetNodeIndex=other.targetNodeIndex;
    this->traversable=other.traversable;
  }
}
