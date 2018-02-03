#ifndef OSMSCOUT_MULTIDBROUTINGSERVICE_H
#define OSMSCOUT_MULTIDBROUTINGSERVICE_H

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

#include <osmscout/Pixel.h>
#include <osmscout/routing/AbstractRoutingService.h>
#include <osmscout/routing/SimpleRoutingService.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/routing/RoutingDB.h>

namespace osmscout {

  /**
   * \ingroup Routing
   *
   * Util class for routing cross databases
   */
  class OSMSCOUT_API MultiDBRoutingService CLASS_FINAL : public AbstractRoutingService<MultiDBRoutingState>
  {
  private:
    struct DatabaseHandle CLASS_FINAL
    {
      DatabaseId              dbId;            //<! Numeric id of the database (also index to the handles array)
      DatabaseRef             database;        //<! Object database
      RoutingDatabaseRef      routingDatabase; //<! Routing database
      SimpleRoutingServiceRef router;          //<! Simple router for the given database
      RoutingProfileRef       profile;         //<! Profile for the given database
    };

  public:
    typedef std::function<RoutingProfileRef(const DatabaseRef&)> RoutingProfileBuilder;

  private:
    static const size_t CELL_MAGNIFICATION;
    static const double LAT_CELL_FACTOR;
    static const double LON_CELL_FACTOR;

  private:
    std::vector<DatabaseHandle> handles;
    bool                        isOpen;

  private:

    Pixel GetCell(const osmscout::GeoCoord& coord);

    bool ReadCellsForRoutingTree(osmscout::Database& database,
                                 std::unordered_set<uint64_t>& cells);

    bool ReadRouteNodesForCells(osmscout::Database& database,
                                std::unordered_set<uint64_t>& cells,
                                std::unordered_set<osmscout::Id>& routeNodes);

    bool FindCommonRoutingNodes(const BreakerRef &breaker,
                                DatabaseRef &database1,
                                DatabaseRef &database2,
                                std::set<Id> &commonRouteNodes);

  private:
    Vehicle GetVehicle(const MultiDBRoutingState& state) override;

    bool CanUseForward(const MultiDBRoutingState& state,
                       const DatabaseId& database,
                       const WayRef& way) override;

    bool CanUseBackward(const MultiDBRoutingState& state,
                        const DatabaseId& database,
                        const WayRef& way) override;

    double GetCosts(const MultiDBRoutingState& state,
                    DatabaseId databaseId,
                    const RouteNode& routeNode,
                    size_t pathIndex) override;

    double GetCosts(const MultiDBRoutingState& state,
                    DatabaseId database,
                    const WayRef &way,
                    double wayLength) override;

    double GetEstimateCosts(const MultiDBRoutingState& state,
                            DatabaseId database,
                            double targetDistance) override;

    double GetCostLimit(const MultiDBRoutingState& state,
                        DatabaseId database,
                        double targetDistance) override;

    bool GetRouteNodesByOffset(const std::set<DBFileOffset> &routeNodeOffsets,
                               std::unordered_map<DBFileOffset,RouteNodeRef> &routeNodeMap) override;

    bool GetRouteNodeByOffset(const DBFileOffset &offset,
                              RouteNodeRef &node) override;

    bool GetWayByOffset(const DBFileOffset &offset,
                        WayRef &way) override;

    bool GetWaysByOffset(const std::set<DBFileOffset> &wayOffsets,
                         std::unordered_map<DBFileOffset,WayRef> &wayMap) override;

    bool GetAreaByOffset(const DBFileOffset &offset,
                         AreaRef &area) override;

    bool GetAreasByOffset(const std::set<DBFileOffset> &areaOffsets,
                          std::unordered_map<DBFileOffset,AreaRef> &areaMap) override;

    bool ResolveRouteDataJunctions(RouteData& route) override;

    std::vector<DBFileOffset> GetNodeTwins(const MultiDBRoutingState& state,
                                           DatabaseId database,
                                           Id id) override;

    bool GetRouteNode(const DatabaseId &databaseId,
                      const Id &id,
                      RouteNodeRef &node) override;

    bool CanUse(const MultiDBRoutingState& state,
                DatabaseId databaseId,
                const RouteNode& routeNode,
                size_t pathIndex) override;

  public:
    MultiDBRoutingService(const RouterParameter& parameter,
                          const std::vector<DatabaseRef> &databases);
    ~MultiDBRoutingService() override;

    bool Open(RoutingProfileBuilder routingProfileBuilder);

    void Close();

    RoutePosition GetClosestRoutableNode(const GeoCoord& coord,
                                         double radius=1000) const;

    RoutingResult CalculateRoute(const RoutePosition &start,
                                 const RoutePosition &target,
                                 const RoutingParameter &parameter);

    RoutingResult CalculateRoute(std::vector<osmscout::GeoCoord> via,
                                 double radius,
                                 const RoutingParameter& parameter);

    bool TransformRouteDataToRouteDescription(const RouteData& data,
                                              RouteDescription& description);

    bool TransformRouteDataToPoints(const RouteData& data,
                                    std::list<Point>& points);

    bool TransformRouteDataToWay(const RouteData& data,
                                 Way& way);

    bool PostProcessRouteDescription(RouteDescription &description,
                                     const std::list<RoutePostprocessor::PostprocessorRef> &postprocessors);
  };

  //! \ingroup Service
  //! Reference counted reference to an RoutingService instance
  typedef std::shared_ptr<MultiDBRoutingService> MultiDBRoutingServiceRef;

}

#endif /* OSMSCOUT_MULTIDBROUTINGSERVICE_H */
