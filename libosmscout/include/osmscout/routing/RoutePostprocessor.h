#ifndef OSMSCOUT_ROUTEPOSTPROCESSOR_H
#define OSMSCOUT_ROUTEPOSTPROCESSOR_H

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
#include <map>
#include <memory>
#include <unordered_map>

#include <osmscout/lib/CoreFeatures.h>

#include <osmscout/TypeConfig.h>

#include <osmscout/feature/BridgeFeature.h>
#include <osmscout/feature/ClockwiseDirectionFeature.h>
#include <osmscout/feature/DestinationFeature.h>
#include <osmscout/feature/LanesFeature.h>
#include <osmscout/feature/NameFeature.h>
#include <osmscout/feature/RefFeature.h>
#include <osmscout/feature/RoundaboutFeature.h>

#include <osmscout/util/Time.h>

// Database
#include <osmscout/db/Database.h>

// Routing
#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/routing/RoutingProfile.h>

namespace osmscout {

  class OSMSCOUT_API PostprocessorContext
  {
  public:
    virtual AreaRef GetArea(const DBFileOffset &offset) const = 0;
    virtual WayRef GetWay(const DBFileOffset &offset) const = 0;
    virtual NodeRef GetNode(const DBFileOffset &offset) const = 0;

    virtual const LanesFeatureValueReader& GetLaneReader(const DatabaseId &dbId) const = 0;
    virtual const AccessFeatureValueReader& GetAccessReader(const DatabaseId &dbId) const = 0;

    virtual Duration GetTime(DatabaseId dbId,const Area& area,const Distance &deltaDistance) const = 0;
    virtual Duration GetTime(DatabaseId dbId,const Way& way,const Distance &deltaDistance) const = 0;

