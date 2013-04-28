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

#include <osmscout/CoreFeatures.h>

// Database
#include <osmscout/Database.h>

// Routing
#include <osmscout/RouteData.h>
#include <osmscout/RoutingProfile.h>

#include <osmscout/util/HashSet.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  class OSMSCOUT_API RoutePostprocessor
  {
  public:
    class OSMSCOUT_API Postprocessor : public Referencable
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
    class OSMSCOUT_API StartPostprocessor : public Postprocessor
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
    class OSMSCOUT_API TargetPostprocessor : public Postprocessor
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
    class OSMSCOUT_API DistanceAndTimePostprocessor : public Postprocessor
    {
    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Places a name description as way description
     */
    class OSMSCOUT_API WayNamePostprocessor : public Postprocessor
    {
    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Places a crossing ways description as a description of the name of all ways crossing the given node
     */
    class OSMSCOUT_API CrossingWaysPostprocessor : public Postprocessor
    {
    private:
      void AddCrossingWaysDescriptions(RouteDescription::CrossingWaysDescription* description,
                                       const RouteDescription::Node& node,
                                       const WayRef& originWay,
                                       const WayRef& targetWay,
                                       const std::map<FileOffset,WayRef>& wayMap);

    public:
      bool Process(const RoutingProfile& profile,
                   RouteDescription& description,
                   Database& database);
    };

    /**
     * Places a turn description for every node
     */
    class OSMSCOUT_API DirectionPostprocessor : public Postprocessor
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
      OSMSCOUT_HASHSET<TypeId> motorwayTypes;
      OSMSCOUT_HASHSET<TypeId> motorwayLinkTypes;

      bool                     inRoundabout;
      size_t                   roundaboutCrossingCounter;

    private:
      State GetInitialState(RouteDescription::Node& node,
                            std::map<FileOffset,WayRef>& wayMap);

      void HandleRoundaboutEnter(RouteDescription::Node& node);
      void HandleRoundaboutNode(RouteDescription::Node& node);
      void HandleRoundaboutLeave(RouteDescription::Node& node);
      void HandleDirectMotorwayEnter(RouteDescription::Node& node,
                                     const RouteDescription::NameDescriptionRef& toName);
      void HandleDirectMotorwayLeave(RouteDescription::Node& node,
                                     const RouteDescription::NameDescriptionRef& fromName);
      bool HandleNameChange(const std::list<RouteDescription::Node>& path,
                            std::list<RouteDescription::Node>::const_iterator& lastNode,
                            std::list<RouteDescription::Node>::iterator& node,
                            const std::map<FileOffset,WayRef>& wayMap);
      bool HandleDirectionChange(const std::list<RouteDescription::Node>& path,
                                 std::list<RouteDescription::Node>::iterator& node,
                                 const std::map<FileOffset,WayRef>& wayMap);

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
}

#endif
