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

#include <osmscout/routing/RoutePostprocessor.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>

#include <osmscout/LocationDescriptionService.h>

#include <iostream>
#include <string_view>

namespace osmscout {

  RoutePostprocessor::Postprocessor::~Postprocessor()
  {
    // no code
  }

  RoutePostprocessor::StartPostprocessor::StartPostprocessor(const std::string& startDescription)
  : startDescription(startDescription)
  {
    // no code
  }

  bool RoutePostprocessor::StartPostprocessor::Process(const RoutePostprocessor& /*postprocessor*/,
                                                       RouteDescription& description)
  {
    if (!description.Nodes().empty()) {
      description.Nodes().front().AddDescription(RouteDescription::NODE_START_DESC,
                                                 std::make_shared<RouteDescription::StartDescription>(startDescription));
    }

    return true;
  }

  RoutePostprocessor::TargetPostprocessor::TargetPostprocessor(const std::string& targetDescription)
  : targetDescription(targetDescription)
  {
    // no code
  }

  bool RoutePostprocessor::TargetPostprocessor::Process(const RoutePostprocessor& /*postprocessor*/,
                                                        RouteDescription& description)
  {
    if (!description.Nodes().empty()) {
      description.Nodes().back().AddDescription(RouteDescription::NODE_TARGET_DESC,
                                                std::make_shared<RouteDescription::TargetDescription>(targetDescription));
    }

    return true;
  }

