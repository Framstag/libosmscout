#ifndef OSMSCOUT_ROUTEDATA_H
#define OSMSCOUT_ROUTEDATA_H

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

#include <list>
#include <vector>

#include <osmscout/Path.h>
#include <osmscout/Types.h>

namespace osmscout {

  class OSMSCOUT_API RouteData
  {
  public:
    class RouteEntry
    {
    private:
      Id                currentNodeId;
      std::vector<Path> paths;
      FileOffset        pathWayOffset;
      Id                targetNodeId;

    public:
      RouteEntry(Id currentNodeId,
                 FileOffset pathWayOffset,
                 Id targetNodeId);

      RouteEntry(Id currentNodeId,
                 const std::vector<Path>& paths,
                 FileOffset pathWayOffset,
                 Id targetNodeId);

      inline Id GetCurrentNodeId() const
      {
        return currentNodeId;
      }

      inline Id GetPathWayOffset() const
      {
        return pathWayOffset;
      }

      inline Id GetTargetNodeId() const
      {
        return targetNodeId;
      }

      inline const std::vector<Path>& GetPaths() const
      {
        return paths;
      }
    };

  private:
    std::list<RouteEntry> entries;

  public:
    RouteData();

    void Clear();

    void AddEntry(Id currentNodeId,
                  FileOffset pathWayOffset,
                  Id targetNodeId);

    void AddEntry(Id currentNodeId,
                  const std::vector<Path>& paths,
                  FileOffset pathWayOffset,
                  Id targetNodeId);

    inline const std::list<RouteEntry>& Entries() const
    {
      return entries;
    }
  };
}

#endif
