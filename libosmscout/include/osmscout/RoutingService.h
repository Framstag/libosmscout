#ifndef OSMSCOUT_ROUTINGSERVICE_H
#define OSMSCOUT_ROUTINGSERVICE_H

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

#include <functional>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <osmscout/CoreFeatures.h>

#include <osmscout/Point.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/RouteNode.h>

// Datafiles
#include <osmscout/DataFile.h>
#include <osmscout/Database.h>
#include <osmscout/ObjectVariantDataFile.h>

// Routing
#include <osmscout/Intersection.h>
#include <osmscout/Route.h>
#include <osmscout/RouteData.h>
#include <osmscout/RoutingProfile.h>

#include <osmscout/util/Cache.h>

namespace osmscout {

  typedef DataFile<RouteNode> RouteNodeDataFile;

  /**
   * \ingroup Routing
   * Database instance initialization parameter to influence the behavior of the database
   * instance.
   *
   * The following groups attributes are currently available:
   * - Switch for showing debug information
   */
  class OSMSCOUT_API RouterParameter
  {
  private:
    bool          debugPerformance;

  public:
    RouterParameter();

    void SetDebugPerformance(bool debug);

    bool IsDebugPerformance() const;
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
  class OSMSCOUT_API RoutingService
  {
  private:
    /**
     * A path in the routing graph from one node to the next (expressed via the target object)
     * with additional information as required by the A* algorithm.
     */
    struct RNode
    {
      FileOffset    nodeOffset;    //!< The file offset of the current route node
      RouteNodeRef  node;          //!< The current route node
      FileOffset    prev;          //!< The file offset of the previous route node
      ObjectFileRef object;        //!< The object (way/area) visited from the current route node

      double        currentCost;   //!< The cost of the current up to the current node
      double        estimateCost;  //!< The estimated cost from here to the target
      double        overallCost;   //!< The overall costs (currentCost+estimateCost)

      bool          access;        //!< Flags to signal, if we had access ("access restrictions") to this node

      RNode()
      : nodeOffset(0)
      {
        // no code
      }

      RNode(FileOffset nodeOffset,
            const RouteNodeRef& node,
            const ObjectFileRef& object)
      : nodeOffset(nodeOffset),
        node(node),
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
            const RouteNodeRef& node,
            const ObjectFileRef& object,
            FileOffset prev)
      : nodeOffset(nodeOffset),
        node(node),
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

    typedef std::shared_ptr<RNode> RNodeRef;

    struct RNodeCostCompare
    {
      inline bool operator()(const RNodeRef& a,
                             const RNodeRef& b) const
      {
        if (a->overallCost==b->overallCost) {
         return a->nodeOffset<b->nodeOffset;
        }
        else {
          return a->overallCost<b->overallCost;
        }
      }
    };

    /**
     * Minimum required data for a node in the ClosedSet.
     *
     * The ClosedSet is the set of routing nodes that have been
     * already handled.
     *
     * From the VNode list from the last routing node back to the start
     * the route is recalculated by following the previousNode chain.
     */
    struct VNode
    {
      FileOffset    currentNode;   //!< FileOffset of this route node
      FileOffset    previousNode;  //!< FileOffset of the previous route node
      ObjectFileRef object;        //!< The object (way/area) visited from the current route node

      /**
       * Equality operator
       * @param other
       *    Other object to compare against
       * @return
       *    True, if both objects are equal. Objects are currently equal
       *    if they have the same route node file offset.
       */
      inline bool operator==(const VNode& other) const
      {
        return currentNode==other.currentNode;
      }

      /**
       * Simple inline constructor for searching for VNodes in the
       * ClosedSet.
       *
       * @param currentNode
       *    Offset of the node to search for
       */
      inline VNode(FileOffset currentNode)
        : currentNode(currentNode),
          previousNode(0)
      {
        // no code
      }

      /**
       * Full featured constructor
       *
       * @param currentNode
       *    FileOffset of the current route node
       * @param object
       *    Type of object used to navigate to this route node
       * @param previousNode
       *    FileOffset of the previous route node visited
       */
      VNode(FileOffset currentNode,
                 const ObjectFileRef& object,
                 FileOffset previousNode)
      : currentNode(currentNode),
        previousNode(previousNode),
        object(object)
      {
        // no code
      }
    };

    /**
     * Helper class for calculating hash codes for
     * VNode instances to make it usable in std::unordered_set.
     */
    struct ClosedNodeHasher
    {
      inline size_t operator()(const VNode& node) const
      {
        return std::hash<FileOffset>()(node.currentNode);
      }
    };

    typedef std::set<RNodeRef,RNodeCostCompare>           OpenList;
    typedef std::set<RNodeRef,RNodeCostCompare>::iterator OpenListRef;

    typedef std::unordered_map<FileOffset,OpenListRef>    OpenMap;
    typedef std::unordered_set<VNode,ClosedNodeHasher>    ClosedSet;