    virtual RouteDescription::NameDescriptionRef GetNameDescription(const RouteDescription::Node& node) const = 0;
    virtual RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                                    const ObjectFileRef& object) const = 0;
    virtual RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                                    const Node& node) const = 0;
    virtual RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                                    const Area& area) const = 0;
    virtual RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                                    const Way& way) const = 0;

    virtual bool IsMotorwayLink(const RouteDescription::Node& node) const = 0;
    virtual bool IsMotorway(const RouteDescription::Node& node) const = 0;

    virtual bool IsMiniRoundabout(const RouteDescription::Node& node) const = 0;
    virtual bool IsClockwise(const RouteDescription::Node& node) const = 0;
    virtual bool IsRoundabout(const RouteDescription::Node& node) const = 0;
    virtual bool IsBridge(const RouteDescription::Node& node) const = 0;

    virtual NodeRef GetJunctionNode(const RouteDescription::Node& node) const = 0;

    virtual RouteDescription::DestinationDescriptionRef GetDestination(const RouteDescription::Node& node) const = 0;

    virtual uint8_t GetMaxSpeed(const RouteDescription::Node& node) const = 0;

    virtual RouteDescription::LaneDescription GetLanes(const DatabaseId& dbId, const WayRef& way, bool forward) const;

    virtual RouteDescription::LaneDescriptionRef GetLanes(const RouteDescription::Node& node) const;

    virtual Id GetNodeId(const RouteDescription::Node& node) const;

    virtual size_t GetNodeIndex(const RouteDescription::Node& node,
                                Id nodeId) const = 0;

    virtual bool CanUseBackward(const DatabaseId& dbId,
                                Id fromNodeId,
                                const ObjectFileRef& object) const = 0;

    virtual bool CanUseForward(const DatabaseId& dbId,
                               Id fromNodeId,
                               const ObjectFileRef& object) const = 0;

    virtual bool IsBackwardPath(const ObjectFileRef& object,
                                size_t fromNodeIndex,
                                size_t toNodeIndex) const = 0;

    virtual bool IsForwardPath(const ObjectFileRef& object,
                               size_t fromNodeIndex,
                               size_t toNodeIndex) const = 0;

    virtual bool IsNodeStartOrEndOfObject(const RouteDescription::Node& node,
                                          const ObjectFileRef& object) const = 0;

    virtual GeoCoord GetCoordinates(const RouteDescription::Node& node,
                                    size_t nodeIndex) const = 0;

    /** Get low level database objects (indexed by DatabaseId)
     */
    virtual std::vector<DatabaseRef> GetDatabases() const = 0;

  };

  /**
   * \ingroup Routing
   *
   * RouteProcessor allows to enhance the raw routing information from the routing algorithm with additional
   * information like way names, turns and similar by traversing the route and its objects.
   *
   * The processor is plugable in the sense that it can get enhanced by classes deriving from the Processor
   * base class allowing to write traversial code for a specific aim. The complete routing description
   * is the result of the sum of all information collected by the individual processors.
   */
  class OSMSCOUT_API RoutePostprocessor: public PostprocessorContext
  {
  public:
    /**
     * \ingroup Routing
     * Base class for routing processors. Routing processors allow iterating of the raw route with the aim
     * to collect further information to enhance information abut the route like street names, turns etc...
     */
    class OSMSCOUT_API Postprocessor
    {
    public:
      virtual ~Postprocessor() = default;

      virtual bool Process(const PostprocessorContext& context,
                           RouteDescription& description) = 0;
    };

    using PostprocessorRef = std::shared_ptr<Postprocessor>;

    /**
     * \ingroup Routing
     * Places the given description at the start node
     */
    class OSMSCOUT_API StartPostprocessor : public Postprocessor
    {
    private:
      std::string startDescription;

    public:
      explicit StartPostprocessor(const std::string& startDescription);

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Places the given description at the target node
     */
    class OSMSCOUT_API TargetPostprocessor : public Postprocessor
    {
    private:
      std::string targetDescription;

    public:
      explicit TargetPostprocessor(const std::string& targetDescription);

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Calculates the overall running distance and time for each node
     */
    class OSMSCOUT_API DistanceAndTimePostprocessor : public Postprocessor
    {
    public:
      DistanceAndTimePostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Places a name description as way description
     */
    class OSMSCOUT_API WayNamePostprocessor : public Postprocessor
    {
    public:
      WayNamePostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Places a name description as way description
     */
    class OSMSCOUT_API WayTypePostprocessor : public Postprocessor
    {
    public:
      WayTypePostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Places a crossing ways description as a description of the name of all ways crossing the given node
     */
    class OSMSCOUT_API CrossingWaysPostprocessor : public Postprocessor
    {
    private:
      void AddCrossingWaysDescriptions(const PostprocessorContext& context,
                                       const RouteDescription::CrossingWaysDescriptionRef& description,
                                       const RouteDescription::Node& node,
                                       const ObjectFileRef& originObject,
                                       const ObjectFileRef& targetObject);

    public:
      CrossingWaysPostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Places a turn description for every node
     */
    class OSMSCOUT_API DirectionPostprocessor : public Postprocessor
    {
    private:
      static const double curveMinInitialAngle;
      static const double curveMaxInitialAngle;
      static const Distance curveMaxNodeDistance;
      static const Distance curveMaxDistance;
      static const double curveMinAngle;

    public:
      DirectionPostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Adds driving hint based on motorway_junction tags
     */
    class OSMSCOUT_API MotorwayJunctionPostprocessor : public Postprocessor
    {
    public:
      MotorwayJunctionPostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
   * \ingroup Routing
   * Evaluates destination tags, hinting at the destination of a way
   */
    class OSMSCOUT_API DestinationPostprocessor : public Postprocessor
    {
    public:
      DestinationPostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Collects max speed information
     */
    class OSMSCOUT_API MaxSpeedPostprocessor : public RoutePostprocessor::Postprocessor
    {
    public:
      MaxSpeedPostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Generates drive instructions
     */
    class OSMSCOUT_API InstructionPostprocessor : public Postprocessor
    {
    private:
      enum State {
        street,
        roundabout,
        motorway,
        link
      };

      struct NodeExit {
        ObjectFileRef ref;
        size_t node;
        Bearing bearing;
        bool canBeUsedAsExit;
      };

    private:

      bool                     inRoundabout{false};
      size_t                   roundaboutCrossingCounter{0};
      bool                     roundaboutClockwise{false};

    private:
      State GetInitialState(const PostprocessorContext& context,
                            RouteDescription::Node& node);

      void HandleRoundaboutEnter(const PostprocessorContext& context, RouteDescription::Node& node);
      void HandleRoundaboutNode(RouteDescription::Node& node);
      void HandleRoundaboutLeave(RouteDescription::Node& node);
      void HandleMiniRoundabout(const PostprocessorContext& context,
                                RouteDescription::Node& node,
                                ObjectFileRef incomingPath,
                                size_t incomingNode);

      void HandleDirectMotorwayEnter(RouteDescription::Node& node,
                                     const RouteDescription::NameDescriptionRef& toName);
      void HandleDirectMotorwayLeave(RouteDescription::Node& node,
                                     const RouteDescription::NameDescriptionRef& fromName);
      void HandleMotorwayLink(const PostprocessorContext& context,
                              const RouteDescription::NameDescriptionRef &originName,
                              const std::list<RouteDescription::Node>::const_iterator &lastNode,
                              const std::list<RouteDescription::Node>::iterator &node,
                              const std::list<RouteDescription::Node>::const_iterator &end);
      bool HandleNameChange(std::list<RouteDescription::Node>::const_iterator& lastNode,
                            std::list<RouteDescription::Node>::iterator& node,
                            const std::list<RouteDescription::Node>::const_iterator &end);
      bool HandleDirectionChange(const PostprocessorContext& context,
                                 const std::list<RouteDescription::Node>::iterator& node,
                                 const std::list<RouteDescription::Node>::const_iterator& end);
      std::vector<NodeExit> CollectNodeWays(const PostprocessorContext& context,
                                            RouteDescription::Node& node,
                                            bool exitsOnly);

      // just ways are supported as exits
      inline std::vector<NodeExit> CollectNodeExits(const PostprocessorContext& context,
                                                    RouteDescription::Node& node)
      {
        return CollectNodeWays(context, node, true);
      }

    public:
      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;

    };

    using InstructionPostprocessorRef = std::shared_ptr<InstructionPostprocessor>;

    /**
     * \ingroup Routing
     * Collects POIs the vehicle passes by
     */
    class OSMSCOUT_API POIsPostprocessor : public RoutePostprocessor::Postprocessor
    {
    private:
      struct POIAtRoute
      {
        ObjectFileRef                               object;
        RouteDescription::NameDescriptionRef        name;
        Distance                                    distance;
        std::list<RouteDescription::Node>::iterator node;
      };

    private:
      std::set<ObjectFileRef> CollectPaths(const std::list<RouteDescription::Node>& nodes) const;
      std::list<WayRef> CollectWays(const PostprocessorContext& context,
                                    const std::list<RouteDescription::Node>& nodes) const;
      std::list<AreaRef> CollectAreas(const PostprocessorContext& context,
                                      const std::list<RouteDescription::Node>& nodes) const;
      std::map<ObjectFileRef,std::set<ObjectFileRef>> CollectPOICandidates(const Database& database,
                                                                           const std::set<ObjectFileRef>& paths,
                                                                           const std::list<WayRef>& ways,
                                                                           const std::list<AreaRef>& areas);
      std::map<ObjectFileRef,POIAtRoute> AnalysePOICandidates(const PostprocessorContext& context,
                                                              const DatabaseId& databaseId,
                                                              std::list<RouteDescription::Node>& nodes,
                                                              const TypeInfoSet& nodeTypes,
                                                              const TypeInfoSet& areaTypes,
                                                              const std::unordered_map<FileOffset,NodeRef>& nodeMap,
                                                              const std::unordered_map<FileOffset,AreaRef>& areaMap,
                                                              const std::map<ObjectFileRef,std::set<ObjectFileRef>>& poiCandidates);
      void SortInCollectedPOIs(const DatabaseId& databaseId,
                               const std::map<ObjectFileRef,POIAtRoute>& pois);

    public:
      POIsPostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    using POIsPostprocessorRef = std::shared_ptr<POIsPostprocessor>;

    /**
     * \ingroup Routing
     * Evaluate route lanes
     */
    class OSMSCOUT_API LanesPostprocessor : public RoutePostprocessor::Postprocessor
    {
    public:
      LanesPostprocessor() = default;

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;
    };

    using LanesPostprocessorRef = std::shared_ptr<LanesPostprocessor>;

    /**
     * \ingroup Routing
     * Evaluate suggested route lanes that may be used.
     * Route lanes have to be evaluated already (using LanesPostprocessor)
     */
    class OSMSCOUT_API SuggestedLanesPostprocessor : public RoutePostprocessor::Postprocessor
    {
    public:
      explicit SuggestedLanesPostprocessor(const Distance &distanceBefore=Meters(500)) :
        Postprocessor(), distanceBefore(distanceBefore) {};

      bool Process(const PostprocessorContext& context,
                   RouteDescription& description) override;

    private:
      RouteDescription::LaneDescriptionRef GetLaneDescription(const RouteDescription::Node &node) const;

      /** Evaluate suggested lanes on nodes from "backBuffer", followed by "node".
       * Node itself is junction, where count of lanes (on way from the node)
       * is smaller than on the previous node (back of backBuffer).
       *
       * @param node
       * @param backBuffer buffer of traveled nodes, recent node at back
       */
      void EvaluateLaneSuggestion(const PostprocessorContext& context,
                                  const RouteDescription::Node &node,
                                  const std::list<RouteDescription::Node*> &backBuffer) const;

    private:
      Distance distanceBefore;
    };
      
    /**
    * \ingroup Routing
    * Adds section to the route if there is one or more via node
    */
    class OSMSCOUT_API SectionsPostprocessor : public Postprocessor
    {
    private:
        std::vector<int> sectionLengths;
    public:
        explicit SectionsPostprocessor(const std::vector<int>& sectionLengths) : Postprocessor(), sectionLengths(sectionLengths) {};

    bool Process(const PostprocessorContext& context,
                 RouteDescription& description) override;
    };

    using SuggestedLanesPostprocessorRef = std::shared_ptr<SuggestedLanesPostprocessor>;

  private:
    /* TODO: separate PostprocessorContext implementation from RoutePostprocessor, move state context
     */
    std::vector<RoutingProfileRef>                                profiles;
    std::vector<DatabaseRef>                                      databases;

    std::unordered_map<DBFileOffset,AreaRef>                      areaMap;
    std::unordered_map<DBFileOffset,WayRef>                       wayMap;
    std::unordered_map<DBFileOffset,NodeRef>                      nodeMap;

    std::unordered_map<DatabaseId,NameFeatureValueReader*>        nameReaders;
    std::unordered_map<DatabaseId,RefFeatureValueReader*>         refReaders;
    std::unordered_map<DatabaseId,BridgeFeatureReader*>           bridgeReaders;
    std::unordered_map<DatabaseId,RoundaboutFeatureReader*>       roundaboutReaders;
    std::unordered_map<DatabaseId,ClockwiseDirectionFeatureReader*> clockwiseDirectionReaders;
    std::unordered_map<DatabaseId,DestinationFeatureValueReader*> destinationReaders;
    std::unordered_map<DatabaseId,MaxSpeedFeatureValueReader*>    maxSpeedReaders;
    std::unordered_map<DatabaseId,LanesFeatureValueReader*>       lanesReaders;
    std::unordered_map<DatabaseId,AccessFeatureValueReader*>      accessReaders;

    std::unordered_map<DatabaseId,TypeInfoSet>                    motorwayTypes;
    std::unordered_map<DatabaseId,TypeInfoSet>                    motorwayLinkTypes;
    std::unordered_map<DatabaseId,TypeInfoSet>                    junctionTypes;
    std::unordered_map<DatabaseId,TypeInfoRef>                    miniRoundaboutTypes;

  private:
    bool ResolveAllPathObjects(const RouteDescription& description,
                               DatabaseId dbId,
                               Database& database);
    void Cleanup();

  private:
    AreaRef GetArea(const DBFileOffset &offset) const override;
    WayRef GetWay(const DBFileOffset &offset) const override;
    NodeRef GetNode(const DBFileOffset &offset) const override;

    const LanesFeatureValueReader& GetLaneReader(const DatabaseId &dbId) const override;
    const AccessFeatureValueReader& GetAccessReader(const DatabaseId &dbId) const override;

    Duration GetTime(DatabaseId dbId,const Area& area,const Distance &deltaDistance) const override;
    Duration GetTime(DatabaseId dbId,const Way& way,const Distance &deltaDistance) const override;

    RouteDescription::NameDescriptionRef GetNameDescription(const RouteDescription::Node& node) const override;
    RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                            const ObjectFileRef& object) const override;
    RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                            const Node& node) const override;
    RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                            const Area& area) const override;
    RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                            const Way& way) const override;

    bool IsMotorwayLink(const RouteDescription::Node& node) const override;
    bool IsMotorway(const RouteDescription::Node& node) const override;

    bool IsMiniRoundabout(const RouteDescription::Node& node) const override;
    bool IsClockwise(const RouteDescription::Node& node) const override;
    bool IsRoundabout(const RouteDescription::Node& node) const override;
    bool IsBridge(const RouteDescription::Node& node) const override;

    NodeRef GetJunctionNode(const RouteDescription::Node& node) const override;

    RouteDescription::DestinationDescriptionRef GetDestination(const RouteDescription::Node& node) const override;

    uint8_t GetMaxSpeed(const RouteDescription::Node& node) const override;

    size_t GetNodeIndex(const RouteDescription::Node& node,
                        Id nodeId) const override;

    bool CanUseBackward(const DatabaseId& dbId,
                        Id fromNodeId,
                        const ObjectFileRef& object) const override;

    bool CanUseForward(const DatabaseId& dbId,
                       Id fromNodeId,
                       const ObjectFileRef& object) const override;

    bool IsBackwardPath(const ObjectFileRef& object,
                        size_t fromNodeIndex,
                        size_t toNodeIndex) const override;

    bool IsForwardPath(const ObjectFileRef& object,
                       size_t fromNodeIndex,
                       size_t toNodeIndex) const override;

    bool IsNodeStartOrEndOfObject(const RouteDescription::Node& node,
                                  const ObjectFileRef& object) const override;

    GeoCoord GetCoordinates(const RouteDescription::Node& node,
                            size_t nodeIndex) const override;

    std::vector<DatabaseRef> GetDatabases() const override;

  public:
    bool PostprocessRouteDescription(RouteDescription& description,
                                     const std::vector<RoutingProfileRef>& profiles,
                                     const std::vector<DatabaseRef>& databases,
                                     const std::list<PostprocessorRef>& processors,
                                     const std::set<std::string,std::less<>>& motorwayTypeNames=std::set<std::string,std::less<>>(),
                                     const std::set<std::string,std::less<>>& motorwayLinkTypeNames=std::set<std::string,std::less<>>(),
                                     const std::set<std::string,std::less<>>& junctionTypeNames=std::set<std::string,std::less<>>(),
                                     const std::string& miniRoundaboutTypeName="highway_mini_roundabout");
  };
}

#endif
