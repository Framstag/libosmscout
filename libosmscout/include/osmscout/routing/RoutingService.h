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
#include <osmscout/routing/DBFileOffset.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/Cache.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * \ingroup Routing
   */
  typedef DataFile<RouteNode> RouteNodeDataFile;

  /**
   * \ingroup Routing
   *
   * Start or end position of a route calculation
   */
  class OSMSCOUT_API RoutePosition CLASS_FINAL
  {
  private:
    ObjectFileRef object;
    size_t        nodeIndex;
    DatabaseId    database;

  public:
    RoutePosition();
    RoutePosition(const ObjectFileRef& object,
                  size_t nodeIndex,
                  DatabaseId database);

    inline bool IsValid() const
    {
      return object.Valid();
    }

    inline ObjectFileRef GetObjectFileRef() const
    {
      return object;
    }

    inline size_t GetNodeIndex() const
    {
      return nodeIndex;
    }

    inline DatabaseId GetDatabaseId() const
    {
      return database;
    }
  };

  /**
   * \ingroup Routing
   *
   * Database instance initialization parameter to influence the behavior of the database
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
    virtual ~RoutingProgress();

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
    virtual void Progress(double currentMaxDistance,
                          double overallDistance) = 0;
  };

  /**
   * \ingroup Routing
   */
  typedef std::shared_ptr<RoutingProgress> RoutingProgressRef;

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

    inline BreakerRef GetBreaker() const
    {
      return breaker;
    }

    inline RoutingProgressRef GetProgress() const
    {
      return progress;
    }
  };

  /**
   * Result of a routing calculation. This object is always returned.
   * In case of an routing error it however may not contain a valid route
   * (route is empty).
   *
   * @TODO: Make setter private and class friend to the RoutingService
   */
  class OSMSCOUT_API RoutingResult CLASS_FINAL
  {
  private:
    RouteData route;
    double    currentMaxDistance;
    double    overallDistance;

  public:
    RoutingResult();

    inline void SetOverallDistance(double overallDistance)
    {
      this->overallDistance=overallDistance;
    }

    inline void SetCurrentMaxDistance(double currentMaxDistance)
    {
      this->currentMaxDistance=currentMaxDistance;
    }

    inline double GetOverallDistance() const
    {
      return overallDistance;
    }

    inline double GetCurrentMaxDistance() const
    {
      return currentMaxDistance;
    }

    inline RouteData& GetRoute()
    {
      return route;
    }

    inline const RouteData& GetRoute() const
    {
      return route;
    }

    inline bool Success() const
    {
      return !route.IsEmpty();
    }
  };

  /**
   * \ingroup Routing
   *
   * Abstract algorithms for routing
   */
  class OSMSCOUT_API RoutingService{
  protected:
    /**
     * \ingroup Routing
     *
     * A path in the routing graph from one node to the next (expressed via the target object)
     * with additional information as required by the A* algorithm.
     */
    struct RNode
    {
      DBFileOffset  nodeOffset;    //!< The file offset of the current route node
      RouteNodeRef  node;          //!< The current route node
      DBFileOffset  prev;          //!< The file offset of the previous route node
      ObjectFileRef object;        //!< The object (way/area) visited from the current route node

      double        currentCost;   //!< The cost of the current up to the current node
      double        estimateCost;  //!< The estimated cost from here to the target
      double        overallCost;   //!< The overall costs (currentCost+estimateCost)

      bool          access;        //!< Flags to signal, if we had access ("access restrictions") to this node

      RNode()
      : nodeOffset()
      {
        // no code
      }

      RNode(DBFileOffset nodeOffset,
            const RouteNodeRef& node,
            const ObjectFileRef& object)
      : nodeOffset(nodeOffset),
        node(node),
        prev(),
        object(object),
        currentCost(0),
        estimateCost(0),
        overallCost(0),
        access(true)
      {
        // no code
      }

      RNode(DBFileOffset nodeOffset,
            const RouteNodeRef& node,
            const ObjectFileRef& object,
            DBFileOffset prev)
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
     * \ingroup Routing
     *
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
      DBFileOffset currentNode;   //!< FileOffset of this route node
      DBFileOffset previousNode;  //!< FileOffset of the previous route node
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
      inline VNode(DBFileOffset currentNode)
        : currentNode(currentNode),
          previousNode()
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
      VNode(DBFileOffset currentNode,
            const ObjectFileRef& object,
            DBFileOffset previousNode)
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
        return std::hash<FileOffset>()(node.currentNode.offset) ^
               std::hash<DatabaseId>()(node.currentNode.database);
      }
    };

    typedef std::set<RNodeRef,RNodeCostCompare>           OpenList;
    typedef std::set<RNodeRef,RNodeCostCompare>::iterator OpenListRef;

    typedef std::unordered_map<DBFileOffset,OpenListRef>  OpenMap;
    typedef std::unordered_set<VNode,ClosedNodeHasher>    ClosedSet;

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

