#ifndef OSMSCOUT_ROUTINGDB_H
#define OSMSCOUT_ROUTINGDB_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2018  Tim Teulings

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

#include <memory>

#include <osmscout/Database.h>
#include <osmscout/DataFile.h>
#include <osmscout/Intersection.h>
#include <osmscout/ObjectVariantDataFile.h>

#include <osmscout/routing/RouteNode.h>
#include <osmscout/routing/RouteNodeDataFile.h>

namespace osmscout {

  /**
   * \ingroup Routing
   *
   * Encapsulation of the routing relevant data files, similar to Database.
   */
  class RoutingDatabase CLASS_FINAL
  {
  private:
    TypeConfigRef                    typeConfig;
    std::string                      path;
    RouteNodeDataFile                routeNodeDataFile;
    IndexedDataFile<Id,Intersection> junctionDataFile;      //!< Cached access to the 'junctions.dat' file
    ObjectVariantDataFile            objectVariantDataFile;

  public:
    RoutingDatabase();

    bool Open(const DatabaseRef& database);
    void Close();

    inline bool GetRouteNode(const Id& id,
                             RouteNodeRef& node) const
    {
      return routeNodeDataFile.Get(id,
                                   node);
    }

    template<typename IteratorIn>
    inline bool GetRouteNodes(IteratorIn begin, IteratorIn end, size_t size,
                              std::unordered_map<Id,RouteNodeRef>& routeNodeMap)
    {
      return routeNodeDataFile.Get(begin,
                                   end,
                                   size,
                                   routeNodeMap);
    }

    template<typename IteratorIn>
    inline bool GetRouteNodes(IteratorIn begin, IteratorIn end, size_t size,
                              std::vector<RouteNodeRef>& routeNodes)
    {
      return routeNodeDataFile.Get(begin,
                                   end,
                                   size,
                                   routeNodes);
    }

    bool GetJunctions(const std::set<Id>& ids,
                      std::vector<JunctionRef>& junctions);

    inline const std::vector<ObjectVariantData>& GetObjectVariantData() const
    {
      return objectVariantDataFile.GetData();
    }

    inline bool ContainsNode(const Id id) const
    {
      RouteNodeRef node;
      routeNodeDataFile.Get(id, node);
      return (bool)node;
    }
  };

  /**
   * \ingroup Routing
   */
  using RoutingDatabaseRef = std::shared_ptr<RoutingDatabase>;
}

#endif
