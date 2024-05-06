#ifndef OSMSCOUT_ABSTRACTROUTINGSERVICE_H
#define OSMSCOUT_ABSTRACTROUTINGSERVICE_H

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

#include <functional>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <osmscout/lib/CoreFeatures.h>

#include <osmscout/TypeConfig.h>
#include <osmscout/Point.h>
#include <osmscout/Pixel.h>

#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/RouteNode.h>
#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/MultiDBRoutingState.h>

namespace osmscout {

  /**
   * Result of a routing calculation. This object is always returned.
   * In case of an routing error it however may not contain a valid route
   * (route is empty).
   *
   * TODO: Adapt it to the same style as RoutePointsResult and Co.
   */
  class OSMSCOUT_API RoutingResult CLASS_FINAL
  {
  private:
    RouteData route;
    Distance  currentMaxDistance;
    Distance  overallDistance;
    std::vector<int> sectionLengths;

  public:
    RoutingResult();

    void SetOverallDistance(const Distance &overallDistance)
    {
      this->overallDistance=overallDistance;
    }

    void SetCurrentMaxDistance(const Distance &currentMaxDistance)
    {
      this->currentMaxDistance=currentMaxDistance;
    }

    Distance GetOverallDistance() const
    {
      return overallDistance;
    }

    Distance GetCurrentMaxDistance() const
    {
      return currentMaxDistance;
    }

    RouteData& GetRoute()
    {
      return route;
    }

    const RouteData& GetRoute() const
    {
      return route;
    }

    bool Success() const
    {
      return !route.IsEmpty();
    }
      
    const std::vector<int>& GetSectionLenghts() const
    {
      return sectionLengths;
    }
      
    void AppendSectionLength(int length)
    {
      this->sectionLengths.push_back(length);
    }

    void ClearSectionLengths()
    {
      this->sectionLengths.clear();
    }
  };

  struct OSMSCOUT_API RoutePoints
  {
    const std::vector<Point> points;

    explicit RoutePoints(const std::list<Point>& points);
  };

  using RoutePointsRef = std::shared_ptr<RoutePoints>;

  class OSMSCOUT_API RoutePointsResult CLASS_FINAL
  {
  private:
    bool           success;
    RoutePointsRef points;

  public:
    RoutePointsResult();
    explicit RoutePointsResult(const RoutePointsRef& points);

    bool Success() const
    {
      return success;
    }

    RoutePointsRef GetPoints() const
    {
      return points;
    }
  };

  class OSMSCOUT_API RouteDescriptionResult CLASS_FINAL
  {
  private:
    bool                success;
    RouteDescriptionRef description;

  public:
    RouteDescriptionResult();
    explicit RouteDescriptionResult(const RouteDescriptionRef& description);

    bool Success() const
    {
      return success;
    }

    RouteDescriptionRef GetDescription() const
    {
      return description;
    }
  };

  class OSMSCOUT_API RouteWayResult CLASS_FINAL
  {
  private:
    bool   success;
    WayRef way;

  public:
    RouteWayResult();
    explicit RouteWayResult(const WayRef& way);

    bool Success() const
    {
      return success;
    }

    WayRef GetWay() const
    {
      return way;
    }
  };

  /**
   * \ingroup Routing
   *
   * Abstract algorithms for routing
   */
  template <class RoutingState>
  class OSMSCOUT_API AbstractRoutingService: public RoutingService
  {
  protected:
    bool debugPerformance;

  protected:
    virtual Vehicle GetVehicle(const RoutingState& state) = 0;

    virtual bool CanUse(const RoutingState& state,
                        DatabaseId database,
                        const RouteNode& routeNode,
                        size_t pathIndex) = 0;

    virtual bool CanUseForward(const RoutingState& state,
                               const DatabaseId& database,
                               const WayRef& way) = 0;

    virtual bool CanUseBackward(const RoutingState& state,
                                const DatabaseId& database,
                                const WayRef& way) = 0;

    virtual double GetCosts(const RoutingState& state,
                            DatabaseId database,
                            const RouteNode& routeNode,
                            size_t inPathIndex,
                            size_t outPathIndex) = 0;

    virtual double GetUTurnCost(const RoutingState& state, const DatabaseId databaseId) = 0;

    virtual double GetCosts(const RoutingState& state,
                            DatabaseId database,
                            const WayRef &way,
                            const Distance &wayLength) = 0;

    virtual double GetEstimateCosts(const RoutingState& state,
                                    DatabaseId database,
                                    const Distance &targetDistance) = 0;

    virtual double GetCostLimit(const RoutingState& state,
                                DatabaseId database,
                                const Distance &targetDistance) = 0;

    virtual std::string GetCostString(const RoutingState& state,
                                      DatabaseId database,
                                      double cost) const = 0;

    virtual bool GetRouteNodes(const std::set<DBId> &routeNodeIds,
                               std::unordered_map<DBId,RouteNodeRef> &routeNodeMap) = 0;

    /**
     * Return the route node for the given db offset
     * @param id
     *    Database and Offset in given db
     * @param node
     *    Node instance to write the result back
     * @return
     *    True, if the node could be loaded, else false
     */
    virtual bool GetRouteNode(const DBId &id,
                              RouteNodeRef &node) = 0;

    virtual bool GetWayByOffset(const DBFileOffset &offset,
                                WayRef &way) = 0;

    virtual bool GetWaysByOffset(const std::set<DBFileOffset> &wayOffsets,
                                 std::unordered_map<DBFileOffset,WayRef> &wayMap) = 0;

