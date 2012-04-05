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

#include <osmscout/TypeConfig.h>

#include <osmscout/RouteNode.h>

// Datafiles
#include <osmscout/WayDataFile.h>

// Fileoffset by Id index
#include <osmscout/WayIndex.h>

// Database
#include <osmscout/Database.h>

// Routing
#include <osmscout/Route.h>
#include <osmscout/RouteData.h>
#include <osmscout/RoutingProfile.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/HashMap.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API RoutePostprocessor
  {
  public:
    OSMSCOUT_API class Postprocessor : public Referencable
    {
    public:
      virtual ~Postprocessor();

      virtual bool Process(const RoutingProfile& profile,
                           RouteDescription& description,
                           Database& database) = 0;
    };

    typedef Ref<Postprocessor> PostprocessorRef;

    /**
     * Places the given description at the start node
     */
    OSMSCOUT_API class StartPostprocessor : public Postprocessor
    {
    private:
      std::string startDescription;

    public:
      StartPostprocessor(const std::string& startDescription);

      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Places the given description at the target node
     */
    OSMSCOUT_API class TargetPostprocessor : public Postprocessor
    {
    private:
      std::string targetDescription;

    public:
      TargetPostprocessor(const std::string& targetDescription);

      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Calculates the overall running distance and time for each node
     */
    OSMSCOUT_API class DistanceAndTimePostprocessor : public Postprocessor
    {
    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Places a name description as way description
     */
    OSMSCOUT_API class WayNamePostprocessor : public Postprocessor
    {
    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Places a crossing ways description as a description of the name of all ways crossing the given node
     */
    OSMSCOUT_API class CrossingWaysPostprocessor : public Postprocessor
    {
    private:
      OSMSCOUT_HASHSET<TypeId> motorwayTypes;
      OSMSCOUT_HASHSET<TypeId> motorwayLinkTypes;

    private:
      void AddCrossingWaysDescriptions(RouteDescription::CrossingWaysDescription* description,
                                       const RouteDescription::Node& node,
                                       const WayRef& originWay,
                                       const WayRef& targetWay,
                                       const std::map<Id,WayRef>& wayMap);

    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);

      void AddMotorwayType(TypeId type);
      void AddMotorwayLinkType(TypeId type);
    };

    /**
     * Places a name change description if the name changes
     */
    OSMSCOUT_API class WayNameChangedPostprocessor : public Postprocessor
    {
    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Places a turn description for every node
     */
    OSMSCOUT_API class DirectionPostprocessor : public Postprocessor
    {
    private:
      static const double curveMinInitialAngle;
      static const double curveMaxInitialAngle;
      static const double curveMaxNodeDistance;
      static const double curveMaxDistance;
      static const double curveMinAngle;

    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Generates drive instructions
     */
    OSMSCOUT_API class InstructionPostprocessor : public Postprocessor
    {
    private:
      enum State {
        street,
        roundabout,
        motorway,
        link
      };

    private:
      OSMSCOUT_HASHSET<TypeId> motorwayTypes;
      OSMSCOUT_HASHSET<TypeId> motorwayLinkTypes;

      bool                     inRoundabout;
      size_t                   roundaboutCrossingCounter;

    private:
      State GetInitialState(RouteDescription::Node& node,
                            std::map<Id,WayRef>& wayMap);

      void HandleRoundaboutEnter(RouteDescription::Node& node);
      void HandleRoundaboutNode(RouteDescription::Node& node);
      void HandleRoundaboutLeave(RouteDescription::Node& node);
      void HandleDirectMotorwayEnter(RouteDescription::Node& node,
                                     const RouteDescription::NameDescriptionRef& toName);
      void HandleDirectMotorwayLeave(RouteDescription::Node& node,
                                     const RouteDescription::NameDescriptionRef& fromName);
      bool HandleNameChange(const std::list<RouteDescription::Node>& path,
                            std::list<RouteDescription::Node>::iterator& node,
                            const std::map<Id,WayRef>& wayMap);
      bool HandleDirectionChange(const std::list<RouteDescription::Node>& path,
                                 std::list<RouteDescription::Node>::iterator& node,
                                 const std::map<Id,WayRef>& wayMap);

    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);

      void AddMotorwayType(TypeId type);
      void AddMotorwayLinkType(TypeId type);
    };

  public:
    bool PostprocessRouteDescription(RouteDescription& description,
                                     const RoutingProfile& profile,
                                     Database& database,
                                     std::list<PostprocessorRef> processors);
  };

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

  class OSMSCOUT_API Router
  {
  private:
    /**
     * A node in the routing graph, normally a node as part of a way
     */
    struct RNode : public Referencable
    {
      Id     nodeId;
      Id     wayId;

      double currentCost;
      double estimateCost;
      double overallCost;

      Id     prev;

      bool   access;

      RNode()
      : nodeId(0),
        wayId(0)
      {
        // no code
      }

      RNode(Id nodeId,
            Id wayId)
      : nodeId(nodeId),
        wayId(wayId),
        currentCost(0),
        estimateCost(0),
        overallCost(0),
        prev(0),
        access(true)
      {
        // no code
      }

      RNode(Id nodeId,
            Id wayId,
            Id prev)
      : nodeId(nodeId),
        wayId(wayId),
        currentCost(0),
        estimateCost(0),
        overallCost(0),
        prev(prev),
        access(true)
      {
        // no code
      }

      inline bool operator==(const RNode& other)
      {
        return nodeId==other.nodeId;
      }

      inline bool operator<(const RNode& other) const
      {
        return nodeId<other.nodeId;
      }
    };

    typedef Ref<RNode> RNodeRef;

    struct RNodeCostCompare
    {
      inline bool operator()(const RNodeRef& a, const RNodeRef& b) const
      {
        if (a->overallCost==b->overallCost) {
         return a->nodeId<b->nodeId;
        }
        else {
          return a->overallCost<b->overallCost;
        }
      }
    };

    typedef std::set<RNodeRef,RNodeCostCompare>           OpenList;
    typedef std::set<RNodeRef,RNodeCostCompare>::iterator OpenListRef;

    typedef OSMSCOUT_HASHMAP<Id,Router::OpenListRef>      OpenMap;
    typedef OSMSCOUT_HASHMAP<Id,Router::RNodeRef>         CloseMap;

  private:
    bool                  isOpen;            //! true, if opened
    bool                  debugPerformance;

    std::string           path;              //! Path to the directory containing all files

    WayDataFile           wayDataFile;       //! Cached access to the 'ways.dat' file
    RouteNodeDataFile     routeNodeDataFile; //! Cached access to the 'route.dat' file

    TypeConfig            *typeConfig;       //! Type config for the currently opened map

  private:
    void GetClosestForwardRouteNode(const WayRef& way,
                                    Id nodeId,
                                    RouteNodeRef& routeNode,
                                    size_t& pos);
    void GetClosestBackwardRouteNode(const WayRef& way,
                                     Id nodeId,
                                     RouteNodeRef& routeNode,
                                     size_t& pos);
    bool ResolveRNodesToList(const RNodeRef& end,
                             const CloseMap& closeMap,
                             std::list<RNodeRef>& nodes);
    bool ResolveRNodesToRouteData(const std::list<RNodeRef>& nodes,
                                  Id startWayId,
                                  Id startNodeId,
                                  Id targetWayId,
                                  Id targetNodeId,
                                  RouteData& route);
    bool AddNodes(RouteData& route,
                  const std::vector<Id>& startCrossingWaysIds,
                  const std::vector<Path>& startPaths,
                  Id startNodeId,
                  Id wayId,
                  Id targetNodeId);


  public:
    Router(const RouterParameter& parameter);
    virtual ~Router();

    bool Open(const std::string& path);
    bool IsOpen() const;
    void Close();

    void FlushCache();

    TypeConfig* GetTypeConfig() const;

    bool CalculateRoute(const RoutingProfile& profile,
                        Id startWayId, Id startNodeId,
                        Id targetWayId, Id targetNodeId,
                        RouteData& route);

    bool TransformRouteDataToWay(const RouteData& data,
                                 Way& way);

    bool TransformRouteDataToPoints(const RouteData& data,
                                    std::list<Point>& points);

    bool TransformRouteDataToRouteDescription(const RouteData& data,
                                              RouteDescription& description);

    void DumpStatistics();
  };
}

#endif
