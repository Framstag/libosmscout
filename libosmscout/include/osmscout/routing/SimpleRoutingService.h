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
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/RoutingDB.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/RoutingService.h>
#include <osmscout/routing/AbstractRoutingService.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/Cache.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class SimpleRoutingService;

  class OSMSCOUT_API ClosestRoutableObjectResult
  {
  private:
    ObjectFileRef object;
    Distance      distance;
    WayRef        way;
    AreaRef       area;
    std::string   name;

  public:
    friend SimpleRoutingService;

    ObjectFileRef GetObject() const
    {
      return object;
    }

    Distance GetDistance() const
    {
      return distance;
    }

    WayRef GetWay() const
    {
      return way;
    }

    AreaRef GetArea() const
    {
      return area;
    }

    std::string GetName() const
    {
      return name;
    }
  };

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

    RoutingDatabase                      routingDatabase;       //!< Access to routing data and index files

  private:
    bool HasNodeWithId(const std::vector<Point>& nodes) const;

  protected:
    Vehicle GetVehicle(const RoutingProfile& profile) override;

    bool CanUse(const RoutingProfile& profile,
                DatabaseId database,
                const RouteNode& routeNode,
                size_t pathIndex) override;

    bool CanUseForward(const RoutingProfile& profile,
                       const DatabaseId& database,
                       const WayRef& way) override;

    bool CanUseBackward(const RoutingProfile& profile,
                        const DatabaseId& database,
                        const WayRef& way) override;

    double GetCosts(const RoutingProfile& profile,
                    DatabaseId database,
                    const RouteNode& routeNode,
                    size_t inPathIndex,
                    size_t outPathIndex) override;

    double GetCosts(const RoutingProfile& profile,
                    DatabaseId database,
                    const WayRef &way,
                    const Distance &wayLength) override;

    double GetEstimateCosts(const RoutingProfile& profile,
                            DatabaseId database,
                            const Distance &targetDistance) override;

    double GetCostLimit(const RoutingProfile& profile,
                        DatabaseId database,
                        const Distance &targetDistance) override;

    std::string GetCostString(const RoutingProfile& profile,
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

    std::vector<DBId> GetNodeTwins(const RoutingProfile& state,
                                   DatabaseId database,
                                   Id id) override;

  public:
    SimpleRoutingService(const DatabaseRef& database,
                         const RouterParameter& parameter,
                         const std::string& filenamebase);
    ~SimpleRoutingService() override;

    bool Open();
    bool IsOpen() const;
    void Close();

    TypeConfigRef GetTypeConfig() const;

    RoutingResult CalculateRouteViaCoords(RoutingProfile& profile,
                                          const std::vector<GeoCoord>& via,
                                          const Distance &radius,
                                          const RoutingParameter& parameter);

    RoutePositionResult GetClosestRoutableNode(const GeoCoord& coord,
                                               const RoutingProfile& profile,
                                               const Distance &radius) const;

    ClosestRoutableObjectResult GetClosestRoutableObject(const GeoCoord& location,
                                                         Vehicle vehicle,
                                                         const Distance &maxRadius);

    std::map<DatabaseId, std::string> GetDatabaseMapping() const override;

    void DumpStatistics();
  };

  //! \ingroup Service
  //! Reference counted reference to an RoutingService instance
  using SimpleRoutingServiceRef = std::shared_ptr<SimpleRoutingService>;

  /**
   * \defgroup Routing Routing based data structures and services
   * Classes and methods for handling routing aspects of object in the libosmscout database
   */
}

#endif