  public:
    //! Relative filename of the intersection data file
    static const char* const FILENAME_INTERSECTIONS_DAT;
    //! Relative filename of the intersection index file
    static const char* const FILENAME_INTERSECTIONS_IDX;

    //! Relative filebase name for touting data as generated by default by the importer
    static const char* const DEFAULT_FILENAME_BASE;

  private:
    DatabaseRef                          database;              //!< Database object, holding all index and data files
    std::string                          filenamebase;          //!< Common base name for all router files
    AccessFeatureValueReader             accessReader;          //!< Read access information from objects
    bool                                 isOpen;                //!< true, if opened
    bool                                 debugPerformance;

    std::string                          path;                  //!< Path to the directory containing all files

    IndexedDataFile<Id,RouteNode>        routeNodeDataFile;     //!< Cached access to the 'route.dat' file
    IndexedDataFile<Id,Intersection>     junctionDataFile;      //!< Cached access to the 'junctions.dat' file
    ObjectVariantDataFile                objectVariantDataFile; //!< DataFile class for loadinfg object variant data

  private:
    std::string GetDataFilename(const std::string& filenamebase) const;
    std::string GetData2Filename(const std::string& filenamebase) const;
    std::string GetIndexFilename(const std::string& filenamebase) const;

    bool HasNodeWithId(const std::vector<Point>& nodes) const;

    void GetStartForwardRouteNode(const RoutingProfile& profile,
                                  const WayRef& way,
                                  size_t nodeIndex,
                                  RouteNodeRef& routeNode,
                                  size_t& routeNodeIndex);
    void GetStartBackwardRouteNode(const RoutingProfile& profile,
                                   const WayRef& way,
                                   size_t nodeIndex,
                                   RouteNodeRef& routeNode,
                                   size_t& routeNodeIndex);
    void GetTargetForwardRouteNode(const RoutingProfile& profile,
                                   const WayRef& way,
                                   size_t nodeIndex,
                                   RouteNodeRef& routeNode);
    void GetTargetBackwardRouteNode(const RoutingProfile& profile,
                                    const WayRef& way,
                                    size_t nodeIndex,
                                    RouteNodeRef& routeNode);

    bool GetStartNodes(const RoutingProfile& profile,
                       const ObjectFileRef& object,
                       size_t nodeIndex,
                       GeoCoord& targetCoord,
                       RouteNodeRef& forwardRouteNode,
                       RouteNodeRef& backwardRouteNode,
                       RNodeRef& forwardRNode,
                       RNodeRef& backwardRNode);

    bool GetTargetNodes(const RoutingProfile& profile,
                        const ObjectFileRef& object,
                        size_t nodeIndex,
                        GeoCoord& targetCoord,
                        RouteNodeRef& forwardNode,
                        RouteNodeRef& backwardNode);

    void ResolveRNodeChainToList(FileOffset finalRouteNode,
                                 const ClosedSet& closedSet,
                                 std::list<VNode>& nodes);
    bool ResolveRNodesToRouteData(const RoutingProfile& profile,
                                  const std::list<VNode>& nodes,
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
    RoutingService(const DatabaseRef& database,
                   const RouterParameter& parameter,
                   const std::string& filenamebase);
    virtual ~RoutingService();

    bool Open();
    bool IsOpen() const;
    void Close();

    TypeConfigRef GetTypeConfig() const;

    bool CalculateRoute(const RoutingProfile& profile,
                        const ObjectFileRef& startObject,
                        size_t startNodeIndex,
                        const ObjectFileRef& targetObject,
                        size_t targetNodeIndex,
                        RouteData& route);

    bool CalculateRoute(const RoutingProfile& profile,
                        Vehicle vehicle,
                        double radius,
                        std::vector<GeoCoord> via,
                        RouteData& route);

    bool TransformRouteDataToWay(const RouteData& data,
                                 Way& way);

    bool TransformRouteDataToPoints(const RouteData& data,
                                    std::list<Point>& points);

    bool TransformRouteDataToRouteDescription(const RouteData& data,
                                              RouteDescription& description);

#ifdef SWIG
    %apply ObjectFileRef& OUTPUT {ObjectFileRef& object};
    %apply long& OUTPUT {size_t& nodeIndex};
#endif
    bool GetClosestRoutableNode(const GeoCoord& coord,
                                const RoutingProfile& profile,
                                const Vehicle& vehicle,
                                double radius,
                                ObjectFileRef& object,
                                size_t& nodeIndex) const;

    void DumpStatistics();
  };

  //! \ingroup Service
  //! Reference counted reference to an RoutingService instance
  typedef std::shared_ptr<RoutingService> RoutingServiceRef;

  /**
   * \defgroup Routing Routing based data structures and services
   * Classes and methods for handling routing aspects of object in the libosmscout database
   */
}

#endif
