#ifndef OSMSCOUT_ROUTER_H
#define OSMSCOUT_ROUTER_H

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
#include <set>

#include <osmscout/CoreFeatures.h>

#include <osmscout/Point.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/RouteNode.h>

// Datafiles
#include <osmscout/WayDataFile.h>

// Routing
#include <osmscout/Intersection.h>
#include <osmscout/Route.h>
#include <osmscout/RouteData.h>
#include <osmscout/RoutingProfile.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  typedef DataFile<RouteNode> RouteNodeDataFile;

  /**
    Database instance initialisation parameter to influence the behaviour of the database
    instance.

    The following groups attributes are currently available:
    * cache sizes.
    */
  class OSMSCOUT_API RouterParameter
  {
  private:
    unsigned long wayIndexCacheSize;
    unsigned long wayCacheSize;

    bool          debugPerformance;

  public:
    RouterParameter();

    void SetWayIndexCacheSize(unsigned long wayIndexCacheSize);
    void SetWayCacheSize(unsigned long wayCacheSize);

    void SetDebugPerformance(bool debug);

    unsigned long GetWayIndexCacheSize() const;
    unsigned long GetWayCacheSize() const;

    bool IsDebugPerformance() const;
  };

  class OSMSCOUT_API Router : public Referencable
  {
  private:
    /**
     * A path in the routing graph from one node to the next (expressed via the target object)
     * with additional information as required by the A* algorithm.
     */
    struct RNode : public Referencable
    {
      FileOffset    nodeOffset;    //! The file offset of the current route node
      FileOffset    prev;          //! The file offset of the previous route node
      ObjectFileRef object;        //! The object (way/area) visited from the current route node

      double        currentCost;   //! The cost of the current up to the current node
      double        estimateCost;  //! The estimated cost from here to the target
      double        overallCost;   //! The overall costs (currentCost+estimateCost)

      bool          access;        //! Flags to signal, if we had access ("access restrictions") to this node

      RNode()
      : nodeOffset(0)
      {
        // no code
      }

      RNode(FileOffset nodeOffset,
            const ObjectFileRef& object)
      : nodeOffset(nodeOffset),
        prev(0),
        object(object),
        currentCost(0),
        estimateCost(0),
        overallCost(0),
        access(true)
      {
        // no code
      }

      RNode(FileOffset nodeOffset,
            const ObjectFileRef& object,
            FileOffset prev)
      : nodeOffset(nodeOffset),
        prev(prev),
        object(object),
        currentCost(0),
        estimateCost(0),
        overallCost(0),
        access(true)
      {
        // no code
      }

      inline bool operator==(const RNode& other)
      {
        return nodeOffset==other.nodeOffset;
      }

      inline bool operator<(const RNode& other) const
      {
        return nodeOffset<other.nodeOffset;
      }
    };

    typedef Ref<RNode> RNodeRef;

    struct RNodeCostCompare
    {
      inline bool operator()(const RNodeRef& a, const RNodeRef& b) const
      {
        if (a->overallCost==b->overallCost) {
         return a->nodeOffset<b->nodeOffset;
        }
        else {
          return a->overallCost<b->overallCost;
        }
      }
    };

    typedef std::set<RNodeRef,RNodeCostCompare>              OpenList;
    typedef std::set<RNodeRef,RNodeCostCompare>::iterator    OpenListRef;

    typedef OSMSCOUT_HASHMAP<FileOffset,Router::OpenListRef> OpenMap;
    typedef OSMSCOUT_HASHMAP<FileOffset,Router::RNodeRef>    CloseMap;

  public:
    static const char* const FILENAME_INTERSECTIONS_DAT;
    static const char* const FILENAME_INTERSECTIONS_IDX;

    static const char* const FILENAME_FOOT_DAT;
    static const char* const FILENAME_FOOT_IDX;

    static const char* const FILENAME_BICYCLE_DAT;
    static const char* const FILENAME_BICYCLE_IDX;

    static const char* const FILENAME_CAR_DAT;
    static const char* const FILENAME_CAR_IDX;

  private:
    Vehicle                              vehicle;           //! We are a router for this vehicle
    bool                                 isOpen;            //! true, if opened
    bool                                 debugPerformance;

    std::string                          path;              //! Path to the directory containing all files

    DataFile<Area>                       areaDataFile;      //! Cached access to the 'areas.dat' file
    DataFile<Way>                        wayDataFile;       //! Cached access to the 'ways.dat' file
    IndexedDataFile<Id,RouteNode>        routeNodeDataFile; //! Cached access to the 'route.dat' file
    IndexedDataFile<Id,Intersection>     junctionDataFile;  //! Cached access to the 'junctions.dat' file

    TypeConfig                           *typeConfig;       //! Type config for the currently opened map

  private:
    std::string GetDataFilename(Vehicle vehicle) const;
    std::string GetIndexFilename(Vehicle vehicle) const;

    void GetClosestForwardRouteNode(const WayRef& way,
                                    size_t nodeIndex,
                                    RouteNodeRef& routeNode,
                                    size_t& routeNodeIndex);
    void GetClosestBackwardRouteNode(const WayRef& way,
                                     size_t nodeIndex,
                                     RouteNodeRef& routeNode,
                                     size_t& routeNodeIndex);

    bool GetStartNodes(const RoutingProfile& profile,
                       const ObjectFileRef& object,
                       size_t nodeIndex,
                       double& targetLon,
                       double& targetLat,
                       RouteNodeRef& forwardRouteNode,
                       RouteNodeRef& backwardRouteNode,
                       RNodeRef& forwardRNode,
                       RNodeRef& backwardRNode);

    bool GetTargetNodes(const ObjectFileRef& object,
                        size_t nodeIndex,
                        double& targetLon,
                        double& targetLat,
                        RouteNodeRef& forwardNode,
                        RouteNodeRef& backwardNode);

    void ResolveRNodeChainToList(const RNodeRef& end,
                                 const CloseMap& closeMap,
                                 std::list<RNodeRef>& nodes);
    bool ResolveRNodesToRouteData(const RoutingProfile& profile,
                                  const std::list<RNodeRef>& nodes,
                                  const ObjectFileRef& startObject,
                                  size_t startNodeIndex,
                                  const ObjectFileRef& targetObject,
                                  size_t targetNodeIndex,
                                  RouteData& route);

    bool ResolveRouteDataJunctions(RouteData& route);

    void AddNodes(RouteData& route,
                  Id startNodeId,
                  size_t startNodeIndex,
                  const ObjectFileRef& object,
                  size_t idCount,
                  bool oneway,
                  size_t targetNodeIndex);

  public:
    Router(const RouterParameter& parameter,
           Vehicle vehicle);
    virtual ~Router();

    Vehicle GetVehicle() const;

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    void FlushCache();

    TypeConfig* GetTypeConfig() const;

    bool CalculateRoute(const RoutingProfile& profile,
                        const ObjectFileRef& startObject,
                        size_t startNodeIndex,
                        const ObjectFileRef& targetObject,
                        size_t targetNodeIndex,
                        RouteData& route);

    bool TransformRouteDataToWay(const RouteData& data,
                                 Way& way);

    bool TransformRouteDataToPoints(const RouteData& data,
                                    std::list<Point>& points);

    bool TransformRouteDataToRouteDescription(const RouteData& data,
                                              RouteDescription& description);

    void DumpStatistics();
  };

  typedef Ref<Router> RouterRef;
}

#endif
