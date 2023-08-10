#ifndef LIBOSMSCOUT_ROUTEDESCRIPTIONPOSTPROCESSOR_H
#define LIBOSMSCOUT_ROUTEDESCRIPTIONPOSTPROCESSOR_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009-2018  Tim Teulings

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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>

#include <osmscout/ObjectRef.h>
#include <osmscout/Path.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/util/Distance.h>
#include <osmscout/util/Time.h>

namespace osmscout {

  /**
   * The RouteDescriptionPostprocessor does all the heavy lifting of creating the
   * various available Postprocessors, evaluate their feedback and map it onto a set
   * of real-life situation callback methods.
   *
   * Just implement your own derived Callback class and pass it to the generator methods.
   */
  class OSMSCOUT_API RouteDescriptionPostprocessor
  {
  public:
    /**
     * Callback class that gets call in various routing situations.
     */
    struct OSMSCOUT_API Callback
    {
      virtual ~Callback() = default;

      /**
       * Call once before evaluation the the RouteDescription starts
       */
      virtual void BeforeRoute();

      /**
       * Called one for the start node
       *
       * @param startDescription
       * @param typeNameDescription
       * @param nameDescription
       */
      virtual void OnStart(const RouteDescription::StartDescriptionRef& startDescription,
                           const RouteDescription::TypeNameDescriptionRef& typeNameDescription,
                           const RouteDescription::NameDescriptionRef& nameDescription);

      /**
       * Called once for the target node reached
       *
       * @param targetDescription
       */
      virtual void OnTargetReached(const RouteDescription::TargetDescriptionRef& targetDescription);

      /**
       * Call everytime a turn is necessary. Call with all information available regarding the turn
       * and the way turned into and its direction.
       *
       * @param turnDescription
       * @param crossingWaysDescription
       * @param directionDescription
       * @param typeNameDescription
       * @param nameDescription
       */
      virtual void OnTurn(const RouteDescription::TurnDescriptionRef& turnDescription,
                          const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                          const RouteDescription::DirectionDescriptionRef& directionDescription,
                          const RouteDescription::TypeNameDescriptionRef& typeNameDescription,
                          const RouteDescription::NameDescriptionRef& nameDescription);

      /**
       * Called if we enter a roundabout
       *
       * @param roundaboutEnterDescription
       * @param crossingWaysDescription
       */
      virtual void OnRoundaboutEnter(const RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                     const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

      /**
       * Called if we leave a roundabout entered before
       *
       * @param roundaboutLeaveDescription
       * @param nameDescription
       */
      virtual void OnRoundaboutLeave(const RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                     const RouteDescription::NameDescriptionRef& nameDescription);

      /**
       * Called if we enter a motorway
       *
       * @param motorwayEnterDescription
       * @param crossingWaysDescription
       */
      virtual void OnMotorwayEnter(const RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                   const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);
      /**
       * Called if we already on a motorway and switch to another motorway
       *
       * @param motorwayLeaveDescription and and ref of leaving motorway
       * @param motorwayJunctionDescription name and ref of the motorway exit
       * @param directionDescription turn direction (right, left...)
       * @param crossingDestinationDescription semicolon separated list of exit destinations
       */
      virtual void OnMotorwayChange(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                                    const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                                    const RouteDescription::DirectionDescriptionRef& directionDescription,
                                    const RouteDescription::DestinationDescriptionRef& crossingDestinationDescription);
      /**
       * Called if we are on a motorway an leave it to a non-motorway way.
       *
       * @param motorwayLeaveDescription and and ref of leaving motorway
       * @param motorwayJunctionDescription name and ref of the motorway exit
       * @param directionDescription turn direction (right, left...)
       * @param nameDescription name of the way used for leaving
       * @param destinationDescription semicolon separated list of exit destinations
       */
      virtual void OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                   const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                                   const RouteDescription::DirectionDescriptionRef& directionDescription,
                                   const RouteDescription::NameDescriptionRef& nameDescription,
                                   const RouteDescription::DestinationDescriptionRef& destinationDescription);

      /**
       * Called anytime the way we are on changes its name.
       *
       * @param nameChangedDescription
       */
      virtual void OnPathNameChange(const RouteDescription::NameChangedDescriptionRef& nameChangedDescription);

      /**
       * Called everytime we have max speed information for a route segment
       *
       * @param maxSpeedDescription
       */
      virtual void OnMaxSpeed(const RouteDescription::MaxSpeedDescriptionRef& maxSpeedDescription);

      /**
       * Called everytime we have a POI at the route
       *
       * @param poiAtRouteDescription
       *    The POI information
       */
      virtual void OnPOIAtRoute(const RouteDescription::POIAtRouteDescriptionRef& poiAtRouteDescription);
    
      /**
        * Called everytime we have a new section at the route when routing with some via points between start and target
        *
        * @param viaDescription
        *    The via information
        */
      virtual void OnViaAtRoute(const RouteDescription::ViaDescriptionRef& viaDescription);

      /**
       * Always called before we analyse a node. It may be that other callback methods are called
       * or not (normally we only call other methods, if something relevant changes).
       *
       * @param node
       */
      virtual void BeforeNode(const RouteDescription::Node& node);
      /**
       * Called after all possible callback methods for a node are called.
       * @param node
       */
      virtual void AfterNode(const RouteDescription::Node& node);

      /**
       * If postprocessor should continue
       *
       * @return continue
       */
      virtual bool Continue() const;
    };

  public:
    void GenerateDescription(const RouteDescription& description,
                             Callback& callback) const;

    void GenerateDescription(const RouteDescription::NodeIterator &first,
                             const RouteDescription::NodeIterator &last,
                             Callback& callback) const;
  };

}

#endif //LIBOSMSCOUT_ROUTEDESCRIPTIONPOSTPROCESSOR_H
