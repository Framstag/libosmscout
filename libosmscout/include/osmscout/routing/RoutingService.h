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

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <osmscout/lib/CoreFeatures.h>

#include <osmscout/Point.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/routing/RouteNode.h>

// Datafiles
#include <osmscout/io/DataFile.h>

#include <osmscout/db/Database.h>
#include <osmscout/db/ObjectVariantDataFile.h>

// Routing
#include <osmscout/Intersection.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/RouteNodeDataFile.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/DBFileOffset.h>

#include <osmscout/async/Breaker.h>
#include <osmscout/util/Cache.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \ingroup Routing
   *
   * Start or end position of a route calculation
   */
  class OSMSCOUT_API RoutePosition CLASS_FINAL
  {
  private:
    ObjectFileRef object;
    size_t        nodeIndex=0;
    DatabaseId    database;

  public:
    RoutePosition() = default;
    RoutePosition(const ObjectFileRef& object,
                  size_t nodeIndex,
                  DatabaseId database);

    bool IsValid() const
    {
      return object.Valid();
    }

    ObjectFileRef GetObjectFileRef() const
    {
      return object;
    }

    size_t GetNodeIndex() const
    {
      return nodeIndex;
    }

    DatabaseId GetDatabaseId() const
    {
      return database;
    }
  };

  class OSMSCOUT_API RoutePositionResult CLASS_FINAL
  {
  private:
    RoutePosition routePosition;
    Distance distance;

  public:
    RoutePositionResult();

    RoutePositionResult(const RoutePosition &routePosition, const Distance &distance);

    RoutePosition GetRoutePosition() const
    {
      return routePosition;
    }

    Distance GetDistance() const
    {
      return distance;
    }

    bool IsValid() const
    {
      return routePosition.IsValid();
    }
  };


  /**
   * \ingroup Routing
   *
   * Database instance initialization parameter to influence the behavior of the db
   * instance.
   *
   * The following groups attributes are currently available:
   * - Switch for showing debug information
   */
  class OSMSCOUT_API RouterParameter CLASS_FINAL
  {
  private:
    bool          debugPerformance;

  public:
    RouterParameter();

    void SetDebugPerformance(bool debug);

    bool IsDebugPerformance() const;
  };

  /**
   * \ingroup Routing
   *
   * Optional callback object for monitoring routing progress
   */
  class OSMSCOUT_API RoutingProgress
  {
  public:
    virtual ~RoutingProgress() = default;

    /**
     * Call, if you want to reset the progress
     */
    virtual void Reset() = 0;

    /**
     * Repeately called by the router while visiting routing nodes
     * @param currentMaxDistance
     *    current maximum distance from start
     * @param overallDistance
     *    distance between start and target
     */
    virtual void Progress(const Distance &currentMaxDistance,
                          const Distance &overallDistance) = 0;
  };

  /**
   * \ingroup Routing
   */
  using RoutingProgressRef = std::shared_ptr<RoutingProgress>;

  /**
   * \ingroup Routing
   *
   * Parameter object for routing calculations. Holds all optional
   * flags and callback objects that can be passed to the router
   */
  class OSMSCOUT_API RoutingParameter CLASS_FINAL
  {
  private:
    BreakerRef         breaker;
    RoutingProgressRef progress;

  public:
    void SetBreaker(const BreakerRef& breaker);
    void SetProgress(const RoutingProgressRef& progress);

    BreakerRef GetBreaker() const
    {
      return breaker;
    }

    RoutingProgressRef GetProgress() const
    {
      return progress;
    }
  };

  /**
   * \ingroup Routing
   *
   * Abstract algorithms for routing
   */
  class OSMSCOUT_API RoutingService
  {
  protected:
    /**
     * \ingroup Routing
     *
     * A path in the routing graph from one node to the next (expressed via the target object)
     * with additional information as required by the A* algorithm.
     */
    struct RNode
    {
      DBId          id;                   //!< The file offset of the current route node
      RouteNodeRef  node;                 //!< The current route node
      DBId          prev;                 //!< The file offset of the previous route node
      Id            exclude;              //!< excluded node to go, similar to route-node excludes,
                                          //!< but used by routing service when initial bearing is restricted
      bool          prevRestricted=false; //!< previous node is restricted
      ObjectFileRef object;               //!< The object (way/area) visited from the current route node

      double        currentCost=0;        //!< The cost of the current up to the current node
      double        estimateCost=0;       //!< The estimated cost from here to the target
      double        overallCost=0;        //!< The overall costs (currentCost+estimateCost)

      bool          restricted=true;      //!< Flag to signal, if access to this node is restricted ("access restrictions")

      /** Flag signaling that we may leave restricted area, because it was start of route.
       * Flag is disabled when we leave to non-restricted way.
       */
      bool          leaveRestricted=false;

      RNode() = default;

      RNode(const DBId& id,
            const RouteNodeRef& node,
            const ObjectFileRef& object)
      : id(id),
        node(node),
        object(object)
      {
        // no code
      }

      RNode(const DBId& id,
            const RouteNodeRef& node,
            const ObjectFileRef& object,
            const DBId& prev,
            bool prevRestricted)
      : id(id),
        node(node),
        prev(prev),
        prevRestricted(prevRestricted),
        object(object)
      {
        // no code
      }

      bool operator==(const RNode& other) const
      {
        return id==other.id;
      }

      bool operator<(const RNode& other) const
      {
        return id<other.id;
      }
    };

    using RNodeRef = std::shared_ptr<RNode>;

    struct RNodeCostCompare
    {
      bool operator()(const RNodeRef& a,
                      const RNodeRef& b) const
      {
        if (a->overallCost==b->overallCost) {
         return a->id<b->id;
        }

        return a->overallCost<b->overallCost;
      }
    };

    /**
     * \ingroup Routing
     *
     * Minimum required data for a node in the ClosedSet.
     *
     * The ClosedSet is the set of routing nodes that have been
     * already handled.
     *
     * From the VNode list from the last routing node back to the start
     * the route is recalculated by following the previousNode chain.
     *
     * Some routing nodes may be accessed from two different ways
     *  - one without any access restriction (!currentRestricted)
     *    and second with restriction (currentRestricted)
     *
     * Restricted way (access=destination) is a way that may be used just
     * in case when start or target is on this way (area of ways).
     */
    struct VNode
    {
      DBId          currentNode;              //!< FileOffset of this route node
      bool          currentRestricted=false;  //!< Current node is accessed from restricted way
      DBId          previousNode;             //!< FileOffset of the previous route node
      bool          previousRestricted=false; //!< Previous node was accessed from restricted way
      ObjectFileRef object;                   //!< The object (way/area) visited from the current route node

      /**
       * Equality operator
       * @param other
       *    Other object to compare against
       * @return
       *    True, if both objects are equal. Objects are currently equal
       *    if they have the same route node file offset.
       */
      bool operator==(const VNode& other) const
      {
        return currentNode==other.currentNode && currentRestricted==other.currentRestricted;
      }

      /**
       * Simple constructor for searching for VNodes in the
       * ClosedSet.
       *
       * @param currentNode
       *    Offset of the node to search for
       */
      explicit VNode(const DBId& currentNode, bool currentRestricted)
        : currentNode(currentNode), currentRestricted(currentRestricted)
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
      VNode(const DBId& currentNode,
            bool currentRestricted,
            const ObjectFileRef& object,
            const DBId& previousNode,
            bool previousRestricted)
      : currentNode(currentNode),
        currentRestricted(currentRestricted),
        previousNode(previousNode),
        previousRestricted(previousRestricted),
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
      size_t operator()(const VNode& node) const
      {
        return std::hash<Id>()(node.currentNode.id) ^
               std::hash<DatabaseId>()(node.currentNode.database) ^
               std::hash<bool>()(node.currentRestricted);
      }
    };

    using OpenList    = std::set<RNodeRef, RNodeCostCompare>;
    using OpenListRef = std::set<RNodeRef, RNodeCostCompare>::iterator;

    using OpenMap     = std::unordered_map<DBId, OpenListRef>;
    using ClosedSet   = std::unordered_set<VNode, ClosedNodeHasher>;

  public:
    //! Relative filename of the intersection data file
    static const char* const FILENAME_INTERSECTIONS_DAT;
    //! Relative filename of the intersection index file
    static const char* const FILENAME_INTERSECTIONS_IDX;

    //! Relative filebase name for touting data as generated by default by the importer
    static const char* const DEFAULT_FILENAME_BASE;

    static std::string GetDataFilename(const std::string& filenamebase);
    static std::string GetData2Filename(const std::string& filenamebase);
    static std::string GetIndexFilename(const std::string& filenamebase);

  public:
    RoutingService();
    virtual ~RoutingService();
  };

}

#endif /* OSMSCOUT_ROUTINGSERVICE_H */

