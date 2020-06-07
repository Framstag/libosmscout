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

#include <osmscout/CoreFeatures.h>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeatures.h>

#include <osmscout/util/Time.h>

// Database
#include <osmscout/Database.h>

// Routing
#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/routing/RoutingProfile.h>

namespace osmscout {

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
  class OSMSCOUT_API RoutePostprocessor
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
      virtual ~Postprocessor();

      virtual bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
                   RouteDescription& description) override;
    };

    /**
     * \ingroup Routing
     * Places a crossing ways description as a description of the name of all ways crossing the given node
     */
    class OSMSCOUT_API CrossingWaysPostprocessor : public Postprocessor
    {
    private:
      void AddCrossingWaysDescriptions(const RoutePostprocessor& postprocessor,
                                       const RouteDescription::CrossingWaysDescriptionRef& description,
                                       const RouteDescription::Node& node,
                                       const ObjectFileRef& originObject,
                                       const ObjectFileRef& targetObject);

    public:
      CrossingWaysPostprocessor() = default;

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

    private:

      bool                     inRoundabout;
      size_t                   roundaboutCrossingCounter;
      bool                     roundaboutClockwise{false};

    private:
      State GetInitialState(const RoutePostprocessor& postprocessor,
                            RouteDescription::Node& node);

      void HandleRoundaboutEnter(const RoutePostprocessor& postprocessor, RouteDescription::Node& node);
      void HandleRoundaboutNode(RouteDescription::Node& node);
      void HandleRoundaboutLeave(RouteDescription::Node& node);
      void HandleDirectMotorwayEnter(RouteDescription::Node& node,
                                     const RouteDescription::NameDescriptionRef& toName);
      void HandleDirectMotorwayLeave(RouteDescription::Node& node,
                                     const RouteDescription::NameDescriptionRef& fromName);
      bool HandleNameChange(const std::list<RouteDescription::Node>& path,
                            std::list<RouteDescription::Node>::const_iterator& lastNode,
                            std::list<RouteDescription::Node>::iterator& node);
      bool HandleDirectionChange(const std::list<RouteDescription::Node>& path,
                                 std::list<RouteDescription::Node>::iterator& node);

    public:
      bool Process(const RoutePostprocessor& postprocessor,
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
      std::list<WayRef> CollectWays(const RoutePostprocessor& postprocessor,
                                    const std::list<RouteDescription::Node>& nodes) const;
      std::list<AreaRef> CollectAreas(const RoutePostprocessor& postprocessor,
                                      const std::list<RouteDescription::Node>& nodes) const;
      std::map<ObjectFileRef,std::set<ObjectFileRef>> CollectPOICandidates(const Database& database,
                                                                           const std::set<ObjectFileRef>& paths,
                                                                           const std::list<WayRef>& ways,
                                                                           const std::list<AreaRef>& areas);
      std::map<ObjectFileRef,POIAtRoute> AnalysePOICandidates(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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

      bool Process(const RoutePostprocessor& postprocessor,
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
      SuggestedLanesPostprocessor(const Distance &distanceBefore=Meters(500)) :
        Postprocessor(), distanceBefore(distanceBefore) {};

      bool Process(const RoutePostprocessor& postprocessor,
                   RouteDescription& description) override;
    private:
      Distance distanceBefore;
    };

    using SuggestedLanesPostprocessorRef = std::shared_ptr<SuggestedLanesPostprocessor>;

  private:
    std::vector<RoutingProfileRef>                                profiles;
    std::vector<DatabaseRef>                                      databases;

    std::unordered_map<DBFileOffset,AreaRef>                      areaMap;
    std::unordered_map<DBFileOffset,WayRef>                       wayMap;

    std::unordered_map<DatabaseId,NameFeatureValueReader*>        nameReaders;
    std::unordered_map<DatabaseId,RefFeatureValueReader*>         refReaders;
    std::unordered_map<DatabaseId,BridgeFeatureReader*>           bridgeReaders;
    std::unordered_map<DatabaseId,RoundaboutFeatureReader*>       roundaboutReaders;
    std::unordered_map<DatabaseId,DestinationFeatureValueReader*> destinationReaders;
    std::unordered_map<DatabaseId,MaxSpeedFeatureValueReader*>    maxSpeedReaders;
    std::unordered_map<DatabaseId,LanesFeatureValueReader*>       lanesReaders;
    std::unordered_map<DatabaseId,AccessFeatureValueReader*>      accessReaders;

    std::unordered_map<DatabaseId,TypeInfoSet>                    motorwayTypes;
    std::unordered_map<DatabaseId,TypeInfoSet>                    motorwayLinkTypes;
    std::unordered_map<DatabaseId,TypeInfoSet>                    junctionTypes;

  private:
    bool ResolveAllAreasAndWays(const RouteDescription& description,
                                DatabaseId dbId,
                                Database& database);
    void Cleanup();

  private:
    AreaRef GetArea(const DBFileOffset &offset) const;
    WayRef GetWay(const DBFileOffset &offset) const;

    Duration GetTime(DatabaseId dbId,const Area& area,const Distance &deltaDistance) const;
    Duration GetTime(DatabaseId dbId,const Way& way,const Distance &deltaDistance) const;

    RouteDescription::NameDescriptionRef GetNameDescription(const RouteDescription::Node& node) const;
    RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                            const ObjectFileRef& object) const;
    RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                            const Node& node) const;
    RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                            const Area& area) const;
    RouteDescription::NameDescriptionRef GetNameDescription(DatabaseId dbId,
                                                            const Way& way) const;

    bool LoadJunction(DatabaseId database,
                      GeoCoord coord,
                      std::string junctionRef,
                      std::string junctionName) const;

    bool IsMotorwayLink(const RouteDescription::Node& node) const;
    bool IsMotorway(const RouteDescription::Node& node) const;

    bool IsRoundabout(const RouteDescription::Node& node) const;
    bool IsBridge(const RouteDescription::Node& node) const;

    RouteDescription::DestinationDescriptionRef GetDestination(const RouteDescription::Node& node) const;

    uint8_t GetMaxSpeed(const RouteDescription::Node& node) const;

    RouteDescription::LaneDescriptionRef GetLanes(const RouteDescription::Node& node) const;

    Id GetNodeId(const RouteDescription::Node& node) const;

    size_t GetNodeIndex(const RouteDescription::Node& node,
                        Id nodeId) const;

    bool CanUseBackward(const DatabaseId& dbId,
                        Id fromNodeId,
                        const ObjectFileRef& object) const;

    bool CanUseForward(const DatabaseId& dbId,
                       Id fromNodeId,
                       const ObjectFileRef& object) const;

    bool IsBackwardPath(const ObjectFileRef& object,
                        size_t fromNodeIndex,
                        size_t toNodeIndex) const;

    bool IsForwardPath(const ObjectFileRef& object,
                       size_t fromNodeIndex,
                       size_t toNodeIndex) const;

    bool IsNodeStartOrEndOfObject(const RouteDescription::Node& node,
                                  const ObjectFileRef& object) const;

    GeoCoord GetCoordinates(const RouteDescription::Node& node,
                            size_t nodeIndex) const;

  public:
    /*
     * TODO:
     * All Postprocessors are allowed to use our internal methods currently.
     * We should fix this by moving helper methods to a separate
     * PostprocessorContext object that gets passed to the postprocessors explicitely.
     * This would also move state out of the RoutePostprocessor itself.
     */
    friend Postprocessor;

    RoutePostprocessor();

    bool PostprocessRouteDescription(RouteDescription& description,
                                     const std::vector<RoutingProfileRef>& profiles,
                                     const std::vector<DatabaseRef>& databases,
                                     const std::list<PostprocessorRef>& processors,
                                     const std::set<std::string>& motorwayTypeNames=std::set<std::string>(),
                                     const std::set<std::string>& motorwayLinkTypeNames=std::set<std::string>(),
                                     const std::set<std::string>& junctionTypeNames=std::set<std::string>());
  };
}

#endif
