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
#include <osmscout/OSMScoutTypes.h>
#include <osmscout/routing/DBFileOffset.h>

namespace osmscout {

  /**
   * \ingroup Routing
   */
  class OSMSCOUT_API RouteData
  {
  public:
    class OSMSCOUT_API RouteEntry
    {
    private:
      DatabaseId                 database;
      Id                         currentNodeId;
      size_t                     currentNodeIndex;
      std::vector<ObjectFileRef> objects;
      ObjectFileRef              pathObject;
      size_t                     targetNodeIndex;

    public:
      RouteEntry(DatabaseId database,
                 Id currentNodeId,
                 size_t currentNodeIndex,
                 const ObjectFileRef& pathObject,
                 size_t targetNodeIndex);

      void SetObjects(const std::vector<ObjectFileRef>& objects);

      Id GetCurrentNodeId() const
      {
        return currentNodeId;
      }

      DBFileOffset GetDBFileOffset() const
      {
        return DBFileOffset(GetDatabaseId(),GetPathObject().GetFileOffset());
      }

      DatabaseId GetDatabaseId() const
      {
        return database;
      }

      size_t GetCurrentNodeIndex() const
      {
        return currentNodeIndex;
      }

      ObjectFileRef GetPathObject() const
      {
        return pathObject;
      }

      size_t GetTargetNodeIndex() const
      {
        return targetNodeIndex;
      }

      const std::vector<ObjectFileRef>& GetObjects() const
      {
        return objects;
      }
    };

  private:
    std::list<RouteEntry> entries;

  public:
    void Clear();

    bool IsEmpty() const
    {
      return entries.empty();
    }

    void AddEntry(DatabaseId database,
                  Id currentNodeId,
                  size_t currentNodeIndex,
                  const ObjectFileRef& pathObject,
                  size_t targetNodeIndex);

    std::list<RouteEntry>& Entries()
    {
      return entries;
    }

    const std::list<RouteEntry>& Entries() const
    {
      return entries;
    }

    void Append(RouteData routePart)
    {
      entries.splice(entries.end(),routePart.Entries());
    }

    void PopEntry()
    {
      entries.pop_back();
    }
  };
}

#endif
