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
    using RoutingProfileBuilder = std::function<RoutingProfileRef (const DatabaseRef &)>;

  private:
    std::vector<DatabaseHandle> handles;
    bool                        isOpen;

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
                    size_t inPathIndex,
                    size_t outPathIndex) override;

    double GetCosts(const MultiDBRoutingState& state,
                    DatabaseId database,
                    const WayRef &way,
                    const Distance &wayLength) override;

    double GetEstimateCosts(const MultiDBRoutingState& state,
                            DatabaseId database,
                            const Distance &targetDistance) override;

    double GetCostLimit(const MultiDBRoutingState& state,
                        DatabaseId database,
                        const Distance &targetDistance) override;

    std::string GetCostString(const MultiDBRoutingState& profile,
                              DatabaseId database,
                              double cost) const override;

    bool GetRouteNodes(const std::set<DBId> &routeNodeIds,
                       std::unordered_map<DBId,RouteNodeRef> &routeNodeMap) override;

    bool GetRouteNode(const DBId &id,
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

    std::vector<DBId> GetNodeTwins(const MultiDBRoutingState& state,
                                   DatabaseId database,
                                   Id id) override;

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

    RoutePositionResult GetClosestRoutableNode(const GeoCoord &coord,
                                               const Distance &radius=Kilometers(1)) const;

    RoutingResult CalculateRoute(const RoutePosition &start,
                                 const RoutePosition &target,
                                 const RoutingParameter &parameter);

    RoutingResult CalculateRoute(std::vector<osmscout::GeoCoord> via,
                                 const Distance &radius,
                                 const RoutingParameter& parameter);

    RouteDescriptionResult TransformRouteDataToRouteDescription(const RouteData& data);

    RoutePointsResult TransformRouteDataToPoints(const RouteData& data);

    RouteWayResult TransformRouteDataToWay(const RouteData& data);

    bool PostProcessRouteDescription(RouteDescription &description,
                                     const std::list<RoutePostprocessor::PostprocessorRef> &postprocessors);

    std::map<DatabaseId, std::string> GetDatabaseMapping() const override;
  };

  //! \ingroup Service
  //! Reference counted reference to an RoutingService instance
  using MultiDBRoutingServiceRef = std::shared_ptr<MultiDBRoutingService>;

}

#endif /* OSMSCOUT_MULTIDBROUTINGSERVICE_H */
