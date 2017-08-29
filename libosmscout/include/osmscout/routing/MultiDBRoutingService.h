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

namespace osmscout {

  /**
   * \ingroup Routing
   *
   * Helper container for MultiDBRoutingService
   */
  class RouterDBFiles{
  public:
    IndexedDataFile<Id,RouteNode>       routeNodeDataFile;     //!< Cached access to the 'route.dat' file
    IndexedDataFile<Id,Intersection>    junctionDataFile;      //!< Cached access to the 'junctions.dat' file
    ObjectVariantDataFile               objectVariantDataFile;
  public:
    RouterDBFiles();
    virtual ~RouterDBFiles();
    bool Open(DatabaseRef database);
    void Close();
  };

  typedef std::shared_ptr<RouterDBFiles> RouterDBFilesRef;

  /**
   * \ingroup Routing
   *
   * Util class for routing cross databases
   */
  class OSMSCOUT_API MultiDBRoutingService CLASS_FINAL : public AbstractRoutingService<MultiDBRoutingState>
  {
  private:

  public:
    typedef std::function<RoutingProfileRef(const DatabaseRef&)> RoutingProfileBuilder;

  private:
    static const double CELL_MAGNIFICATION;
    static const double LAT_CELL_FACTOR;
    static const double LON_CELL_FACTOR;

  private:
    std::map<std::string,DatabaseId>              databaseMap;

    std::map<DatabaseId,DatabaseRef>              databases;
    std::map<DatabaseId,SimpleRoutingServiceRef>  services;
    std::map<DatabaseId,RoutingProfileRef>        profiles;

    std::map<DatabaseId,RouterDBFilesRef>         routerFiles;

    bool  isOpen;

  private:
    virtual Vehicle GetVehicle(const MultiDBRoutingState& state);

    virtual bool CanUseForward(const MultiDBRoutingState& state,
                               const DatabaseId& database,
                               const WayRef& way);

    virtual bool CanUseBackward(const MultiDBRoutingState& state,
                                const DatabaseId& database,
                                const WayRef& way);

    virtual double GetCosts(const MultiDBRoutingState& state,
                            const DatabaseId database,
                            const RouteNode& routeNode,
                            size_t pathIndex);

    virtual double GetCosts(const MultiDBRoutingState& state,
                            const DatabaseId database,
                            const WayRef &way,
                            double wayLength);

    virtual double GetEstimateCosts(const MultiDBRoutingState& state,
                                    const DatabaseId database,
                                    double targetDistance);

    virtual double GetCostLimit(const MultiDBRoutingState& state,
                                const DatabaseId database,
                                double targetDistance);

    virtual bool GetRouteNodesByOffset(const std::set<DBFileOffset> &routeNodeOffsets,
                                       std::unordered_map<DBFileOffset,RouteNodeRef> &routeNodeMap);

    virtual bool GetRouteNodeByOffset(const DBFileOffset &offset,
                                      RouteNodeRef &node);

    virtual bool GetRouteNodeOffset(const DatabaseId &database,
                                    const Id &id,
                                    FileOffset &offset);

    virtual bool GetWayByOffset(const DBFileOffset &offset,
                                WayRef &way);

    virtual bool GetWaysByOffset(const std::set<DBFileOffset> &wayOffsets,
                                 std::unordered_map<DBFileOffset,WayRef> &wayMap);

    virtual bool GetAreaByOffset(const DBFileOffset &offset,
                                 AreaRef &area);

    virtual bool GetAreasByOffset(const std::set<DBFileOffset> &areaOffsets,
                                  std::unordered_map<DBFileOffset,AreaRef> &areaMap);

    virtual bool ResolveRouteDataJunctions(RouteData& route);

    virtual std::vector<DBFileOffset> GetNodeTwins(const MultiDBRoutingState& state,
                                                   const DatabaseId database,
                                                   const Id id);

    virtual bool GetRouteNode(const DatabaseId &database,
                              const Id &id,
                              RouteNodeRef &node);
    
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

    virtual bool CanUse(const MultiDBRoutingState& state,
                        const DatabaseId database,
                        const RouteNode& routeNode,
                        size_t pathIndex);

  public:
    MultiDBRoutingService(const RouterParameter& parameter,
                          const std::vector<DatabaseRef> &databases);

    virtual ~MultiDBRoutingService();

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
