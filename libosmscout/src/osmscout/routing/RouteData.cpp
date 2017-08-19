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

#include <osmscout/routing/RouteData.h>

namespace osmscout {

  RouteData::RouteEntry::RouteEntry(DatabaseId database,
                                    Id currentNodeId,
                                    size_t currentNodeIndex,
                                    const ObjectFileRef& pathObject,
                                    size_t targetNodeIndex)
   : database(database),
     currentNodeId(currentNodeId),
     currentNodeIndex(currentNodeIndex),
     pathObject(pathObject),
     targetNodeIndex(targetNodeIndex)
  {
    // no code
  }

  void RouteData::RouteEntry::SetObjects(const std::vector<ObjectFileRef> objects)
  {
    this->objects=objects;
  }

  void RouteData::Clear()
  {
    entries.clear();
  }

  void RouteData::AddEntry(DatabaseId database,
                           Id currentNodeId,
                           size_t currentNodeIndex,
                           const ObjectFileRef& pathObject,
                           size_t targetNodeIndex)
  {
    entries.push_back(RouteEntry(database,
                                 currentNodeId,
                                 currentNodeIndex,
                                 pathObject,
                                 targetNodeIndex));
  }
}