  bool RoutePostprocessor::DistanceAndTimePostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                                 RouteDescription& description)
  {
    ObjectFileRef prevObject;
    GeoCoord      prevCoord(0.0,0.0);

    ObjectFileRef curObject;
    GeoCoord      curCoord(0.0,0.0);

    AreaRef       area;
    WayRef        way;

    Distance      distance;
    Duration      time(0);

    for (auto& node : description.Nodes()) {
      // The last node does not have a pathWayId set, since we are not going anywhere!
      if (node.HasPathObject()) {
        // Only load the next way, if it is different from the old one
        curObject=node.GetPathObject();

        if (curObject!=prevObject) {
          switch (node.GetPathObject().GetType()) {
          case refNone:
          case refNode:
            assert(false);
            break;
          case refArea:
            area=postprocessor.GetArea(node.GetDBFileOffset());
            break;
          case refWay:
            way=postprocessor.GetWay(node.GetDBFileOffset());
            break;
          }
        }

        switch (node.GetPathObject().GetType()) {
        case refNone:
        case refNode:
          assert(false);
          break;
        case refArea:
          curCoord=area->rings.front().GetCoord(node.GetCurrentNodeIndex());
          break;
        case refWay:
          curCoord=way->GetCoord(node.GetCurrentNodeIndex());
        }

        // There is no delta for the first route node
        if (prevObject.Valid()) {
          Distance deltaDistance=GetEllipsoidalDistance(prevCoord,
                                                        curCoord);

          Duration deltaTime(0);

          if (node.GetPathObject().GetType()==refArea) {
            deltaTime=postprocessor.GetTime(node.GetDatabaseId(),
                                            *area,
                                            deltaDistance);
          }
          else if (node.GetPathObject().GetType()==refWay) {
            deltaTime=postprocessor.GetTime(node.GetDatabaseId(),
                                            *way,
                                            deltaDistance);
          }

          distance+=deltaDistance;
          time+=deltaTime;
        }
      }

      node.SetDistance(distance);
      node.SetTime(time);
      node.SetLocation(curCoord);

      prevObject=curObject;
      prevCoord=curCoord;
    }

    return true;
  }

  bool RoutePostprocessor::WayNamePostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                         RouteDescription& description)
  {
    //
    // Store the name of each way
    //

    for (auto node=description.Nodes().begin();
         node!=description.Nodes().end();
         ++node) {
      // The last node does not have a pathWayId set, since we are not going anywhere from the target node!
      if (!node->HasPathObject()) {
        break;
      }

      if (node->GetPathObject().GetType()==refArea) {
        RouteDescription::NameDescriptionRef nameDesc=postprocessor.GetNameDescription(*node);

        node->AddDescription(RouteDescription::WAY_NAME_DESC,
                             nameDesc);
      }
      else if (node->GetPathObject().GetType()==refWay) {
        WayRef                               way=postprocessor.GetWay(node->GetDBFileOffset());
        RouteDescription::NameDescriptionRef nameDesc=postprocessor.GetNameDescription(node->GetDatabaseId(),*way);

        if (postprocessor.IsBridge(*node) &&
            node!=description.Nodes().begin()) {
          auto lastNode=node;

          lastNode--;

          RouteDescription::DescriptionRef lastDescription=lastNode->GetDescription(RouteDescription::WAY_NAME_DESC);
          RouteDescription::NameDescriptionRef lastDesc=std::dynamic_pointer_cast<RouteDescription::NameDescription>(lastDescription);


          if (lastDesc &&
              lastDesc->GetRef()==nameDesc->GetRef() &&
              lastDesc->GetName()!=nameDesc->GetName()) {
            nameDesc=lastDesc;
          }
        }

        node->AddDescription(RouteDescription::WAY_NAME_DESC,
                             nameDesc);
      }
    }

    return true;
  }

  bool RoutePostprocessor::WayTypePostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                         RouteDescription& description)
  {
    //
    // Store the name of each way
    //

    for (auto& node : description.Nodes()) {
      // The last node does not have a pathWayId set, since we are not going anywhere from the target node!
      if (!node.HasPathObject()) {
        break;
      }

      if (node.GetPathObject().GetType()==refArea) {
        AreaRef                                  area=postprocessor.GetArea(node.GetDBFileOffset());
        RouteDescription::TypeNameDescriptionRef typeNameDesc=std::make_shared<RouteDescription::TypeNameDescription>(area->GetType()->GetName());

        node.AddDescription(RouteDescription::WAY_TYPE_NAME_DESC,
                             typeNameDesc);
      }
      else if (node.GetPathObject().GetType()==refWay) {
        WayRef                                   way=postprocessor.GetWay(node.GetDBFileOffset());
        RouteDescription::TypeNameDescriptionRef typeNameDesc=std::make_shared<RouteDescription::TypeNameDescription>(way->GetType()->GetName());

        node.AddDescription(RouteDescription::WAY_TYPE_NAME_DESC,
                             typeNameDesc);
      }
    }

    return true;
  }

  void RoutePostprocessor::CrossingWaysPostprocessor::AddCrossingWaysDescriptions(const RoutePostprocessor& postprocessor,
                                                                                  const RouteDescription::CrossingWaysDescriptionRef& description,
                                                                                  const RouteDescription::Node& node,
                                                                                  const ObjectFileRef& originObject,
                                                                                  const ObjectFileRef& targetObject)
  {
    for (const auto& object : node.GetObjects()) {
      // Way is origin way and starts or ends here so it is not an additional crossing way
      if (originObject.Valid() &&
          object==originObject &&
          postprocessor.IsNodeStartOrEndOfObject(node,
                                                 originObject)) {
        continue;
      }

      // Way is target way and starts or ends here so it is not an additional crossing way
      if (targetObject.Valid() &&
          object==targetObject &&
          postprocessor.IsNodeStartOrEndOfObject(node,
                                                 targetObject)) {
        continue;
      }

      // ways is origin way and target way so it is not an additional crossing way
      if (originObject.Valid() &&
          targetObject.Valid() &&
          object==originObject &&
          object==targetObject) {
        continue;
      }

      description->AddDescription(postprocessor.GetNameDescription(node.GetDatabaseId(),object));
    }
  }

  bool RoutePostprocessor::CrossingWaysPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                              RouteDescription& description)
  {
    //
    // Analyze crossing
    //

    auto lastJunction=description.Nodes().end();
    auto lastNode=description.Nodes().end();
    auto node=description.Nodes().begin();

    while (node!=description.Nodes().end()) {
      // We only analyze junctions, that means nodes that cross other objects
      if (node->GetObjects().empty()) {
        lastNode=node;
        ++node;

        continue;
      }

      // We ignore the first and the last node
      if (lastNode==description.Nodes().end() ||
          !node->HasPathObject()) {
        lastNode=node;
        ++node;

        continue;
      }

      // Count existing exits, that means, the number of objects that can be used to leave the current
      // node
      size_t exitCount=0;

      Id nodeId=postprocessor.GetNodeId(*node);

      size_t currentNodeIndexOnLastPath=postprocessor.GetNodeIndex(*lastNode,
                                                                   nodeId);

      for (const auto& object : node->GetObjects()) {
        bool canUseForward=postprocessor.CanUseForward(node->GetDatabaseId(),
                                                       nodeId,
                                                       object);
        bool canUseBackward=postprocessor.CanUseBackward(node->GetDatabaseId(),
                                                         nodeId,
                                                         object);

        // We can travel this way in the forward direction
        if (canUseForward) {
          // And it is not the way back to the last routing node
          if (lastNode->GetPathObject()!=object ||
              postprocessor.IsRoundabout(*lastNode) ||
              !postprocessor.IsForwardPath(lastNode->GetPathObject(),
                                           currentNodeIndexOnLastPath,
                                           lastNode->GetCurrentNodeIndex())) {
            exitCount++;
          }
        }

        // We can travel this way in the backward direction
        if (canUseBackward) {
          // And it is not the way to back the last routing node
          if (lastNode->GetPathObject()!=object ||
              !postprocessor.IsBackwardPath(lastNode->GetPathObject(),
                                            currentNodeIndexOnLastPath,
                                            lastNode->GetCurrentNodeIndex())) {
            exitCount++;
          }
        }
      }

      RouteDescription::CrossingWaysDescriptionRef desc=std::make_shared<RouteDescription::CrossingWaysDescription>(exitCount,
                                                                                                                    postprocessor.GetNameDescription(*lastNode),
                                                                                                                    postprocessor.GetNameDescription(*node));
      AddCrossingWaysDescriptions(postprocessor,
                                  desc,
                                  *node,
                                  lastNode->GetPathObject(),
                                  node->GetPathObject());

      node->AddDescription(RouteDescription::CROSSING_WAYS_DESC,
                           desc);

      lastNode=node;
      lastJunction=node;

      node++;
    }

    return true;
  }

  const double RoutePostprocessor::DirectionPostprocessor::curveMinInitialAngle=5.0;
  const double RoutePostprocessor::DirectionPostprocessor::curveMaxInitialAngle=10.0;
  const Distance RoutePostprocessor::DirectionPostprocessor::curveMaxNodeDistance=Distance::Of<Kilometer>(0.020);
  const Distance RoutePostprocessor::DirectionPostprocessor::curveMaxDistance=Distance::Of<Kilometer>(0.300);
  const double RoutePostprocessor::DirectionPostprocessor::curveMinAngle=5.0;

  bool RoutePostprocessor::DirectionPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                           RouteDescription& description)
  {
    std::list<RouteDescription::Node>::const_iterator prevNode=description.Nodes().end();
    for (auto node=description.Nodes().begin();
         node!=description.Nodes().end();
         prevNode=node++) {
      auto nextNode=node;

      nextNode++;

      if (prevNode!=description.Nodes().end() &&
          nextNode!=description.Nodes().end() &&
          nextNode->HasPathObject()) {

        GeoCoord prevCoord=postprocessor.GetCoordinates(*prevNode,
                                                        prevNode->GetCurrentNodeIndex());

        GeoCoord coord=postprocessor.GetCoordinates(*node,
                                                    node->GetCurrentNodeIndex());

        GeoCoord nextCoord=postprocessor.GetCoordinates(*nextNode,
                                                        nextNode->GetCurrentNodeIndex());

        double inBearing=GetSphericalBearingFinal(prevCoord,coord).AsDegrees();
        double outBearing=GetSphericalBearingInitial(coord,nextCoord).AsDegrees();

        double turnAngle= NormalizeRelativeAngle(outBearing - inBearing);

        double curveAngle=turnAngle;

        if (fabs(turnAngle)>=curveMinInitialAngle && fabs(turnAngle)<=curveMaxInitialAngle) {
          auto   curveB=nextNode;
          double currentBearing=outBearing;
          Distance forwardDistance=nextNode->GetDistance()-node->GetDistance();
          auto   lookup=nextNode;

          lookup++;
          while (true) {
            // Next node does not exists or does not have a path?
            if (lookup==description.Nodes().end() ||
                !lookup->HasPathObject()) {
              break;
            }

            // Next node is to far away from last node?
            if (lookup->GetDistance()-curveB->GetDistance()>curveMaxNodeDistance) {
              break;
            }

            // Next node is to far away from turn origin?
            if (forwardDistance+lookup->GetDistance()-curveB->GetDistance()>curveMaxDistance) {
              break;
            }

            forwardDistance+=lookup->GetDistance()-curveB->GetDistance();

            GeoCoord curveBCoord=postprocessor.GetCoordinates(*curveB,
                                                              curveB->GetCurrentNodeIndex());

            GeoCoord lookupCoord=postprocessor.GetCoordinates(*lookup,
                                                              lookup->GetCurrentNodeIndex());

            double lookupBearing=GetSphericalBearingInitial(curveBCoord,lookupCoord).AsDegrees();


            double lookupAngle= NormalizeRelativeAngle(lookupBearing - currentBearing);

            // The next node does not have enough direction change to be still part of a turn?
            if (fabs(lookupAngle)<curveMinAngle) {
              break;
            }
            // The turn direction changes?
            if ((turnAngle>0 && lookupAngle<=0) ||
                (turnAngle<0 && lookupAngle>=0)) {
              break;
            }

            currentBearing=lookupBearing;
            curveAngle= NormalizeRelativeAngle(currentBearing - inBearing);

            curveB++;
            lookup++;
          }
        }

        node->AddDescription(RouteDescription::DIRECTION_DESC,
                             std::make_shared<RouteDescription::DirectionDescription>(turnAngle,curveAngle));
      }
    }

    return true;
  }

  bool RoutePostprocessor::MotorwayJunctionPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                                  RouteDescription& description)
  {
     ObjectFileRef           prevObject;
     DatabaseId              prevDatabase=0;
     ObjectFileRef           curObject;
     DatabaseId              curDatabase;

     WayRef                  way;
     std::string             junctionRef;
     std::string             junctionName;

     for (auto& node: description.Nodes()) {
       junctionName="";
       junctionRef="";

       // The last node does not have a pathWayId set, since we are not going anywhere!
       if (!node.HasPathObject()) {
         continue;
       }

       // Only load the next way, if it is different from the old one
       curObject=node.GetPathObject();
       curDatabase=node.GetDatabaseId();

       if (curObject==prevObject && curDatabase==prevDatabase) {
         continue;
       }

       switch (node.GetPathObject().GetType()) {
       case refNone:
         assert(false);
         break;
       case refNode:
         assert(false);
         break;
       case refArea:
         // TODO: junction in area?
         break;
       case refWay:
         way=postprocessor.GetWay(node.GetDBFileOffset());

         GeoCoord coord=way->GetCoord(node.GetCurrentNodeIndex());
         if (!postprocessor.LoadJunction(curDatabase,
                                         coord,
                                         junctionRef,
                                         junctionName)){
           log.Error() << "Error loading junction!";
           return false;
         }

         break;
       }

       if (!junctionName.empty() || !junctionRef.empty()) {
         RouteDescription::NameDescriptionRef nameDescription=std::make_shared<RouteDescription::NameDescription>(junctionName,
                                                                                                                  junctionRef);
         node.AddDescription(RouteDescription::MOTORWAY_JUNCTION_DESC,
                             std::make_shared<RouteDescription::MotorwayJunctionDescription>(nameDescription));
       }

       prevObject=curObject;
       prevDatabase=curDatabase;
     }

     return true;
  }

  bool RoutePostprocessor::DestinationPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                             RouteDescription& description)
  {
    auto          lastJunction=description.Nodes().end();
    ObjectFileRef prevObject;
    DatabaseId    prevDb=0;
    ObjectFileRef curObject;
    DatabaseId    curDb;

    for (auto node=description.Nodes().begin(); node!=description.Nodes().end(); ++node) {
      if (!node->GetObjects().empty()) {
        lastJunction=node;
      }

      // The last node does not have a pathWayId set, since we are not going anywhere!
      if (node->HasPathObject()) {
        // Only load the next way, if it is different from the old one
        curObject=node->GetPathObject();
        curDb=node->GetDatabaseId();

        if (curObject==prevObject && curDb==prevDb){
          continue;
        }

        if (lastJunction!=description.Nodes().end()){
          RouteDescription::DestinationDescriptionRef dest=postprocessor.GetDestination(*node);
          if (dest){
            lastJunction->AddDescription(RouteDescription::CROSSING_DESTINATION_DESC,
                                         dest);
          }
        }

        prevObject=curObject;
        prevDb=curDb;
      }
    }

    return true;
  }

  bool RoutePostprocessor::MaxSpeedPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                          RouteDescription& description)
  {
    ObjectFileRef              prevObject;
    DatabaseId                 prevDb=0;
    ObjectFileRef              curObject;
    DatabaseId                 curDb;

    uint8_t                    speed=0;

    for (auto& node : description.Nodes()) {
      // The last node does not have a pathWayId set, since we are not going anywhere!
      if (node.HasPathObject()) {
        // Only load the next way, if it is different from the old one
        curObject=node.GetPathObject();
        curDb=node.GetDatabaseId();

        if (curObject!=prevObject || curDb!=prevDb) {
          speed=postprocessor.GetMaxSpeed(node);
        }

        if (speed!=0) {
          node.AddDescription(RouteDescription::WAY_MAXSPEED_DESC,
                              std::make_shared<RouteDescription::MaxSpeedDescription>(speed));
        }

        prevObject=curObject;
        prevDb=curDb;
      }
    }

    return true;
  }


  RoutePostprocessor::InstructionPostprocessor::State RoutePostprocessor::InstructionPostprocessor::GetInitialState(const RoutePostprocessor& postprocessor,
                                                                                                                    RouteDescription::Node& node)
  {
    if (!node.HasPathObject()) {
      return street;
    }

    if (postprocessor.IsRoundabout(node)) {
      return roundabout;
    }

    if (postprocessor.IsMotorwayLink(node)){
        return link;
    }
    if (postprocessor.IsMotorway(node)){
        return motorway;
    }

    return street;
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutEnter(const RoutePostprocessor& postprocessor,
                                                                           RouteDescription::Node& node)
  {
    WayRef way;
    auto pathObject=node.GetPathObject();
    if(pathObject.GetType()==refWay){
      way=postprocessor.GetWay(DBFileOffset{node.GetDatabaseId(), pathObject.GetFileOffset()});
    }
    roundaboutClockwise=false;
    if (way && way->nodes.size()>=3){
      // roundabout is closed ring, so we can use method for area
      roundaboutClockwise=AreaIsClockwise(way->nodes);
    }

    RouteDescription::RoundaboutEnterDescriptionRef desc=std::make_shared<RouteDescription::RoundaboutEnterDescription>(roundaboutClockwise);

    node.AddDescription(RouteDescription::ROUNDABOUT_ENTER_DESC,
                        desc);
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutNode(RouteDescription::Node& node)
  {
    if (node.HasDescription(RouteDescription::CROSSING_WAYS_DESC)) {
      RouteDescription::CrossingWaysDescriptionRef crossing=std::dynamic_pointer_cast<RouteDescription::CrossingWaysDescription>(node.GetDescription(RouteDescription::CROSSING_WAYS_DESC));

      if (crossing->GetExitCount()>1) {
        roundaboutCrossingCounter+=crossing->GetExitCount()-1;
      }
    }
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutLeave(RouteDescription::Node& node)
  {
    RouteDescription::RoundaboutLeaveDescriptionRef desc=std::make_shared<RouteDescription::RoundaboutLeaveDescription>(roundaboutCrossingCounter, roundaboutClockwise);

    node.AddDescription(RouteDescription::ROUNDABOUT_LEAVE_DESC,
                        desc);
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleDirectMotorwayEnter(RouteDescription::Node& node,
                                                                               const RouteDescription::NameDescriptionRef& toName)
  {
    RouteDescription::MotorwayEnterDescriptionRef desc=std::make_shared<RouteDescription::MotorwayEnterDescription>(toName);

    node.AddDescription(RouteDescription::MOTORWAY_ENTER_DESC,
                        desc);
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleDirectMotorwayLeave(RouteDescription::Node& node,
                                                                               const RouteDescription::NameDescriptionRef& fromName)
  {
    RouteDescription::MotorwayLeaveDescriptionRef desc=std::make_shared<RouteDescription::MotorwayLeaveDescription>(fromName);

    node.AddDescription(RouteDescription::MOTORWAY_LEAVE_DESC,
                        desc);
  }

  bool RoutePostprocessor::InstructionPostprocessor::HandleNameChange(const std::list<RouteDescription::Node>& path,
                                                                      std::list<RouteDescription::Node>::const_iterator& lastNode,
                                                                      std::list<RouteDescription::Node>::iterator& node)
  {
    RouteDescription::NameDescriptionRef nextName;
    RouteDescription::NameDescriptionRef lastName;

    if (lastNode==path.end()) {
      return false;
    }

    lastName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));
    nextName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(node->GetDescription(RouteDescription::WAY_NAME_DESC));

    // Nothing changed
    if (lastName->GetName()==nextName->GetName() &&
        lastName->GetRef()==nextName->GetRef()) {
      return false;
    }

    // The name changes from empty to non-empty, but the ref stays the same
    if (lastName->GetName().empty() &&
        !nextName->GetName().empty() &&
        lastName->GetRef()==nextName->GetRef()) {
      return false;
    }

    // The name changes from non-empty to empty, but the ref stays the same
    if (!lastName->GetName().empty() &&
        nextName->GetName().empty() &&
        lastName->GetRef()==nextName->GetRef()) {
      return false;
    }

    // ref changes, but name stays the same
    if (!lastName->GetName().empty() &&
        lastName->GetName()==nextName->GetName()) {
      return false;
    }

    node->AddDescription(RouteDescription::WAY_NAME_CHANGED_DESC,
                         std::make_shared<RouteDescription::NameChangedDescription>(lastName,
                                                                                    nextName));

    return true;
  }

  bool RoutePostprocessor::InstructionPostprocessor::HandleDirectionChange(const std::list<RouteDescription::Node>& path,
                                                                           std::list<RouteDescription::Node>::iterator& node)
  {
    if (node->GetObjects().size()<=1){
      return false;
    }

    std::list<RouteDescription::Node>::const_iterator lastNode=node;
    RouteDescription::NameDescriptionRef              nextName;
    RouteDescription::NameDescriptionRef              lastName;

    --lastNode;

    if (lastNode==path.end()) {
      return false;
    }

    lastName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));
    nextName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(node->GetDescription(RouteDescription::WAY_NAME_DESC));

    RouteDescription::DescriptionRef          desc=node->GetDescription(RouteDescription::DIRECTION_DESC);
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(desc);

    if (lastName &&
        nextName &&
        directionDesc &&
        lastName->GetName()==nextName->GetName() &&
        lastName->GetRef()==nextName->GetRef()) {
      if (directionDesc->GetCurve()!=RouteDescription::DirectionDescription::slightlyLeft &&
          directionDesc->GetCurve()!=RouteDescription::DirectionDescription::straightOn &&
          directionDesc->GetCurve()!=RouteDescription::DirectionDescription::slightlyRight) {

          node->AddDescription(RouteDescription::TURN_DESC,
                               std::make_shared<RouteDescription::TurnDescription>());

          return true;
      }
    }
    else if (directionDesc &&
        directionDesc->GetCurve()!=RouteDescription::DirectionDescription::straightOn) {

      node->AddDescription(RouteDescription::TURN_DESC,
                           std::make_shared<RouteDescription::TurnDescription>());

      return true;
    }

    return false;
  }

  bool RoutePostprocessor::InstructionPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                             RouteDescription& description)
  {
    //
    // Detect initial state
    //

    State  state=GetInitialState(postprocessor,
                                 description.Nodes().front());

    inRoundabout=false;
    roundaboutCrossingCounter=0;

    if (state==roundabout) {
      inRoundabout=true;
    }

    //
    // Analyze crossing
    //

    std::list<RouteDescription::Node>::iterator       node=description.Nodes().begin();
    std::list<RouteDescription::Node>::const_iterator lastNode=description.Nodes().end();
    while (node!=description.Nodes().end()) {
      RouteDescription::NameDescriptionRef originName;
      RouteDescription::NameDescriptionRef targetName;

      // First or last node
      if (lastNode==description.Nodes().end() ||
          lastNode->GetPathObject().Invalid() ||
          node->GetPathObject().Invalid()) {
        lastNode=node++;
        continue;
      }

      originName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));

      if (node->HasPathObject()) {
        targetName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(node->GetDescription(RouteDescription::WAY_NAME_DESC));
      }

      if (!postprocessor.IsRoundabout(*lastNode) &&
          postprocessor.IsRoundabout(*node)) {
        inRoundabout=true;
        roundaboutCrossingCounter=0;

        HandleRoundaboutEnter(postprocessor, *node);

        lastNode=node++;

        continue;
      }

      if (postprocessor.IsRoundabout(*lastNode) &&
          !postprocessor.IsRoundabout(*node)) {
        HandleRoundaboutNode(*node);

        HandleRoundaboutLeave(*node);

        inRoundabout=false;

        lastNode=node++;

        continue;
      }

      // Non-Link, non-motorway to motorway (enter motorway))
      if (!postprocessor.IsMotorwayLink(*lastNode) &&
          !postprocessor.IsMotorway(*lastNode) &&
          postprocessor.IsMotorway(*node)) {
        HandleDirectMotorwayEnter(*node,
                                  targetName);

        lastNode=node++;

        continue;
      }

      // motorway to Non-Link, non-motorway /leave motorway)
      if (postprocessor.IsMotorway(*lastNode) &&
          !postprocessor.IsMotorwayLink(*node) &&
          !postprocessor.IsMotorway(*node)) {
        HandleDirectMotorwayLeave(*node,
                                  originName);

        lastNode=node++;

        continue;
      }

      else if (!postprocessor.IsMotorwayLink(*lastNode) &&
               postprocessor.IsMotorwayLink(*node)) {
        bool                                 originIsMotorway=postprocessor.IsMotorway(*lastNode);
        bool                                 targetIsMotorway=false;
        auto                                 next=node;
        RouteDescription::NameDescriptionRef nextName;

        next++;
        while (next!=description.Nodes().end() &&
               next->HasPathObject()) {

          nextName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(next->GetDescription(RouteDescription::WAY_NAME_DESC));

          if (!postprocessor.IsMotorwayLink(*next)) {
            break;
          }

          next++;
        }

        if (next->GetPathObject().Valid()) {
          targetIsMotorway=postprocessor.IsMotorway(*next);
        }

        if (originIsMotorway && targetIsMotorway) {
          RouteDescription::MotorwayChangeDescriptionRef desc=std::make_shared<RouteDescription::MotorwayChangeDescription>(originName,
                                                                                                                            nextName);

          node->AddDescription(RouteDescription::MOTORWAY_CHANGE_DESC,
                               desc);

          node=next;
          lastNode=node++;

          continue;

        }

        if (originIsMotorway && !targetIsMotorway) {
          RouteDescription::MotorwayLeaveDescriptionRef desc=std::make_shared<RouteDescription::MotorwayLeaveDescription>(originName);

          node->AddDescription(RouteDescription::MOTORWAY_LEAVE_DESC,
                               desc);

          HandleDirectionChange(description.Nodes(),
                                next);

          node=next;
          lastNode=node++;

          continue;
        }

        if (!originIsMotorway && targetIsMotorway) {
          RouteDescription::MotorwayEnterDescriptionRef desc=std::make_shared<RouteDescription::MotorwayEnterDescription>(nextName);

          node->AddDescription(RouteDescription::MOTORWAY_ENTER_DESC,
                               desc);

          node=next;
          lastNode=node++;

          continue;
        }

        lastNode=node++;

        continue;
      }

      if (inRoundabout) {
        HandleRoundaboutNode(*node);
      }
      else if (HandleDirectionChange(description.Nodes(),
                                     node)) {
        lastNode=node++;

        continue;
      }

      if (HandleNameChange(description.Nodes(),
                           lastNode,
                           node)) {
        lastNode=node++;

        continue;
      }

      lastNode=node++;
    }

    return true;
  }

  class AdminRegionPathCollectorVisitor : public AdminRegionVisitor
  {
  private:
    const Database&            database;
    const std::list<WayRef>&   ways;
    const std::list<AreaRef>&  areas;

  public:
    std::list<AdminRegionRef> regions;

  public:
    explicit AdminRegionPathCollectorVisitor(const Database& database,
                                             const std::list<WayRef>&   ways,
                                             const std::list<AreaRef>&  areas)
      : database(database),
        ways(ways),
        areas(areas)
    {
      // no code
    }

    Action Visit(const AdminRegion& region) override
    {
      AreaRef regionArea;

      if (!database.GetAreaByOffset(region.object.GetFileOffset(),
                                    regionArea)) {
        return error;
      }

      GeoBox regionBox=regionArea->GetBoundingBox();

      AdminRegionRef regionRef=std::make_shared<AdminRegion>(region);
      bool           candidates=false;

      for (const auto& way : ways) {
        GeoBox box=way->GetBoundingBox();

        if (box.Intersects(regionBox)) {
          candidates=true;
        }
      }

      for (const auto& area : areas) {
        GeoBox box=area->GetBoundingBox();

        if (box.Intersects(regionBox)) {
          candidates=true;
        }
      }


      if (candidates) {
        regions.push_back(regionRef);

        return visitChildren;
      }

      return skipChildren;
    }
  };

  class LocationNameByPathCollectorVisitor : public LocationVisitor
  {
  public:
    const std::set<ObjectFileRef>&                 paths;
    std::map<std::string,std::list<ObjectFileRef>> namePathsMap;

  public:
    explicit LocationNameByPathCollectorVisitor(const std::set<ObjectFileRef>& paths)
      : paths(paths)
    {

    }

    bool Visit(const AdminRegion& /*adminRegion*/,
               const PostalArea& /*postalArea*/,
               const Location &location) override
    {
      for (const auto& object : location.objects) {
        if (paths.find(object)!=paths.end()) {
          namePathsMap[location.name].push_back(object);
        }
      }

      return true;
    }

  };

  class LocationByNameCollectorVisitor : public LocationVisitor
  {
  public:
    const std::map<std::string,std::list<ObjectFileRef>>& namePathsMap;
    std::list<LocationRef>                                locations;
    std::map<FileOffset,std::list<ObjectFileRef>>         locationPathsMap;

  public:
    explicit LocationByNameCollectorVisitor(const std::map<std::string,std::list<ObjectFileRef>>& namePathsMap)
      : namePathsMap(namePathsMap)
    {
    }

    bool Visit(const AdminRegion& /*adminRegion*/,
               const PostalArea& /*postalArea*/,
               const Location &location) override
    {
      const auto namePathEntry=namePathsMap.find(location.name);

      if (namePathEntry!=namePathsMap.end()) {
        LocationRef locationRef=std::make_shared<Location>(location);

        locations.push_back(locationRef);
        locationPathsMap[location.locationOffset]=namePathEntry->second;
      }

      return true;
    }
  };

  class AddressCollectorVisitor : public AddressVisitor
  {
  public:
    const std::map<FileOffset,std::list<ObjectFileRef>>&   locationPathsMap;
    std::map<ObjectFileRef,std::set<ObjectFileRef>>&       poiCandidates;

  public:
    AddressCollectorVisitor(const std::map<FileOffset,std::list<ObjectFileRef>>& locationPathsMap,
                            std::map<ObjectFileRef,std::set<ObjectFileRef>>& poiCandidates)
      : locationPathsMap(locationPathsMap),
        poiCandidates(poiCandidates)
    {
    }

    bool Visit(const AdminRegion& /*adminRegion*/,
               const PostalArea& /*postalArea*/,
               const Location &location,
               const Address& address) override
    {
      const auto locationPaths=locationPathsMap.find(location.locationOffset);

      for (const auto& path: locationPaths->second) {
        poiCandidates[path].insert(address.object);
      }

      return true;
    }
  };

  std::set<ObjectFileRef> RoutePostprocessor::POIsPostprocessor::CollectPaths(const std::list<RouteDescription::Node>& nodes) const
  {
    ObjectFileRef           prevObject;
    ObjectFileRef           curObject;
    std::set<ObjectFileRef> pathSet;

    for (auto& pathNode : nodes) {
      if (pathNode.HasPathObject()) {
        curObject=pathNode.GetPathObject();

        if (curObject!=prevObject) {
          pathSet.insert(pathNode.GetPathObject());
        }
      }

      prevObject=curObject;
    }

    return pathSet;
  }

  std::list<WayRef> RoutePostprocessor::POIsPostprocessor::CollectWays(const RoutePostprocessor& postprocessor,
                                                                       const std::list<RouteDescription::Node>& nodes) const
  {
    ObjectFileRef     prevObject;
    ObjectFileRef     curObject;
    std::list<WayRef> ways;

    for (auto& pathNode : nodes) {
      if (pathNode.HasPathObject()) {
        curObject=pathNode.GetPathObject();

        if (curObject!=prevObject) {
          if (pathNode.GetPathObject().GetType()==refWay) {
            ways.push_back(postprocessor.GetWay(pathNode.GetDBFileOffset()));
          }
        }
      }

      prevObject=curObject;
    }

    return ways;
  }

  std::list<AreaRef> RoutePostprocessor::POIsPostprocessor::CollectAreas(const RoutePostprocessor& postprocessor,
                                                                         const std::list<RouteDescription::Node>& nodes) const
  {
    ObjectFileRef      prevObject;
    ObjectFileRef      curObject;
    std::list<AreaRef> areas;

    for (auto& pathNode : nodes) {
      if (pathNode.HasPathObject()) {
        curObject=pathNode.GetPathObject();

        if (curObject!=prevObject) {
          if (pathNode.GetPathObject().GetType()==refArea) {
            areas.push_back(postprocessor.GetArea(pathNode.GetDBFileOffset()));
          }
        }
      }

      prevObject=curObject;
    }

    return areas;
  }

  std::map<ObjectFileRef,std::set<ObjectFileRef>> RoutePostprocessor::POIsPostprocessor::CollectPOICandidates(const Database& database,
                                                                                                              const std::set<ObjectFileRef>& paths,
                                                                                                              const std::list<WayRef>& ways,
                                                                                                              const std::list<AreaRef>& areas)
  {
    std::map<ObjectFileRef,std::set<ObjectFileRef>> poiCandidates;

    StopClock scanLocationTime;

    AdminRegionPathCollectorVisitor regionCollector(database,
                                                    ways,
                                                    areas);

    database.GetLocationIndex()->VisitAdminRegions(regionCollector);

    for (const auto& adminRegion : regionCollector.regions) {
      LocationNameByPathCollectorVisitor locationCollector(paths);

      database.GetLocationIndex()->VisitLocations(*adminRegion,
                                                  locationCollector,
                                                  false);

      if (locationCollector.namePathsMap.empty()) {
        continue;
      }

      LocationByNameCollectorVisitor location2Collector(locationCollector.namePathsMap);

      database.GetLocationIndex()->VisitLocations(*adminRegion,
                                                   location2Collector,
                                                   false);

      AddressCollectorVisitor addressCollector(location2Collector.locationPathsMap,
                                               poiCandidates);

      for (const auto& location : location2Collector.locations) {
        AdminRegion fakeRegion;
        PostalArea  fakePostalArea;

        database.GetLocationIndex()->VisitAddresses(*adminRegion,
                                                    fakePostalArea,
                                                    *location,
                                                    addressCollector);
      }
    }

    scanLocationTime.Stop();

    std::cout << "Scanning locations: " << scanLocationTime.ResultString() << std::endl;

    return poiCandidates;
  }

  std::map<ObjectFileRef,RoutePostprocessor::POIsPostprocessor::POIAtRoute>
    RoutePostprocessor::POIsPostprocessor::AnalysePOICandidates(const RoutePostprocessor& postprocessor,
                                                                const DatabaseId& databaseId,
                                                                std::list<RouteDescription::Node>& nodes,
                                                                const TypeInfoSet& nodeTypes,
                                                                const TypeInfoSet& areaTypes,
                                                                const std::unordered_map<FileOffset,NodeRef>& nodeMap,
                                                                const std::unordered_map<FileOffset,AreaRef>& areaMap,
                                                                const std::map<ObjectFileRef,std::set<ObjectFileRef>>& poiCandidates)
  {
    std::map<ObjectFileRef,RoutePostprocessor::POIsPostprocessor::POIAtRoute> poisAtRoute;

    NodeRef                node;
    AreaRef                area;

    StopClock analysingCandidatesTime;

    for (auto routeNode=nodes.begin();
         routeNode!=nodes.end();
         routeNode++) {
      auto nextNode=routeNode;

      nextNode++;

      if (nextNode==nodes.end() ||
          !routeNode->HasPathObject()) {
        continue;
      }

      GeoCoord curCoord=routeNode->GetLocation();
      GeoCoord nextCoord=nextNode->GetLocation();

      const auto pathPoiCandidates=poiCandidates.find(routeNode->GetPathObject());

      if (pathPoiCandidates!=poiCandidates.end()) {
        for (const auto& candidate : pathPoiCandidates->second) {
          GeoCoord    location;
          GeoCoord    intersection;
          double      r;
          RouteDescription::NameDescriptionRef name;

          switch (candidate.GetType()) {
          case refNone:
          case refWay:
            assert(false);
            break;
          case refNode:
            node=nodeMap.at(candidate.GetFileOffset());
            if (!nodeTypes.IsSet(node->GetType())) {
              continue;
            }
            name=postprocessor.GetNameDescription(databaseId,*node);
            location=node->GetCoords();
            DistanceToSegment(location,
                              curCoord,
                              nextCoord,
                              r,
                              intersection);
            break;
          case refArea:
            area=areaMap.at(candidate.GetFileOffset());
            if (!areaTypes.IsSet(area->GetType())) {
              continue;
            }
            name=postprocessor.GetNameDescription(databaseId,*area);
            DistanceToSegment(area->GetBoundingBox(),
                              curCoord,
                              nextCoord,
                              location,
                              intersection);
            break;
          }


          if (!name) {
            continue;
          }

          Distance distance=GetEllipsoidalDistance(location,
                                                   intersection);

          if (distance<Distance::Of<Meter>(30)) {
            auto existingPoiAtRoute=poisAtRoute.find(candidate);

            if (existingPoiAtRoute!=poisAtRoute.end() &&
                existingPoiAtRoute->second.distance<distance) {
              continue;
            }

            POIAtRoute poiAtRoute;

            poiAtRoute.distance=distance;
            poiAtRoute.name=name;
            poiAtRoute.object=candidate;
            poiAtRoute.node=routeNode;

            poisAtRoute[candidate]=poiAtRoute;
          }
        }
      }
    }

    analysingCandidatesTime.Stop();

    std::cout << "Analysing candidates: " << analysingCandidatesTime.ResultString() << std::endl;

    return poisAtRoute;
  }

  void RoutePostprocessor::POIsPostprocessor::SortInCollectedPOIs(const DatabaseId& databaseId,
                                                                  const std::map<ObjectFileRef,POIAtRoute>& pois)
  {
    for (const auto& poiAtRoute : pois) {
      RouteDescription::POIAtRouteDescriptionRef desc=std::make_shared<RouteDescription::POIAtRouteDescription>(databaseId,
                                                                                                                poiAtRoute.second.object,
                                                                                                                poiAtRoute.second.name,
                                                                                                                poiAtRoute.second.distance);
      poiAtRoute.second.node->AddDescription(RouteDescription::POI_AT_ROUTE_DESC,
                                             desc);
    }
  }

  bool RoutePostprocessor::POIsPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                      RouteDescription& description)
  {
    TypeInfoSet        nodeTypes;
    TypeInfoSet        areaTypes;
    NodeRef            node;
    WayRef             way;
    AreaRef            area;

    StopClock loadDataTime;

    std::set<ObjectFileRef> paths=CollectPaths(description.Nodes());
    std::list<WayRef>       ways=CollectWays(postprocessor,description.Nodes());
    std::list<AreaRef>      areas=CollectAreas(postprocessor,description.Nodes());

    loadDataTime.Stop();
    std::cout << "Loading path data: " << loadDataTime.ResultString() << std::endl;

    StopClock loadingCandidatesTime;

    for (DatabaseId databaseId=0; databaseId<postprocessor.databases.size(); databaseId++) {
      auto database=postprocessor.databases[databaseId];

      for (const auto& type : database->GetTypeConfig()->GetTypes()) {
        if (type->IsInGroup("routingPOI")) {
          if (type->CanBeNode()) {
            nodeTypes.Set(type);
          }

          if (type->CanBeArea()) {
            areaTypes.Set(type);
          }
        }
      }

      std::map<ObjectFileRef,std::set<ObjectFileRef>> poiCandidates=CollectPOICandidates(*database,
                                                                                         paths,
                                                                                         ways,
                                                                                         areas);

      ways.clear();
      areas.clear();

      std::set<FileOffset> nodeOffsets;
      std::set<FileOffset> areaOffsets;

      for (const auto& pathPoiCandidates: poiCandidates) {
        for (const auto& candidate : pathPoiCandidates.second) {
          switch (candidate.GetType()) {
          case refNone:
          case refWay:
            assert(false);
            break;
          case refNode:
            nodeOffsets.insert(candidate.GetFileOffset());
            break;
          case refArea:
            areaOffsets.insert(candidate.GetFileOffset());
            break;
          }
        }
      }

      std::unordered_map<FileOffset,NodeRef> nodeMap;
      std::unordered_map<FileOffset,AreaRef> areaMap;

      database->GetNodesByOffset(nodeOffsets,
                                 nodeMap);

      database->GetAreasByOffset(areaOffsets,
                                 areaMap);

      loadingCandidatesTime.Stop();

      std::cout << "Loading candidates: " << loadingCandidatesTime.ResultString() << std::endl;

      std::map<ObjectFileRef,POIAtRoute> poisAtRoute=AnalysePOICandidates(postprocessor,
                                                                          databaseId,
                                                                          description.Nodes(),
                                                                          nodeTypes,
                                                                          areaTypes,
                                                                          nodeMap,
                                                                          areaMap,
                                                                          poiCandidates);

      SortInCollectedPOIs(databaseId,
                          poisAtRoute);
    }

    return true;
  }

  bool RoutePostprocessor::LanesPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                       RouteDescription& description)
  {
    ObjectFileRef              prevObject;
    DatabaseId                 prevDb=0;
    ObjectFileRef              curObject;
    DatabaseId                 curDb;

    RouteDescription::LaneDescriptionRef lanes;

    for (auto& node : description.Nodes()) {
      // The last node does not have a pathWayId set, since we are not going anywhere!
      if (node.HasPathObject()) {
        // Only load the next way, if it is different from the old one
        curObject=node.GetPathObject();
        curDb=node.GetDatabaseId();

        if (curObject!=prevObject || curDb!=prevDb) {
          lanes=postprocessor.GetLanes(node);
        }
        if (lanes) {
          node.AddDescription(RouteDescription::LANES_DESC, lanes);
        }

        prevObject=curObject;
        prevDb=curDb;
      }
    }

    return true;
  }

  bool RoutePostprocessor::SuggestedLanesPostprocessor::Process(const RoutePostprocessor& /*postprocessor*/,
                                                                RouteDescription& description)
  {
    using namespace std::string_view_literals;

    auto GetLaneDescription = [](const RouteDescription::Node &node) -> RouteDescription::LaneDescriptionRef {
        return std::dynamic_pointer_cast<RouteDescription::LaneDescription>(node.GetDescription(RouteDescription::LANES_DESC));
    };

    // buffer of traveled nodes, recent node at back
    std::list<RouteDescription::Node*> backBuffer;
    for (auto& node : description.Nodes()) {

      while (!backBuffer.empty() &&
             (node.GetDistance() - backBuffer.front()->GetDistance() > Meters(500))){
        backBuffer.pop_front();
      }
      auto lanes = GetLaneDescription(node);
      if (!lanes){
        // it should not happened, just on last node
        backBuffer.clear();
        continue;
      }

      if (!backBuffer.empty() && !node.GetObjects().empty()) { // we know history and there is some crossing on current node
        auto prevLanes = GetLaneDescription(*backBuffer.back());
        assert(prevLanes);
        if (prevLanes->GetLaneCount() > lanes->GetLaneCount()) { // lane count was decreased
          RouteDescription::DirectionDescriptionRef direction = std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(node.GetDescription(RouteDescription::DIRECTION_DESC));

          using Move = RouteDescription::DirectionDescription::Move;
          Move directionMove = direction ? direction->GetTurn() : Move::straightOn;

          int allowedLaneFrom = -1;
          int allowedLaneTo = -1; // inclusive

          auto LookupLanesTurns = [&](const std::set<std::string_view> &possibilities){
            for (size_t i = 0; i < prevLanes->GetLaneTurns().size(); i++){
              std::string_view turn = prevLanes->GetLaneTurns()[i];
              if (possibilities.find(turn) != possibilities.end()){
                // it is possible to use this turn
                if (allowedLaneFrom < 0) {
                  allowedLaneFrom=i;
                  allowedLaneTo=i;
                } else {
                  allowedLaneTo=i;
                }
              } else {
                if (allowedLaneFrom>0){
                  break;
                }
              }
            }
          };

          static const std::set<std::string_view> leftPossibilities{
            "left"sv, "slight_left"sv, "through;left"sv, "through;slight_left"sv, "through;sharp_left"sv};

          static const std::set<std::string_view> straightPossibilities{
            "through;left"sv, "through;slight_left"sv, "through;sharp_left"sv,
            "through"sv,
            ""sv, // no-sign implicitly as through
            "through;right"sv, "through;slight_right"sv, "through;sharp_right"sv};

          static const std::set<std::string_view> rightPossibilities{
              "right"sv, "slight_right"sv, "through;right"sv, "through;slight_right"sv, "through;sharp_right"sv};

          // after some direction change, we will evaluate allowed lanes in backBuffer
          if (!prevLanes->GetLaneTurns().empty()){
            // we know explicit lane turns
            switch (directionMove){
              case Move::sharpLeft:
              case Move::left:
              case Move::slightlyLeft:
                LookupLanesTurns(leftPossibilities);
                break;
              case Move::straightOn:
                LookupLanesTurns(straightPossibilities);
                break;
              case Move::slightlyRight:
              case Move::right:
              case Move::sharpRight:
                LookupLanesTurns(rightPossibilities);
                break;
            }
          }
          if (allowedLaneFrom < 0){
            // explicit turns are not available, or evaluation was not successful
            // it may happen when we detected move is for example slightlyLeft but lane turn is "through"

            // so, just estimate lanes on the count
            assert(lanes->GetLaneCount()>0);
            switch (directionMove){
              case Move::sharpLeft:
              case Move::left:
              case Move::slightlyLeft:
                allowedLaneFrom = 0;
                allowedLaneTo = lanes->GetLaneCount() - 1;
                break;
              case Move::straightOn:
                // ignore right now, we cannot estimate which lanes
                // are through without deeper analysis
                break;
              case Move::slightlyRight:
              case Move::right:
              case Move::sharpRight:
                allowedLaneFrom = prevLanes->GetLaneCount() - lanes->GetLaneCount();
                allowedLaneTo = prevLanes->GetLaneCount() -1;
                break;
            }
          }

          if (allowedLaneFrom >= 0) {
            assert(allowedLaneTo >= allowedLaneFrom);
            auto suggested = std::make_shared<RouteDescription::SuggestedLaneDescription>(allowedLaneFrom, allowedLaneTo);
            for (auto it = backBuffer.rbegin(); it != backBuffer.rend(); it++) {
              auto nodePtr = *it;
              auto nodeLanes = GetLaneDescription(*nodePtr);
              if (*prevLanes != *nodeLanes){
                break;
              }
              nodePtr->AddDescription(RouteDescription::SUGGESTED_LANES_DESC, suggested);
            }
          }

          backBuffer.clear();
        }
      }

      backBuffer.push_back(&node);
    }
    return true;
  }

  RoutePostprocessor::RoutePostprocessor()
  {
  }

  bool RoutePostprocessor::ResolveAllAreasAndWays(const RouteDescription& description,
                                                  DatabaseId dbId,
                                                  Database& database)
  {
    std::set<FileOffset>         areaOffsets;
    std::vector<AreaRef>         areas;

    std::set<FileOffset>         wayOffsets;
    std::vector<WayRef>          ways;

    for (const auto &node : description.Nodes()) {
      if (node.GetDatabaseId()!=dbId){
        continue;
      }
      if (node.HasPathObject()) {
        switch (node.GetPathObject().GetType()) {
        case refNone:
        case refNode:
          assert(false);
          break;
        case refArea:
          areaOffsets.insert(node.GetPathObject().GetFileOffset());
        break;
        case refWay:
          wayOffsets.insert(node.GetPathObject().GetFileOffset());
          break;
        }
      }

      for (const auto &object : node.GetObjects()) {
        switch (object.GetType()) {
        case refNone:
        case refNode:
          assert(false);
          break;
        case refArea:
          areaOffsets.insert(object.GetFileOffset());
        break;
        case refWay:
          wayOffsets.insert(object.GetFileOffset());
          break;
        }
      }
    }

    if (!database.GetWaysByOffset(wayOffsets,ways)) {
      log.Error() << "Cannot retrieve crossing ways";
      return false;
    }

    wayOffsets.clear();

    for (const auto& way : ways) {
      wayMap[DBFileOffset(dbId,way->GetFileOffset())]=way;
    }

    ways.clear();

    if (!database.GetAreasByOffset(areaOffsets,areas)) {
      log.Error() << "Cannot retrieve crossing areas";
      return false;
    }

    areaOffsets.clear();

    for (const auto& area : areas) {
      areaMap[DBFileOffset(dbId,area->GetFileOffset())]=area;
    }

    areas.clear();

    return true;
  }

  void RoutePostprocessor::Cleanup()
  {
    areaMap.clear();
    wayMap.clear();

    motorwayTypes.clear();
    motorwayLinkTypes.clear();

    for (const auto p:nameReaders){
      delete p.second;
    }
    nameReaders.clear();

    for (const auto p:refReaders){
      delete p.second;
    }
    refReaders.clear();

    for (const auto p:bridgeReaders){
      delete p.second;
    }
    bridgeReaders.clear();

    for (const auto p:roundaboutReaders){
      delete p.second;
    }
    roundaboutReaders.clear();

    for (const auto p:destinationReaders){
      delete p.second;
    }
    destinationReaders.clear();

    for (const auto p:maxSpeedReaders){
      delete p.second;
    }
    maxSpeedReaders.clear();

    for (const auto p:lanesReaders){
      delete p.second;
    }
    lanesReaders.clear();
  }

  AreaRef RoutePostprocessor::GetArea(const DBFileOffset &offset) const
  {
    auto entry=areaMap.find(offset);

    assert(entry!=areaMap.end());

    return entry->second;
  }

  WayRef RoutePostprocessor::GetWay(const DBFileOffset &offset) const
  {
    auto entry=wayMap.find(offset);

    assert(entry!=wayMap.end());

    return entry->second;
  }

  Duration RoutePostprocessor::GetTime(DatabaseId dbId,const Area& area,const Distance &deltaDistance) const
  {
    assert(dbId<profiles.size() && profiles[dbId]);
    auto profile=profiles[dbId];
    return profile->GetTime(area,deltaDistance);
  }

  Duration RoutePostprocessor::GetTime(DatabaseId dbId,const Way& way,const Distance &deltaDistance) const
  {
    assert(dbId<profiles.size() && profiles[dbId]);
    auto profile=profiles[dbId];
    return profile->GetTime(way,deltaDistance);
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::GetNameDescription(const RouteDescription::Node& node) const
  {
    return GetNameDescription(node.GetDatabaseId(),node.GetPathObject());
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::GetNameDescription(DatabaseId dbId,
                                                                              const ObjectFileRef& object) const
  {
    RouteDescription::NameDescriptionRef description;

    if (object.GetType()==refArea) {
      AreaRef area=GetArea(DBFileOffset(dbId,object.GetFileOffset()));

      return GetNameDescription(dbId,*area);
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(dbId,object.GetFileOffset()));

      return GetNameDescription(dbId,*way);
    }
    else {
      assert(false);
    }

    return description;
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::GetNameDescription(DatabaseId dbId,
                                                                              const Node& node) const
  {
    auto nameReader=nameReaders.find(dbId);
    assert(nameReader!=nameReaders.end());
    NameFeatureValue *nameValue=nameReader->second->GetValue(node.GetFeatureValueBuffer());
    std::string      name;

    if (nameValue!=nullptr) {
      name=nameValue->GetName();
    }

    return std::make_shared<RouteDescription::NameDescription>(name);
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::GetNameDescription(DatabaseId dbId,
                                                                              const Area& area) const
  {
    auto nameReader=nameReaders.find(dbId);
    assert(nameReader!=nameReaders.end());
    NameFeatureValue *nameValue=nameReader->second->GetValue(area.rings.front().GetFeatureValueBuffer());
    std::string      name;

    if (nameValue!=nullptr) {
      name=nameValue->GetName();
    }

    return std::make_shared<RouteDescription::NameDescription>(name);
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::GetNameDescription(DatabaseId dbId,
                                                                              const Way& way) const
  {
    auto nameReader=nameReaders.find(dbId);
    assert(nameReader!=nameReaders.end());
    auto refReader=refReaders.find(dbId);
    assert(refReader!=refReaders.end());

    NameFeatureValue *nameValue=nameReader->second->GetValue(way.GetFeatureValueBuffer());
    RefFeatureValue  *refValue=refReader->second->GetValue(way.GetFeatureValueBuffer());
    std::string      name;
    std::string      ref;

    if (nameValue!=nullptr) {
      name=nameValue->GetName();
    }

    if (refValue!=nullptr) {
      ref=refValue->GetRef();
    }

    return std::make_shared<RouteDescription::NameDescription>(name,ref);
  }

  bool RoutePostprocessor::LoadJunction(DatabaseId dbId,
                                        GeoCoord coord,
                                        std::string junctionRef,
                                        std::string junctionName) const
  {
    double                  delta=1E-7;
    std::vector<FileOffset> nodeOffsets;
    std::vector<NodeRef>    nodes;

    auto nameReaderIt=nameReaders.find(dbId);
    assert(nameReaderIt!=nameReaders.end());
    NameFeatureValueReader* nameReader=nameReaderIt->second;

    auto refReaderIt=refReaders.find(dbId);
    assert(refReaderIt!=refReaders.end());
    RefFeatureValueReader* refReader=refReaderIt->second;

    assert(dbId<databases.size() && databases[dbId]);
    DatabaseRef database=databases[dbId];

    AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();

    if (!areaNodeIndex) {
      return false;
    }

    auto it=junctionTypes.find(dbId);
    assert(it!=junctionTypes.end());
    TypeInfoSet nodeTypes=it->second;

    TypeInfoSet loadedTypes;
    GeoBox boundingBox(GeoCoord(coord.GetLat()-delta,coord.GetLon()-delta),
                       GeoCoord(coord.GetLat()+delta,coord.GetLon()+delta));

    nodeOffsets.clear();
    if (!areaNodeIndex->GetOffsets(boundingBox,
                                   nodeTypes,
                                   nodeOffsets,
                                   loadedTypes)) {
      log.Error() << "Error getting nodes from area node index!";
      return false;
    }

    if (nodeOffsets.empty()) {
      return true;
    }

    nodes.clear();
    std::sort(nodeOffsets.begin(),nodeOffsets.end());

    if (!database->GetNodesByOffset(nodeOffsets,
                                   nodes)) {
      log.Error() << "Error reading nodes in area!";
      return false;
    }

    for (const auto& node : nodes) {
      if (fabs(node->GetCoords().GetLat() - coord.GetLat()) < delta &&
          fabs(node->GetCoords().GetLon() - coord.GetLon()) < delta) {
        RefFeatureValue *refFeatureValue=refReader->GetValue(node->GetFeatureValueBuffer());

        if (refFeatureValue!=nullptr) {
          junctionRef=refFeatureValue->GetRef();
        }

        NameFeatureValue *nameFeatureValue=nameReader->GetValue(node->GetFeatureValueBuffer());

        if (nameFeatureValue!=nullptr) {
          junctionName = nameFeatureValue->GetName();
        }

        break;
      }
    }

    return true;
  }

  bool RoutePostprocessor::IsMotorwayLink(const RouteDescription::Node& node) const
  {
    auto types=motorwayLinkTypes.find(node.GetDatabaseId());
    assert(types!=motorwayLinkTypes.end());

    if (node.GetPathObject().GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());
      return types->second.IsSet(area->GetType());
    }
    else if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());
      return types->second.IsSet(way->GetType());
    }

    return false;
  }

  bool RoutePostprocessor::IsMotorway(const RouteDescription::Node& node) const
  {
    auto types=motorwayTypes.find(node.GetDatabaseId());
    assert(types!=motorwayTypes.end());

    if (node.GetPathObject().GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());
      return types->second.IsSet(area->GetType());
    }
    else if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());
      return types->second.IsSet(way->GetType());
    }

    return false;
  }

  bool RoutePostprocessor::IsRoundabout(const RouteDescription::Node& node) const
  {
    if (node.GetPathObject().GetType()==refArea) {
      return false;
    }
    else if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      auto roundaboutReader=roundaboutReaders.find(node.GetDatabaseId());
      assert(roundaboutReader!=roundaboutReaders.end());
      return roundaboutReader->second->IsSet(way->GetFeatureValueBuffer());
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::IsBridge(const RouteDescription::Node& node) const
  {
    if (node.GetPathObject().GetType()==refWay) {
      auto bridgeReader=bridgeReaders.find(node.GetDatabaseId());
      assert(bridgeReader!=bridgeReaders.end());
      WayRef way=GetWay(node.GetDBFileOffset());
      return bridgeReader->second->IsSet(way->GetFeatureValueBuffer());
    }
    return false;
  }

  RouteDescription::DestinationDescriptionRef RoutePostprocessor::GetDestination(const RouteDescription::Node& node) const
  {
    RouteDescription::DestinationDescriptionRef dest;
    if (node.GetPathObject().GetType()==refWay) {
      auto destinationReader=destinationReaders.find(node.GetDatabaseId());
      assert(destinationReader!=destinationReaders.end());
      WayRef way=GetWay(node.GetDBFileOffset());

      DestinationFeatureValue *destinationValue=destinationReader->second->GetValue(way->GetFeatureValueBuffer());
      if (destinationValue!=nullptr){
        std::string destination=destinationValue->GetDestination();
        dest=std::make_shared<RouteDescription::DestinationDescription>(destination);
      }
    }
    return dest;
  }

  uint8_t RoutePostprocessor::GetMaxSpeed(const RouteDescription::Node& node) const
  {
    uint8_t speed=0;
    if (node.GetPathObject().GetType()==refWay) {
      auto maxSpeedReader=maxSpeedReaders.find(node.GetDatabaseId());
      assert(maxSpeedReader!=maxSpeedReaders.end());
      WayRef way=GetWay(node.GetDBFileOffset());

      MaxSpeedFeatureValue *maxSpeedValue=maxSpeedReader->second->GetValue(way->GetFeatureValueBuffer());
      if (maxSpeedValue!=nullptr) {
        speed=maxSpeedValue->GetMaxSpeed();
      }
      else {
        speed=0;
      }
    }
    return speed;
  }

  RouteDescription::LaneDescriptionRef RoutePostprocessor::GetLanes(const RouteDescription::Node& node) const
  {
    RouteDescription::LaneDescriptionRef lanes;
    if (node.GetPathObject().GetType()==refWay) {
      auto lanesReader=lanesReaders.find(node.GetDatabaseId());
      auto accessReader=accessReaders.find(node.GetDatabaseId());
      assert(lanesReader != lanesReaders.end());
      assert(accessReader != accessReaders.end());

      WayRef way=GetWay(node.GetDBFileOffset());

      bool forward = node.GetCurrentNodeIndex() < node.GetTargetNodeIndex();

      AccessFeatureValue *accessValue=accessReader->second->GetValue(way->GetFeatureValueBuffer());
      bool oneway = accessValue ? accessValue->IsOneway() : false;

      uint8_t laneCount;
      std::vector<std::string> laneTurns;
      LanesFeatureValue *lanesValue=lanesReader->second->GetValue(way->GetFeatureValueBuffer());
      if (lanesValue!=nullptr) {
        laneCount=std::max((uint8_t)1,forward ? lanesValue->GetForwardLanes() : lanesValue->GetBackwardLanes());
        std::string turns=forward ? lanesValue->GetTurnForward() : lanesValue->GetTurnBackward();;
        std::list<std::string> turnList=SplitString(turns, "|", laneCount);
        laneTurns.reserve(turnList.size());
        laneTurns.insert(laneTurns.begin(), turnList.begin(), turnList.end());
      } else {
        // default lane count by object type
        if (oneway) {
          laneCount=way->GetType()->GetOnewayLanes();
        } else {
          laneCount=std::max(1,way->GetType()->GetLanes()/2);
        }
      }
      lanes=std::make_shared<RouteDescription::LaneDescription>(oneway, laneCount, laneTurns);
    }
    return lanes;
  }

  Id RoutePostprocessor::GetNodeId(const RouteDescription::Node& node) const
  {
    const ObjectFileRef& object=node.GetPathObject();
    size_t nodeIndex=node.GetCurrentNodeIndex();
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());

      return area->rings.front().nodes[nodeIndex].GetId();
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      return way->GetId(nodeIndex);
    }
    else {
      assert(false);

      return 0;
    }
  }

  size_t RoutePostprocessor::GetNodeIndex(const RouteDescription::Node& node,
                                          Id nodeId) const
  {
    const ObjectFileRef& object=node.GetPathObject();
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());

      for (size_t i=0; i<area->rings.front().nodes.size(); i++) {
        if (area->rings.front().nodes[i].GetId()==nodeId) {
          return i;
        }

      }

      assert(false);

      return 0;
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      size_t index;

      if (!way->GetNodeIndexByNodeId(nodeId,
                                     index)) {
        assert(false);
      }

      return index;
    }
    else {
      assert(false);

      return 0;
    }
  }

  bool RoutePostprocessor::CanUseBackward(const DatabaseId& dbId,
                                          Id fromNodeId,
                                          const ObjectFileRef& object) const
  {
    assert(dbId<profiles.size() && profiles[dbId]);
    auto profile=profiles[dbId];

    if (object.GetType()==refArea) {
      AreaRef area=GetArea(DBFileOffset(dbId,object.GetFileOffset()));

      return profile->CanUse(*area);
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(dbId,object.GetFileOffset()));

      size_t fromNodeIndex;

      if (!way->GetNodeIndexByNodeId(fromNodeId,
                                     fromNodeIndex)) {
        assert(false);
      }

      return fromNodeIndex>0 &&
             profile->CanUseBackward(*way);
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::CanUseForward(const DatabaseId& dbId,
                                         Id fromNodeId,
                                         const ObjectFileRef& object) const
  {
    assert(dbId<profiles.size() && profiles[dbId]);
    auto profile=profiles[dbId];

    if (object.GetType()==refArea) {
      AreaRef area=GetArea(DBFileOffset(dbId,object.GetFileOffset()));

      return profile->CanUse(*area);
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(dbId,object.GetFileOffset()));

      size_t fromNodeIndex;

      if (!way->GetNodeIndexByNodeId(fromNodeId,
                                     fromNodeIndex)) {
        assert(false);
      }

      return fromNodeIndex!=way->nodes.size()-1 &&
             profile->CanUseForward(*way);
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::IsBackwardPath(const ObjectFileRef& object,
                                          size_t fromNodeIndex,
                                          size_t toNodeIndex) const
  {
    if (object.GetType()==refArea) {
      return true;
    }
    else if (object.GetType()==refWay) {
      return toNodeIndex<fromNodeIndex;
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::IsForwardPath(const ObjectFileRef& object,
                                         size_t fromNodeIndex,
                                         size_t toNodeIndex) const
  {
    if (object.GetType()==refArea) {
      return true;
    }
    else if (object.GetType()==refWay) {
      return toNodeIndex>fromNodeIndex;
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::IsNodeStartOrEndOfObject(const RouteDescription::Node& node,
                                                    const ObjectFileRef& object) const
  {
    size_t nodeIndex=node.GetCurrentNodeIndex();
    Id nodeId;

    if (node.GetPathObject().GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());

      nodeId=area->rings.front().GetId(nodeIndex);
    }
    else if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      nodeId=way->GetId(nodeIndex);
    }
    else {
      assert(false);

      return false;
    }

    if (object.GetType()==refArea) {
      return false;
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(node.GetDatabaseId(),object.GetFileOffset()));

      return way->GetFrontId()==nodeId ||
             way->GetBackId()==nodeId;
    }
    else {
      assert(false);

      return false;
    }
  }

  GeoCoord RoutePostprocessor::GetCoordinates(const RouteDescription::Node& node,
                                              size_t nodeIndex) const
  {
    if (node.GetPathObject().GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());

      return area->rings.front().GetCoord(nodeIndex);
    }
    else if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      return way->GetCoord(nodeIndex);
    }
    else {
      assert(false);
      return GeoCoord();
    }
  }

  bool RoutePostprocessor::PostprocessRouteDescription(RouteDescription& description,
                                                       const std::vector<RoutingProfileRef>& profiles,
                                                       const std::vector<DatabaseRef>& databases,
                                                       const std::list<PostprocessorRef>& processors,
                                                       const std::set<std::string>& motorwayTypeNames,
                                                       const std::set<std::string>& motorwayLinkTypeNames,
                                                       const std::set<std::string>& junctionTypeNames)
  {
    Cleanup(); // We do not trust ourself ;-)

    this->databases=databases;
    this->profiles=profiles;

    for (DatabaseId dbIdx=0; dbIdx<databases.size(); dbIdx++){
      // init feature readers
      DatabaseId dbId=dbIdx;
      DatabaseRef database=databases[dbIdx];
      TypeConfigRef typeConfig=database->GetTypeConfig();

      nameReaders[dbId]=new NameFeatureValueReader(*typeConfig);
      refReaders[dbId]=new RefFeatureValueReader(*typeConfig);
      bridgeReaders[dbId]=new BridgeFeatureReader(*typeConfig);
      roundaboutReaders[dbId]=new RoundaboutFeatureReader(*typeConfig);
      destinationReaders[dbId]=new DestinationFeatureValueReader(*typeConfig);
      maxSpeedReaders[dbId]=new MaxSpeedFeatureValueReader(*typeConfig);
      lanesReaders[dbId]=new LanesFeatureValueReader(*typeConfig);
      accessReaders[dbId]=new AccessFeatureValueReader(*typeConfig);

      // init types
      motorwayTypes[dbId]; // insert empty TypeInfoSet
      for (const std::string &typeName:motorwayTypeNames){
        TypeInfoRef type=typeConfig->GetTypeInfo(typeName);
        motorwayTypes[dbId].Set(type);
      }

      motorwayLinkTypes[dbId]; // insert empty TypeInfoSet
      for (const std::string &typeName:motorwayLinkTypeNames){
        TypeInfoRef type=typeConfig->GetTypeInfo(typeName);
        motorwayLinkTypes[dbId].Set(type);
      }

      junctionTypes[dbId]; // insert empty TypeInfoSet
      for (const std::string &typeName:junctionTypeNames){
        TypeInfoRef type=typeConfig->GetTypeInfo(typeName);
        junctionTypes[dbId].Set(type);
      }

      // load objects
      if (!ResolveAllAreasAndWays(description,
                                  dbId,
                                  *database)) {
        Cleanup();
        return false;
      }
    }

    size_t pos=1;
    for (const auto& processor : processors) {
      if (!processor->Process(*this,description)) {
        log.Error() << "Error during execution of postprocessor " << pos;
        Cleanup();

        return false;
      }

      pos++;
    }

    Cleanup();

    return true;
  }
}
