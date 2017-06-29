/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings
  Copyright (C) 2017  Lukas Karas

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

#ifndef MULTIDBROUTING_H
#define MULTIDBROUTING_H

#include <osmscout/Pixel.h>

namespace osmscout {

  /**
   * \ingroup Routing
   *
   * Util class for routing cross databases
   */
  class OSMSCOUT_API MultiDBRouting CLASS_FINAL
  {
  public:
    typedef std::function<RoutingProfileRef(const DatabaseRef&)> RoutingProfileBuilder;

  private:
    static const double CELL_MAGNIFICATION;
    static const double LAT_CELL_FACTOR;
    static const double LON_CELL_FACTOR;

  private:
    std::map<std::string,DatabaseRef>       databases;
    std::map<std::string,RoutingServiceRef> services;
    std::map<std::string,RoutingProfileRef> profiles;

    bool                  isOpen;

  private:
    Pixel GetCell(const osmscout::GeoCoord& coord);
    
    bool ReadCellsForRoutingTree(osmscout::Database& database,
                                 std::unordered_set<uint64_t>& cells);

    bool ReadRouteNodesForCells(osmscout::Database& database,
                                std::unordered_set<uint64_t>& cells,
                                std::unordered_set<osmscout::Id>& routeNodes);

    bool FindCommonRoutingNodes(DatabaseRef &database1,
                                DatabaseRef &database2,
                                std::set<Id> &commonRouteNodes);
    
    bool ReadRouteNodeEntries(DatabaseRef &database,
                              std::set<Id> &commonRouteNodes,
                              std::unordered_map<osmscout::Id,osmscout::RouteNodeRef> &routeNodeEntries);

  public:
    MultiDBRouting(std::vector<DatabaseRef> databases);
    virtual ~MultiDBRouting();

    bool Open(RoutingProfileBuilder routingProfileBuilder,
              RouterParameter routerParameter);
    void Close();

    RoutePosition GetClosestRoutableNode(const GeoCoord& coord,
                                         double radius=1000,
                                         std::string databaseHint="") const;

    bool CalculateRoute(const RoutePosition &start,
                        const RoutePosition &target,
                        const RoutingParameter &parameter);
  };
}

#endif /* MULTIDBROUTING_H */

