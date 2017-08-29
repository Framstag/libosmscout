#ifndef OSMSCOUT_SIMPLEROUTINGSERVICE_H
#define OSMSCOUT_SIMPLEROUTINGSERVICE_H

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

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <osmscout/CoreFeatures.h>

#include <osmscout/Point.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/routing/RouteNode.h>

// Datafiles
#include <osmscout/DataFile.h>
#include <osmscout/Database.h>
#include <osmscout/ObjectVariantDataFile.h>

// Routing
#include <osmscout/Intersection.h>
#include <osmscout/routing/Route.h>
#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/AbstractRoutingService.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/Cache.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {


  /**
   * \ingroup Service
   * \ingroup Routing
   * The RoutingService implements functionality in the context of routing.
   * The following functions are available:
   * - Calculation of a route from a start node to a target node
   * - Transformation of the resulting route to a Way
   * - Transformation of the resulting route to a simple list of points
   * - Transformation of the resulting route to a routing description with is the base
   * for further transformations to a textual or visual description of the route
   * - Returning the closest routeable node to  given geolocation
   */
  class OSMSCOUT_API SimpleRoutingService: public AbstractRoutingService<RoutingProfile>
  {

  private:
    DatabaseRef                          database;              //!< Database object, holding all index and data files
    std::string                          filenamebase;          //!< Common base name for all router files
    AccessFeatureValueReader             accessReader;          //!< Read access information from objects
    bool                                 isOpen;                //!< true, if opened    

    std::string                          path;                  //!< Path to the directory containing all files

    IndexedDataFile<Id,RouteNode>        routeNodeDataFile;     //!< Cached access to the 'route.dat' file
    IndexedDataFile<Id,Intersection>     junctionDataFile;      //!< Cached access to the 'junctions.dat' file
    ObjectVariantDataFile                objectVariantDataFile; //!< DataFile class for loading object variant data

  protected:
    virtual Vehicle GetVehicle(const RoutingProfile& profile);

    virtual bool CanUse(const RoutingProfile& profile,
                        const DatabaseId database,
                        const RouteNode& routeNode,
                        size_t pathIndex);

    virtual bool CanUseForward(const RoutingProfile& profile,
                               const DatabaseId& database,
                               const WayRef& way);

    virtual bool CanUseBackward(const RoutingProfile& profile,
                                const DatabaseId& database,
                                const WayRef& way);

    virtual double GetCosts(const RoutingProfile& profile,
                            const DatabaseId database,
                            const RouteNode& routeNode,
                            size_t pathIndex);

    virtual double GetCosts(const RoutingProfile& profile,
                            const DatabaseId database,
                            const WayRef &way,
                            double wayLength);

    virtual double GetEstimateCosts(const RoutingProfile& profile,
                                    const DatabaseId database,
                                    double targetDistance);

    virtual double GetCostLimit(const RoutingProfile& profile,
                                const DatabaseId database,
                                double targetDistance);

    virtual bool GetRouteNode(const DatabaseId &database,
                              const Id &id,
                              RouteNodeRef &node);

    virtual bool GetRouteNodesByOffset(const std::set<DBFileOffset> &routeNodeOffsets,
                                       std::unordered_map<DBFileOffset,RouteNodeRef> &routeNodeMap);

    virtual bool GetRouteNodeByOffset(const DBFileOffset &offset,
                                      RouteNodeRef &node);

    virtual bool GetRouteNodeOffset(const DatabaseId &database,
                                    const Id &id,
                                    FileOffset &offset);

    bool HasNodeWithId(const std::vector<Point>& nodes) const;

    virtual bool GetWayByOffset(const DBFileOffset &offset,
                                WayRef &way);

    virtual bool GetWaysByOffset(const std::set<DBFileOffset> &wayOffsets,
                                 std::unordered_map<DBFileOffset,WayRef> &wayMap);

    virtual bool GetAreaByOffset(const DBFileOffset &offset,
                                 AreaRef &area);

    virtual bool GetAreasByOffset(const std::set<DBFileOffset> &areaOffsets,
                                  std::unordered_map<DBFileOffset,AreaRef> &areaMap);

    virtual bool ResolveRouteDataJunctions(RouteData& route);

    virtual std::vector<DBFileOffset> GetNodeTwins(const RoutingProfile& state,
                                                   const DatabaseId database,
                                                   const Id id);
  public:
    SimpleRoutingService(const DatabaseRef& database,
                         const RouterParameter& parameter,
                         const std::string& filenamebase);
    virtual ~SimpleRoutingService();

    bool Open();
    bool IsOpen() const;
    void Close();

    TypeConfigRef GetTypeConfig() const;

    /**
     * Calculate a route
     *
     * @param profile
     *    Profile to use
     * @param start
     *    Start of the route
     * @param target
     *    Target of teh route
     * @param progress
     *    Optional callback for handling routing progress
     * @param route
     *    The route object holding the resulting route on success
     * @return
     *    True, if the engine was able to find a route, else false
     */
    virtual RoutingResult CalculateRoute(RoutingProfile& profile,
                                         const RoutePosition& start,
                                         const RoutePosition& target,
                                         const RoutingParameter& parameter);

    RoutingResult CalculateRoute(RoutingProfile& profile,
                                 std::vector<GeoCoord> via,
                                 double radius,
                                 const RoutingParameter& parameter);

    RoutePosition GetClosestRoutableNode(const GeoCoord& coord,
                                         const RoutingProfile& profile,
                                         double& radius) const;

    void DumpStatistics();
  };

  //! \ingroup Service
  //! Reference counted reference to an RoutingService instance
  typedef std::shared_ptr<SimpleRoutingService> SimpleRoutingServiceRef;

  /**
   * \defgroup Routing Routing based data structures and services
   * Classes and methods for handling routing aspects of object in the libosmscout database
   */
}

#endif