    virtual bool GetAreaByOffset(const DBFileOffset &offset,
                                 AreaRef &area) = 0;

    virtual bool GetAreasByOffset(const std::set<DBFileOffset> &areaOffsets,
                                  std::unordered_map<DBFileOffset,AreaRef> &areaMap) = 0;

    void ResolveRNodeChainToList(const RNode &finalRouteNode,
                                 const ClosedSet& closedSet,
                                 std::list<VNode>& nodes);

    virtual bool ResolveRouteDataJunctions(RouteData& route) = 0;

    virtual std::vector<DBId> GetNodeTwins(const RoutingState& state,
                                           DatabaseId database,
                                           Id id) = 0;

    void GetStartForwardRouteNode(const RoutingState& state,
                                  const DatabaseId& database,
                                  const WayRef& way,
                                  size_t nodeIndex,
                                  RouteNodeRef& routeNode,
                                  size_t& routeNodeIndex);
    void GetStartBackwardRouteNode(const RoutingState& state,
                                   const DatabaseId& database,
                                   const WayRef& way,
                                   size_t nodeIndex,
                                   RouteNodeRef& routeNode,
                                   size_t& routeNodeIndex);
    void GetTargetForwardRouteNode(const RoutingState& state,
                                   const DatabaseId& database,
                                   const WayRef& way,
                                   size_t nodeIndex,
                                   RouteNodeRef& routeNode);
    void GetTargetBackwardRouteNode(const RoutingState& state,
                                    const DatabaseId& database,
                                    const WayRef& way,
                                    size_t nodeIndex,
                                    RouteNodeRef& routeNode);

    bool GetStartNodes(const RoutingState& state,
                       const RoutePosition& position,
                       GeoCoord& startCoord,
                       const GeoCoord& targetCoord,
                       RouteNodeRef& forwardRouteNode,
                       RouteNodeRef& backwardRouteNode,
                       RNodeRef& forwardRNode,
                       RNodeRef& backwardRNode);

    bool GetWayTargetNodes(const RoutingState& state,
                           const RoutePosition& position,
                           GeoCoord& targetCoord,
                           RouteNodeRef& forwardNode,
                           RouteNodeRef& backwardNode);

    bool GetTargetNodes(const RoutingState& state,
                        const RoutePosition& position,
                        GeoCoord& targetCoord,
                        RouteNodeRef& forwardNode,
                        RouteNodeRef& backwardNode);

    bool GetRNode(const RoutingState& state,
                  const RoutePosition& position,
                  const WayRef& way,
                  size_t routeNodeIndex,
                  const RouteNodeRef& routeNode,
                  const GeoCoord& startCoord,
                  const GeoCoord& targetCoord,
                  RNodeRef& node);

    void AddNodes(RouteData& route,
                  DatabaseId database,
                  Id startNodeId,
                  size_t startNodeIndex,
                  const ObjectFileRef& object,
                  size_t idCount,
                  bool oneway,
                  size_t targetNodeIndex);

    bool GetWayStartNodes(const RoutingState& state,
                          const RoutePosition& position,
                          GeoCoord& startCoord,
                          const GeoCoord& targetCoord,
                          RouteNodeRef& forwardRouteNode,
                          RouteNodeRef& backwardRouteNode,
                          RNodeRef& forwardRNode,
                          RNodeRef& backwardRNode);

    bool ResolveRNodesToRouteData(const RoutingState& state,
                                  const std::list<VNode>& nodes,
                                  const RoutePosition& start,
                                  const RoutePosition& target,
                                  RouteData& route);

    virtual bool WalkToOtherDatabases(const RoutingState& state,
                                      RNodeRef &current,
                                      RouteNodeRef &currentRouteNode,
                                      OpenList &openList,
                                      OpenMap &openMap,
                                      const ClosedSet &closedSet);

    virtual bool WalkPaths(const RoutingState& state,
                           RNodeRef &current,
                           RouteNodeRef &currentRouteNode,
                           OpenList &openList,
                           OpenMap &openMap,
                           ClosedSet &closedSet,
                           RoutingResult &result,
                           const RoutingParameter& parameter,
                           const GeoCoord &targetCoord,
                           const Vehicle &vehicle,
                           size_t &nodesIgnoredCount,
                           Distance &currentMaxDistance,
                           const Distance &overallDistance,
                           const double &costLimit);

    bool RestrictInitialUTurn(const RoutingState& state,
                              const Bearing& vehicleBearing,
                              const RoutePosition& start,
                              RNodeRef startForwardNode,
                              RNodeRef startBackwardNode);

  public:
    explicit AbstractRoutingService(const RouterParameter& parameter);
    ~AbstractRoutingService() override;

    RoutingResult CalculateRoute(RoutingState& state,
                                 const RoutePosition& start,
                                 const RoutePosition& target,
                                 const std::optional<osmscout::Bearing> &bearing,
                                 const RoutingParameter& parameter);

    RouteDescriptionResult TransformRouteDataToRouteDescription(const RouteData& data);
    RoutePointsResult TransformRouteDataToPoints(const RouteData& data);
    RouteWayResult TransformRouteDataToWay(const RouteData& data);

    /**
     * Get current mapping of DatabaseId to db path than be used
     * later for lookup objects in description
     *
     * @return
     */
    virtual std::map<DatabaseId, std::string> GetDatabaseMapping() const = 0;
  };

}

#endif /* OSMSCOUT_ABSTRACTROUTINGSERVICE_H */
