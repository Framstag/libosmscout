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
    IndexedDataFile<Id,RouteNode>    routeNodeDataFile;     //!< Cached access to the 'route.dat' file
    IndexedDataFile<Id,Intersection> junctionDataFile;      //!< Cached access to the 'junctions.dat' file
    ObjectVariantDataFile            objectVariantDataFile;

  public:
    RoutingDatabase();

    bool Open(const DatabaseRef& database);
    void Close();

    inline bool GetRouteNode(const Id& id,
                             RouteNodeRef& node)
    {
      return routeNodeDataFile.Get(id,
                                   node);
    }

    inline bool GetRouteNodesByOffset(const std::vector<FileOffset>& routeNodeOffsets,
                                      std::unordered_map<FileOffset,RouteNodeRef>& routeNodeMap)
    {
      return routeNodeDataFile.GetByOffset(routeNodeOffsets.begin(),
                                           routeNodeOffsets.end(),
                                           routeNodeOffsets.size(),
                                           routeNodeMap);
    }

    inline bool GetRouteNodesByOffset(const std::set<FileOffset>& routeNodeOffsets,
                                      std::vector<RouteNodeRef>& routeNodes)
    {
      return routeNodeDataFile.GetByOffset(routeNodeOffsets.begin(),
                                           routeNodeOffsets.end(),
                                           routeNodeOffsets.size(),
                                           routeNodes);
    }

    /**
     * Return the route node for the given database offset
     * @param offset
     *    Offset in given database
     * @param node
     *    Node instance to write the result back
     * @return
     *    True, if the node couldbe loaded, else false
     */
    inline bool GetRouteNodeByOffset(const FileOffset& offset,
                                     RouteNodeRef& node)
    {
      return routeNodeDataFile.GetByOffset(offset,
                                           node);
    }

    inline bool GetRouteNodeOffset(Id id,
                                   FileOffset& offset)
    {
      return routeNodeDataFile.GetOffset(id,
                                         offset);
    }

    bool GetJunctions(const std::set<Id>& ids,
                      std::vector<JunctionRef>& junctions);

    inline const std::vector<ObjectVariantData>& GetObjectVariantData() const
    {
      return objectVariantDataFile.GetData();
    }
  };

  /**
   * \ingroup Routing
   */
  typedef std::shared_ptr<RoutingDatabase> RoutingDatabaseRef;
}

#endif
