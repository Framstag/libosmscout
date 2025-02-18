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
#include <osmscout/log/Logger.h>

#include <osmscout/location/LocationDescriptionService.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <numeric>

namespace osmscout {

  RouteDescription::LaneDescription PostprocessorContext::GetLanes(const DatabaseId& dbId, const WayRef& way, bool forward) const
  {
    auto lanesReader = GetLaneReader(dbId);
    auto accessReader = GetAccessReader(dbId);

    AccessFeatureValue *accessValue=accessReader.GetValue(way->GetFeatureValueBuffer());
    bool oneway=accessValue!=nullptr && accessValue->IsOneway();

    uint8_t laneCount;
    std::vector<LaneTurn> laneTurns;
    LanesFeatureValue *lanesValue=lanesReader.GetValue(way->GetFeatureValueBuffer());
    if (lanesValue!=nullptr) {
      laneCount=std::max((uint8_t)1,forward ? lanesValue->GetForwardLanes() : lanesValue->GetBackwardLanes());
      laneTurns=forward ? lanesValue->GetTurnForward() : lanesValue->GetTurnBackward();
      while (laneTurns.size() < laneCount) {
        laneTurns.push_back(LaneTurn::None);
      }
    } else {
      // default lane count by object type
      if (oneway) {
        laneCount=way->GetType()->GetOnewayLanes();
      } else {
        laneCount=std::max(1,way->GetType()->GetLanes()/2);
      }
    }

    return RouteDescription::LaneDescription(oneway, laneCount, laneTurns);
  }

  RouteDescription::LaneDescriptionRef PostprocessorContext::GetLanes(const RouteDescription::Node& node) const
  {
    RouteDescription::LaneDescriptionRef lanes;
    if (node.GetPathObject().GetType()==refWay) {

      WayRef way=GetWay(node.GetDBFileOffset());
      bool forward = node.GetCurrentNodeIndex() < node.GetTargetNodeIndex();

      lanes=std::make_shared<RouteDescription::LaneDescription>(GetLanes(node.GetDatabaseId(), way, forward));
    }
    return lanes;
  }

  Id PostprocessorContext::GetNodeId(const RouteDescription::Node& node) const
  {
    const ObjectFileRef& object=node.GetPathObject();
    size_t nodeIndex=node.GetCurrentNodeIndex();
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());

      return area->rings.front().nodes[nodeIndex].GetId();
    }

    if (object.GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      return way->GetId(nodeIndex);
    }

    assert(false);
    return 0;
  }

  RoutePostprocessor::StartPostprocessor::StartPostprocessor(const std::string& startDescription)
  : startDescription(startDescription)
  {
    // no code
  }

  bool RoutePostprocessor::StartPostprocessor::Process(const PostprocessorContext& /*postprocessor*/,
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

  bool RoutePostprocessor::TargetPostprocessor::Process(const PostprocessorContext& /*postprocessor*/,
                                                        RouteDescription& description)
  {
    if (!description.Nodes().empty()) {
      description.Nodes().back().AddDescription(RouteDescription::NODE_TARGET_DESC,
                                                std::make_shared<RouteDescription::TargetDescription>(targetDescription));
    }

    return true;
  }

  bool RoutePostprocessor::DistanceAndTimePostprocessor::Process(const PostprocessorContext& postprocessor,
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

  bool RoutePostprocessor::WayNamePostprocessor::Process(const PostprocessorContext& postprocessor,
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
            nameDesc=std::move(lastDesc);
          }
        }

        node->AddDescription(RouteDescription::WAY_NAME_DESC,
                             nameDesc);
      }
    }

    return true;
  }

  bool RoutePostprocessor::WayTypePostprocessor::Process(const PostprocessorContext& postprocessor,
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

  void RoutePostprocessor::CrossingWaysPostprocessor::AddCrossingWaysDescriptions(const PostprocessorContext& postprocessor,
                                                                                  const RouteDescription::CrossingWaysDescriptionRef& description,
                                                                                  const RouteDescription::Node& node,
                                                                                  const ObjectFileRef& originObject,
                                                                                  const ObjectFileRef& targetObject)
  {
    for (const auto& object : node.GetObjects()) {
      if (object.type==refNode) {
        continue;
      }

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

  bool RoutePostprocessor::CrossingWaysPostprocessor::Process(const PostprocessorContext& postprocessor,
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
        if (object.type==refNode) {
          continue;
        }
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

  bool RoutePostprocessor::DirectionPostprocessor::Process(const PostprocessorContext& postprocessor,
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
            // Next node does not exist or does not have a path?
            if (lookup==description.Nodes().end() ||
                !lookup->HasPathObject()) {
              break;
            }

            // Next node is too far away from last node?
            if (lookup->GetDistance()-curveB->GetDistance()>curveMaxNodeDistance) {
              break;
            }

            // Next node is too far away from turn origin?
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

  bool RoutePostprocessor::MotorwayJunctionPostprocessor::Process(const PostprocessorContext& postprocessor,
                                                                  RouteDescription& description)
  {
     for (auto& node: description.Nodes()) {
       const DatabaseId dbId=node.GetDatabaseId();

       if (const NodeRef n=postprocessor.GetJunctionNode(node); n){
         RouteDescription::NameDescriptionRef nameDescription = postprocessor.GetNameDescription(dbId, *n);

         if (!nameDescription->GetName().empty() || !nameDescription->GetRef().empty()) {
           node.AddDescription(RouteDescription::MOTORWAY_JUNCTION_DESC,
                               std::make_shared<RouteDescription::MotorwayJunctionDescription>(nameDescription));
         }
       }
     }

     return true;
  }

  bool RoutePostprocessor::DestinationPostprocessor::Process(const PostprocessorContext& postprocessor,
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

  bool RoutePostprocessor::MaxSpeedPostprocessor::Process(const PostprocessorContext& postprocessor,
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


  RoutePostprocessor::InstructionPostprocessor::State RoutePostprocessor::InstructionPostprocessor::GetInitialState(const PostprocessorContext& postprocessor,
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

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutEnter(const PostprocessorContext& postprocessor,
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

  std::vector<RoutePostprocessor::InstructionPostprocessor::NodeExit>
      RoutePostprocessor::InstructionPostprocessor::CollectNodeWays(const PostprocessorContext& postprocessor,
                                                                    RouteDescription::Node& node,
                                                                    bool exitsOnly)
  {
    std::vector<NodeExit> exits;
    if (!node.GetPathObject().IsWay()) {
      return exits; // just ways are supported in this method
    }

    WayRef outgoingWay=postprocessor.GetWay(DBFileOffset(node.GetDatabaseId(), node.GetPathObject().GetFileOffset()));
    Point nodePoint=outgoingWay->nodes[node.GetCurrentNodeIndex()];
    for (const ObjectFileRef &obj: node.GetObjects()) {
      if (obj.IsWay()) {
        WayRef way=postprocessor.GetWay(DBFileOffset(node.GetDatabaseId(), obj.GetFileOffset()));
        for (size_t ni=0; ni<way->nodes.size(); ++ni) {
          if (way->nodes[ni].IsIdentical(nodePoint)) {
            if (ni>0) {
              bool canBeUsedAsExit = postprocessor.CanUseBackward(node.GetDatabaseId(), nodePoint.GetId(), obj);
              if (!exitsOnly || canBeUsedAsExit) {
                exits.push_back({obj, ni-1, GetSphericalBearingInitial(nodePoint.GetCoord(), way->nodes[ni-1].GetCoord()), canBeUsedAsExit});
              }
            }
            if (ni+1<way->nodes.size()) {
              bool canBeUsedAsExit = postprocessor.CanUseForward(node.GetDatabaseId(), nodePoint.GetId(), obj);
              if (!exitsOnly || canBeUsedAsExit) {
                exits.push_back({obj, ni + 1, GetSphericalBearingInitial(nodePoint.GetCoord(), way->nodes[ni + 1].GetCoord()), canBeUsedAsExit});
              }
            }
            break;
          }
        }
      }
    }
    return exits;
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleMiniRoundabout(const PostprocessorContext& postprocessor,
                                                                          RouteDescription::Node& node,
                                                                          ObjectFileRef incomingPath,
                                                                          size_t incomingNode)
  {
    if (incomingPath.type!=refWay) {
      return; // just ways are supported as roundabout exits
    }

    roundaboutClockwise=postprocessor.IsClockwise(node);

    roundaboutCrossingCounter=0;
    RouteDescription::RoundaboutEnterDescriptionRef desc=std::make_shared<RouteDescription::RoundaboutEnterDescription>(roundaboutClockwise);
    node.AddDescription(RouteDescription::ROUNDABOUT_ENTER_DESC,desc);

    // collect roundabout exits (and even inputs, to be able find entering path when it is oneway)
    std::vector<NodeExit> exits=CollectNodeWays(postprocessor, node, false);
    // sort exists by its bearing
    std::sort(exits.begin(), exits.end(), [&](const NodeExit &a, const NodeExit &b) {
      if (roundaboutClockwise) {
        return a.bearing.AsRadians() < b.bearing.AsRadians();
      } else {
        return a.bearing.AsRadians() > b.bearing.AsRadians();
      }
    });

    bool entered=false;
    for (size_t i=0;; ++i) {
      const NodeExit &exit=exits[i%exits.size()];
      if (entered) {
        if (exit.canBeUsedAsExit) {
          roundaboutCrossingCounter++;
          if (exit.ref == node.GetPathObject() && exit.node == node.GetTargetNodeIndex()) {
            break;
          }
        }
      } else if (exit.ref==incomingPath && exit.node==incomingNode) {
        entered=true;
      }
      assert(i<exits.size()*2);
    }

    RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutLeave(node);
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

  void RoutePostprocessor::InstructionPostprocessor::HandleMotorwayLink(const PostprocessorContext& postprocessor,
                                                                        const RouteDescription::NameDescriptionRef &originName,
                                                                        const std::list<RouteDescription::Node>::const_iterator &lastNode,
                                                                        const std::list<RouteDescription::Node>::iterator &node,
                                                                        const std::list<RouteDescription::Node>::const_iterator &end)
  {
    bool                                 originIsMotorway=postprocessor.IsMotorway(*lastNode);
    bool                                 targetIsMotorway=false;
    auto                                 next=node;
    RouteDescription::NameDescriptionRef nextName;

    next++;
    while (next!=end && next->HasPathObject()) {

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
    }
    else if (originIsMotorway && !targetIsMotorway) {
      RouteDescription::MotorwayLeaveDescriptionRef desc=std::make_shared<RouteDescription::MotorwayLeaveDescription>(originName);

      node->AddDescription(RouteDescription::MOTORWAY_LEAVE_DESC,
                           desc);
    }
    else if (!originIsMotorway && targetIsMotorway) {

      // add direction description howto enter motorway link
      HandleDirectionChange(postprocessor, node, end);

      RouteDescription::MotorwayEnterDescriptionRef desc=std::make_shared<RouteDescription::MotorwayEnterDescription>(nextName);

      node->AddDescription(RouteDescription::MOTORWAY_ENTER_DESC,
                           desc);
    }
  }

  bool RoutePostprocessor::InstructionPostprocessor::HandleNameChange(std::list<RouteDescription::Node>::const_iterator& lastNode,
                                                                      std::list<RouteDescription::Node>::iterator& node,
                                                                      const std::list<RouteDescription::Node>::const_iterator &end)
  {
    RouteDescription::NameDescriptionRef nextName;
    RouteDescription::NameDescriptionRef lastName;

    if (lastNode==end) {
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

  bool RoutePostprocessor::InstructionPostprocessor::HandleDirectionChange(const PostprocessorContext& postprocessor,
                                                                           const std::list<RouteDescription::Node>::iterator& node,
                                                                           const std::list<RouteDescription::Node>::const_iterator& end)
  {
    if (node->GetObjects().size()<=1){
      return false;
    }

    std::list<RouteDescription::Node>::const_iterator lastNode=node;
    RouteDescription::NameDescriptionRef              nextName;
    RouteDescription::NameDescriptionRef              lastName;

    --lastNode;

    if (lastNode==end) {
      return false;
    }

    lastName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));
    nextName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(node->GetDescription(RouteDescription::WAY_NAME_DESC));

    RouteDescription::DescriptionRef          desc=node->GetDescription(RouteDescription::DIRECTION_DESC);
    RouteDescription::DirectionDescriptionRef directionDesc=std::dynamic_pointer_cast<RouteDescription::DirectionDescription>(desc);

    if (!directionDesc ||
        directionDesc->GetCurve()==RouteDescription::DirectionDescription::straightOn) {
      return false;
    }

    // When there is no other way in junction that can be used for driving from the junction,
    // we don't have to signalize explicit turn.
    // There may be one-way in forbidden direction or way with forbidden type for current vehicle.
    if (lastNode->GetDatabaseId() == node->GetDatabaseId()) {
      std::vector<NodeExit> exits=CollectNodeExits(postprocessor, *node);
      auto IsIncoming = [&](const auto &exit) { return exit.ref==lastNode->GetPathObject() && exit.node==lastNode->GetCurrentNodeIndex(); };
      auto IsOutgoing = [&](const auto &exit) { return exit.ref==node->GetPathObject() && exit.node==node->GetTargetNodeIndex(); };
      auto IsUsable = [&](const auto &exit) { return !IsIncoming(exit) && !IsOutgoing(exit); };
      auto usableExit=std::find_if(exits.begin(), exits.end(), IsUsable);
      if (usableExit == exits.end()) {
        return false;
      }
    }

    if (lastName &&
        nextName &&
        lastName->GetName()==nextName->GetName() &&
        lastName->GetRef()==nextName->GetRef()) {
      if (directionDesc->GetCurve()!=RouteDescription::DirectionDescription::slightlyLeft &&
          directionDesc->GetCurve()!=RouteDescription::DirectionDescription::slightlyRight) {

          node->AddDescription(RouteDescription::TURN_DESC,
                               std::make_shared<RouteDescription::TurnDescription>());

          return true;
      }
    }
    else {

      node->AddDescription(RouteDescription::TURN_DESC,
                           std::make_shared<RouteDescription::TurnDescription>());

      return true;
    }

    return false;
  }

  bool RoutePostprocessor::InstructionPostprocessor::Process(const PostprocessorContext& postprocessor,
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

      if (postprocessor.IsMiniRoundabout(*node) &&
          !inRoundabout &&
          lastNode->GetDatabaseId() == node->GetDatabaseId()) {
        HandleMiniRoundabout(postprocessor, *node, lastNode->GetPathObject(), lastNode->GetCurrentNodeIndex());

        lastNode=node++;
        continue;
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

      if (!postprocessor.IsMotorwayLink(*lastNode) &&
               postprocessor.IsMotorwayLink(*node)) {

        // adds MotorwayEnter, MotorwayChange or MotorwayLeave depending on what is after motorway_link
        HandleMotorwayLink(postprocessor,
                           originName,
                           lastNode,
                           node,
                           description.Nodes().end());

        lastNode=node++;

        continue;
      }

      if (inRoundabout) {
        HandleRoundaboutNode(*node);
      }
      else if (HandleDirectionChange(postprocessor, node, description.Nodes().end())) {
        lastNode=node++;

        continue;
      }

      if (HandleNameChange(lastNode,
                           node,
                           description.Nodes().end())) {
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

    for (const auto& pathNode : nodes) {
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

  std::list<WayRef> RoutePostprocessor::POIsPostprocessor::CollectWays(const PostprocessorContext& postprocessor,
                                                                       const std::list<RouteDescription::Node>& nodes) const
  {
    ObjectFileRef     prevObject;
    ObjectFileRef     curObject;
    std::list<WayRef> ways;

    for (const auto& pathNode : nodes) {
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

  std::list<AreaRef> RoutePostprocessor::POIsPostprocessor::CollectAreas(const PostprocessorContext& postprocessor,
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

    LocationIndexRef locationIndex=database.GetLocationIndex();
    assert(locationIndex);
    locationIndex->VisitAdminRegions(regionCollector);

    for (const auto& adminRegion : regionCollector.regions) {
      LocationNameByPathCollectorVisitor locationCollector(paths);

      locationIndex->VisitLocations(*adminRegion,
                                    locationCollector,
                                    false);

      if (locationCollector.namePathsMap.empty()) {
        continue;
      }

      LocationByNameCollectorVisitor location2Collector(locationCollector.namePathsMap);

      locationIndex->VisitLocations(*adminRegion,
                                   location2Collector,
                                   false);

      AddressCollectorVisitor addressCollector(location2Collector.locationPathsMap,
                                               poiCandidates);

      for (const auto& location : location2Collector.locations) {
        AdminRegion fakeRegion;
        PostalArea  fakePostalArea;

        locationIndex->VisitAddresses(*adminRegion,
                                      fakePostalArea,
                                      *location,
                                      addressCollector);
      }
    }

    scanLocationTime.Stop();

    std::cout << "Scanning locations: " << scanLocationTime.ResultString() << std::endl;

    locationIndex->FlushCache();
    return poiCandidates;
  }

  std::map<ObjectFileRef,RoutePostprocessor::POIsPostprocessor::POIAtRoute>
    RoutePostprocessor::POIsPostprocessor::AnalysePOICandidates(const PostprocessorContext& postprocessor,
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
            poiAtRoute.name=std::move(name);
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

  bool RoutePostprocessor::POIsPostprocessor::Process(const PostprocessorContext& postprocessor,
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
    auto dbs = postprocessor.GetDatabases();
    for (DatabaseId databaseId=0; databaseId < dbs.size(); databaseId++) {
      auto database = dbs[databaseId];

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

  bool RoutePostprocessor::LanesPostprocessor::Process(const PostprocessorContext& postprocessor,
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


  namespace {
    uint32_t TurnToBits(LaneTurn turn)
    {
      switch (turn) {
        case LaneTurn::None:
          return 0b11111111;
        case LaneTurn::SlightLeft:
          return 0b00000001;
        case LaneTurn::Left:
          return 0b00000010;
        case LaneTurn::SharpLeft:
          return 0b00000100;
        case LaneTurn::Through_SlightLeft:
          return 0b00001001;
        case LaneTurn::Through_Left:
          return 0b00001010;
        case LaneTurn::Through_SharpLeft:
          return 0b00001100;
        case LaneTurn::Through:
          return 0b00001000;
        case LaneTurn::Through_SlightRight:
          return 0b00011000;
        case LaneTurn::Through_Right:
          return 0b00101000;
        case LaneTurn::Through_SharpRight:
          return 0b01001000;
        case LaneTurn::SlightRight:
          return 0b00010000;
        case LaneTurn::Right:
          return 0b00100000;
        case LaneTurn::MergeToRight:
          return 0b01000000;
        default:
          return 0b00000000;
      }
    }

    LaneTurn BitsToTurn(uint32_t turnBits)
    {
      switch (turnBits) {
        case 0b00000001:
          return LaneTurn::SlightLeft;
        case 0b00000010:
          return LaneTurn::Left;
        case 0b00000100:
          return LaneTurn::SharpLeft;
        case 0b00001001:
          return LaneTurn::Through_SlightLeft;
        case 0b00001010:
          return LaneTurn::Through_Left;
        case 0b00001100:
          return LaneTurn::Through_SharpLeft;
        case 0b00001000:
          return LaneTurn::Through;
        case 0b00011000:
          return LaneTurn::Through_SlightRight;
        case 0b00101000:
          return LaneTurn::Through_Right;
        case 0b01001000:
          return LaneTurn::Through_SharpRight;
        case 0b00010000:
          return LaneTurn::SlightRight;
        case 0b00100000:
          return LaneTurn::Right;
        case 0b01000000:
          return LaneTurn::MergeToRight;
        default:
          return LaneTurn::Unknown;
      }
    }

  } // end of anonymous namespace

  void RoutePostprocessor::SuggestedLanesPostprocessor::EvaluateLaneSuggestion(const PostprocessorContext& postprocessor,
                                                                               const RouteDescription::Node &node,
                                                                               const std::list<RouteDescription::Node*> &backBuffer) const
  {
    if (node.GetPathObject().GetType()!=refWay) {
      return; // areas not considered now
    }

    assert(!backBuffer.empty());
    const RouteDescription::Node &prevNode=*backBuffer.back();
    DatabaseId dbId = node.GetDatabaseId();
    if (prevNode.GetDatabaseId() != dbId) {
      return; // we consider nodes just from the same database for simplicity
    }

    auto lanes = GetLaneDescription(node);
    assert(lanes);
    auto prevLanes = GetLaneDescription(prevNode);
    assert(prevLanes);

    // read lanes on the ALL junction ways, use heuristics what lanes we can use to move forward
    Id nodeId = postprocessor.GetNodeId(node);

    WayRef prevWay = postprocessor.GetWay(prevNode.GetDBFileOffset());
    Id prevNodeId = prevWay->GetId(prevNode.GetCurrentNodeIndex());
    // bearing from current node to previous
    Bearing prevNodeBearing = GetSphericalBearingInitial(prevWay->nodes[prevNode.GetTargetNodeIndex()].GetCoord(),
                                                         prevWay->nodes[prevNode.GetCurrentNodeIndex()].GetCoord());

    WayRef way = postprocessor.GetWay(node.GetDBFileOffset());
    Id nextNodeId = way->GetId(node.GetTargetNodeIndex());
    Bearing nextNodeBearing = GetSphericalBearingInitial(way->nodes[node.GetCurrentNodeIndex()].GetCoord(),
                                                         way->nodes[node.GetTargetNodeIndex()].GetCoord());

    struct JunctionExit {
      Id nextId;
      Bearing bearing;
      Bearing relativeBearing; // angle between current way and the straight direction. 0°-180° means to right, 180°-360° is left
      RouteDescription::LaneDescription lanes;
    };
    std::vector<JunctionExit> junctionExits; // excluding incoming and outgoing way
    std::vector<JunctionExit> junctionLeftExits;
    std::vector<JunctionExit> junctionRightExits;
    junctionExits.reserve(node.GetObjects().size());
    junctionLeftExits.reserve(node.GetObjects().size());
    junctionRightExits.reserve(node.GetObjects().size());

    for (const auto &o: node.GetObjects()){
      if (!o.IsWay()) {
        continue; // areas not considered now
      }

      way=postprocessor.GetWay(DBFileOffset(node.GetDatabaseId(), o.GetFileOffset()));
      for (size_t i = 0; i < way->nodes.size(); i++) {
        if (way->nodes[i].GetId() == nodeId) {
          if (i < way->nodes.size()-1 && postprocessor.CanUseForward(dbId, nodeId, o)) {
            Id junctionNodeId = way->nodes[i+1].GetId();
            if (junctionNodeId!=prevNodeId && junctionNodeId!=nextNodeId) {
              auto bearing = GetSphericalBearingInitial(way->nodes[i].GetCoord(), way->nodes[i + 1].GetCoord());
              auto wayLanes = postprocessor.GetLanes(node.GetDatabaseId(), way, true);
              junctionExits.push_back({junctionNodeId, bearing, Bearing(), wayLanes});
            }
          }
          if (i > 0 && postprocessor.CanUseBackward(dbId, nodeId, o)) {
            Id junctionNodeId = way->nodes[i-1].GetId();
            if (junctionNodeId!=prevNodeId && junctionNodeId!=nextNodeId) {
              auto bearing = GetSphericalBearingInitial(way->nodes[i].GetCoord(), way->nodes[i - 1].GetCoord());
              auto wayLanes = postprocessor.GetLanes(node.GetDatabaseId(), way, false);
              junctionExits.push_back({junctionNodeId, bearing, Bearing(), wayLanes});
            }
          }
          break;
        }
      }
    }

    Bearing prevNodeRelativeToNextBearing = prevNodeBearing - nextNodeBearing;
    for (JunctionExit &exit: junctionExits){
      Bearing relativeToNextBearing = exit.bearing - nextNodeBearing; // relative to next
      exit.relativeBearing = exit.bearing - (prevNodeBearing + Bearing::Radians(M_PI)); // relative to straight direction

      if (relativeToNextBearing < prevNodeRelativeToNextBearing) {
        junctionRightExits.push_back(exit);
      } else {
        junctionLeftExits.push_back(exit);
      }
    }

    // sort by bearing, descent
    std::sort(junctionRightExits.begin(), junctionRightExits.end(), [](const JunctionExit &j1, const JunctionExit &j2) -> bool {
      return j1.bearing > j2.bearing;
    });
    std::sort(junctionLeftExits.begin(), junctionLeftExits.end(), [](const JunctionExit &j1, const JunctionExit &j2) -> bool {
      return j1.bearing > j2.bearing;
    });

    int allowedLaneFrom = 0;
    int allowedLaneTo = prevLanes->GetLaneCount()-1; // inclusive
    std::vector<LaneTurn> laneTurns = prevLanes->GetLaneTurns();

    // when number of outgoing lanes match to incoming one, we may decide easily what lane belong to what exit...
    bool laneCntMatch = laneTurns.size() ==
      std::transform_reduce(junctionExits.begin(), junctionExits.end(), size_t(lanes->GetLaneCount()), std::plus{},
                            [](const auto &exit) -> size_t { return exit.lanes.GetLaneCount(); });

    // remove allowed lanes used for left exits
    for (const auto &exit: junctionLeftExits) {
      LaneTurn exitVariant = LaneTurn::Through;
      if (size_t(allowedLaneFrom) < laneTurns.size()) {
        exitVariant=laneTurns[allowedLaneFrom];
      }
      if (exitVariant==LaneTurn::Through and exit.relativeBearing < Bearing::Degrees(-40) and exit.relativeBearing > Bearing::Degrees(-180)) {
        continue; // hack: ignore the exit, relative bearing is too high and lane turn is through, probably not allowed to go there (U-Turn?)
      }
      for (int used=0; used < exit.lanes.GetLaneCount(); used++) {
        LaneTurn turn = LaneTurn::Through;
        if (size_t(allowedLaneFrom) < laneTurns.size()) {
          turn=laneTurns[allowedLaneFrom];
        }
        if (turn==LaneTurn::Through_Left ||
            turn==LaneTurn::Through_SlightLeft ||
            turn==LaneTurn::Through_SharpLeft) {
          // do not consume lane when it allows to use with two directions, just remove left direction
          laneTurns[allowedLaneTo]=LaneTurn::Through;
          continue;
        }
        if ((laneCntMatch || exitVariant==turn) && allowedLaneFrom < allowedLaneTo) {
          allowedLaneFrom++;
        } else {
          break;
        }
      }
    }
    // remove allowed lanes used for right exits
    for (const auto &exit: junctionRightExits) {
      LaneTurn exitVariant = LaneTurn::Through;
      if (size_t(allowedLaneTo) < laneTurns.size()) {
        exitVariant=laneTurns[allowedLaneTo];
      }
      if (exitVariant==LaneTurn::Through and exit.relativeBearing > Bearing::Degrees(40) and exit.relativeBearing < Bearing::Degrees(180)) {
        continue; // hack: ignore the exit, relative bearing is too high and lane turn is through, probably not allowed to go there (U-Turn?)
      }
      for (int used=0; used < exit.lanes.GetLaneCount(); used++) {
        LaneTurn turn = LaneTurn::Through;
        if (size_t(allowedLaneTo) < laneTurns.size()) {
          turn=laneTurns[allowedLaneTo];
        }
        if (turn==LaneTurn::Through_Right ||
            turn==LaneTurn::Through_SlightRight ||
            turn==LaneTurn::Through_SharpRight) {
          // do not consume lane when it allows to use with two directions, just remove right direction
          laneTurns[allowedLaneTo]=LaneTurn::Through;
          continue;
        }
        if ((laneCntMatch || exitVariant==turn) && allowedLaneFrom < allowedLaneTo) {
          allowedLaneTo--;
        } else {
          break;
        }
      }
    }

    // setup suggested lane description to incoming route segment
    assert(allowedLaneTo >= allowedLaneFrom);

    // detect what all allowed turns have common
    // keep in mind that laneTurns may have removed directions used by exits, it is benefitial here
    uint32_t suggestedTurnBits;
    if (allowedLaneFrom < int(laneTurns.size())) {
      suggestedTurnBits = TurnToBits(laneTurns[allowedLaneFrom]);
    } else {
      suggestedTurnBits = TurnToBits(LaneTurn::Unknown);
    }
    for (int i = allowedLaneFrom + 1; i <= allowedLaneTo; i++) {
      if (i < int(laneTurns.size())) {
        suggestedTurnBits &= TurnToBits(laneTurns[i]);
      } else {
        suggestedTurnBits &= TurnToBits(LaneTurn::Unknown);
      }
    }
    // evaluate suggested direction from lane turns
    Bearing relativeBearing = nextNodeBearing - (prevNodeBearing + Bearing::Radians(M_PI)); // relative to straight direction
    LaneTurn suggestedTurn = BitsToTurn(suggestedTurnBits);
    if (suggestedTurn == LaneTurn::Through_SlightRight || suggestedTurn == LaneTurn::Through_Right  || suggestedTurn == LaneTurn::Through_SharpRight) {
      if (relativeBearing > Bearing::Degrees(30) and relativeBearing < Bearing::Degrees(180)) {
        suggestedTurn = BitsToTurn(suggestedTurnBits ^ TurnToBits(LaneTurn::Through));
      } else {
        suggestedTurn = LaneTurn::Through;
      }
    }
    if (suggestedTurn == LaneTurn::Through_SlightLeft || suggestedTurn == LaneTurn::Through_Left  || suggestedTurn == LaneTurn::Through_SharpLeft) {
      if (relativeBearing < Bearing::Degrees(-30) and relativeBearing > Bearing::Degrees(180)) {
        suggestedTurn = BitsToTurn(suggestedTurnBits ^ TurnToBits(LaneTurn::Through));
      } else {
        suggestedTurn = LaneTurn::Through;
      }
    }

    auto suggested = std::make_shared<RouteDescription::SuggestedLaneDescription>(allowedLaneFrom, allowedLaneTo, suggestedTurn);
    for (auto it = backBuffer.rbegin(); it != backBuffer.rend(); it++) {
      auto* nodePtr = *it;
      auto nodeLanes = GetLaneDescription(*nodePtr);
      if (*prevLanes != *nodeLanes){
        break;
      }
      nodePtr->AddDescription(RouteDescription::SUGGESTED_LANES_DESC, suggested);
    }
  }

  RouteDescription::LaneDescriptionRef RoutePostprocessor::SuggestedLanesPostprocessor::GetLaneDescription(const RouteDescription::Node &node) const
  {
    return std::dynamic_pointer_cast<RouteDescription::LaneDescription>(node.GetDescription(RouteDescription::LANES_DESC));
  }

  bool RoutePostprocessor::SuggestedLanesPostprocessor::Process(const PostprocessorContext& postprocessor,
                                                                RouteDescription& description)
  {
    using namespace std::string_view_literals;

    // buffer of traveled nodes, recent node at back
    std::list<RouteDescription::Node*> backBuffer;
    for (auto& node : description.Nodes()) {

      while (!backBuffer.empty() &&
             (node.GetDistance() - backBuffer.front()->GetDistance() > Meters(500))){
        backBuffer.pop_front();
      }
      auto lanes = GetLaneDescription(node);
      if (!lanes){
        // it should not happen, just on last node
        backBuffer.clear();
        continue;
      }

      if (!backBuffer.empty() && !node.GetObjects().empty()) { // we know history and there is some crossing on current node
        auto prevLanes = GetLaneDescription(*backBuffer.back());
        assert(prevLanes);
        if (prevLanes->GetLaneCount() > lanes->GetLaneCount() // lane count was decreased
            || prevLanes->GetLaneTurns() != lanes->GetLaneTurns()) { // lane turns changed

          EvaluateLaneSuggestion(postprocessor, node, backBuffer);

          backBuffer.clear();
        }
      }

      backBuffer.push_back(&node);
    }
    return true;
  }

  bool RoutePostprocessor::ResolveAllPathObjects(const RouteDescription& description,
                                                 DatabaseId dbId,
                                                 Database& database)
  {
    std::set<FileOffset>         areaOffsets;
    std::vector<AreaRef>         areas;

    std::set<FileOffset>         wayOffsets;
    std::vector<WayRef>          ways;

    std::set<FileOffset>         nodeOffsets;
    std::vector<NodeRef>         nodes;

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
          assert(false);
        case refNode:
          nodeOffsets.insert(object.GetFileOffset());
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

    if (!database.GetNodesByOffset(nodeOffsets,nodes)) {
      log.Error() << "Cannot retrieve junction nodes";
      return false;
    }

    nodeOffsets.clear();

    for (const auto& node : nodes) {
      nodeMap[DBFileOffset(dbId,node->GetFileOffset())]=node;
    }

    nodes.clear();

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
    nodeMap.clear();

    motorwayTypes.clear();
    motorwayLinkTypes.clear();

    for (const auto& p:nameReaders){
      delete p.second;
    }
    nameReaders.clear();

    for (const auto& p:refReaders){
      delete p.second;
    }
    refReaders.clear();

    for (const auto& p:bridgeReaders){
      delete p.second;
    }
    bridgeReaders.clear();

    for (const auto& p:roundaboutReaders){
      delete p.second;
    }
    roundaboutReaders.clear();

    for (const auto& p:clockwiseDirectionReaders){
      delete p.second;
    }
    clockwiseDirectionReaders.clear();

    for (const auto& p:destinationReaders){
      delete p.second;
    }
    destinationReaders.clear();

    for (const auto& p:maxSpeedReaders){
      delete p.second;
    }
    maxSpeedReaders.clear();

    for (const auto& p:lanesReaders){
      delete p.second;
    }
    lanesReaders.clear();

    for (const auto& p:accessReaders){
      delete p.second;
    }
    accessReaders.clear();
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

  NodeRef RoutePostprocessor::GetNode(const DBFileOffset &offset) const
  {
    auto entry=nodeMap.find(offset);

    assert(entry!=nodeMap.end());

    return entry->second;
  }

  const LanesFeatureValueReader& RoutePostprocessor::GetLaneReader(const DatabaseId &dbId) const
  {
    auto lanesReader=lanesReaders.find(dbId);
    assert(lanesReader != lanesReaders.end());
    return *(lanesReader->second);
  }

  const AccessFeatureValueReader& RoutePostprocessor::GetAccessReader(const DatabaseId &dbId) const
  {
    auto accessReader=accessReaders.find(dbId);
    assert(accessReader != accessReaders.end());
    return *(accessReader->second);
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

    if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(dbId,object.GetFileOffset()));

      return GetNameDescription(dbId,*way);
    }

    assert(false);

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

  bool RoutePostprocessor::IsMotorwayLink(const RouteDescription::Node& node) const
  {
    auto types=motorwayLinkTypes.find(node.GetDatabaseId());
    assert(types!=motorwayLinkTypes.end());

    if (node.GetPathObject().GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());
      return types->second.IsSet(area->GetType());
    }

    if (node.GetPathObject().GetType()==refWay) {
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

    if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());
      return types->second.IsSet(way->GetType());
    }

    return false;
  }

  bool RoutePostprocessor::IsMiniRoundabout(const RouteDescription::Node& node) const
  {
    const auto &it=miniRoundaboutTypes.find(node.GetDatabaseId());
    if (it==miniRoundaboutTypes.end()) {
      return false;
    }
    TypeInfoRef miniRoundaboutType=it->second;
    for (const auto &obj : node.GetObjects()){
      if (obj.IsNode()) {
        NodeRef n=GetNode(DBFileOffset(node.GetDatabaseId(),obj.GetFileOffset()));
        if (n->GetType()==miniRoundaboutType) {
          return true;
        }
      }
    }
    return false;
  }

  bool RoutePostprocessor::IsRoundabout(const RouteDescription::Node& node) const
  {
    if (node.GetPathObject().GetType()==refArea) {
      return false;
    }

    if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      auto roundaboutReader=roundaboutReaders.find(node.GetDatabaseId());
      assert(roundaboutReader!=roundaboutReaders.end());
      return roundaboutReader->second->IsSet(way->GetFeatureValueBuffer());
    }

    assert(false);

    return false;
  }

  bool RoutePostprocessor::IsClockwise(const RouteDescription::Node& node) const
  {
    for (const auto &obj : node.GetObjects()){
      if (obj.IsNode()) {
        NodeRef n=GetNode(DBFileOffset(node.GetDatabaseId(),obj.GetFileOffset()));
        auto clockwiseDirectionReader = clockwiseDirectionReaders.find(node.GetDatabaseId());
        assert(clockwiseDirectionReader != clockwiseDirectionReaders.end());
        if (clockwiseDirectionReader->second->IsSet(n->GetFeatureValueBuffer())) {
          return true;
        }
      }
    }

    return false;
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

  NodeRef RoutePostprocessor::GetJunctionNode(const RouteDescription::Node& node) const
  {
    const auto &it=junctionTypes.find(node.GetDatabaseId());
    if (it==junctionTypes.end()) {
      return nullptr;
    }
    TypeInfoSet junctionTypeSet=it->second;
    for (const auto &obj : node.GetObjects()) {
      if (obj.IsNode()) {
        NodeRef n = GetNode(DBFileOffset(node.GetDatabaseId(), obj.GetFileOffset()));
        if (junctionTypeSet.IsSet(n->GetType())) {
          return n;
        }
      }
    }
    return nullptr;
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

    if (object.GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      size_t index;

      if (!way->GetNodeIndexByNodeId(nodeId,
                                     index)) {
        assert(false);
      }

      return index;
    }

    assert(false);
    return 0;
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

    if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(dbId,object.GetFileOffset()));

      size_t fromNodeIndex;

      if (!way->GetNodeIndexByNodeId(fromNodeId,
                                     fromNodeIndex)) {
        assert(false);
      }

      return fromNodeIndex>0 &&
             profile->CanUseBackward(*way);
    }

    assert(false);
    return false;
  }

  bool RoutePostprocessor::CanUseForward(const DatabaseId& dbId,
                                         Id fromNodeId,
                                         const ObjectFileRef& object) const
  {
    assert(dbId<profiles.size() && profiles[dbId]);
    auto profile=profiles[dbId];

    if (object.GetType()==refArea) {
      AreaRef area=GetArea(DBFileOffset(dbId,
                                        object.GetFileOffset()));

      return profile->CanUse(*area);
    }

    if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(dbId,
                                     object.GetFileOffset()));

      size_t fromNodeIndex;

      if (!way->GetNodeIndexByNodeId(fromNodeId,
                                     fromNodeIndex)) {
        assert(false);
      }

      return fromNodeIndex!=way->nodes.size()-1 &&
             profile->CanUseForward(*way);
    }

    assert(false);
    return false;
  }

  bool RoutePostprocessor::IsBackwardPath(const ObjectFileRef& object,
                                          size_t fromNodeIndex,
                                          size_t toNodeIndex) const
  {
    if (object.GetType()==refArea) {
      return true;
    }

    if (object.GetType()==refWay) {
      return toNodeIndex<fromNodeIndex;
    }

    assert(false);
    return false;
  }

  bool RoutePostprocessor::IsForwardPath(const ObjectFileRef& object,
                                         size_t fromNodeIndex,
                                         size_t toNodeIndex) const
  {
    if (object.GetType()==refArea) {
      return true;
    }

    if (object.GetType()==refWay) {
      return toNodeIndex>fromNodeIndex;
    }

    assert(false);
    return false;
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

    if (object.GetType()==refWay) {
      WayRef way=GetWay(DBFileOffset(node.GetDatabaseId(),object.GetFileOffset()));

      return way->GetFrontId()==nodeId ||
             way->GetBackId()==nodeId;
    }

    assert(false);
    return false;
  }

  GeoCoord RoutePostprocessor::GetCoordinates(const RouteDescription::Node& node,
                                              size_t nodeIndex) const
  {
    if (node.GetPathObject().GetType()==refArea) {
      AreaRef area=GetArea(node.GetDBFileOffset());

      return area->rings.front().GetCoord(nodeIndex);
    }

    if (node.GetPathObject().GetType()==refWay) {
      WayRef way=GetWay(node.GetDBFileOffset());

      return way->GetCoord(nodeIndex);
    }

    assert(false);
    return GeoCoord();
  }

  std::vector<DatabaseRef> RoutePostprocessor::GetDatabases() const
  {
    return databases;
  }

  bool RoutePostprocessor::PostprocessRouteDescription(RouteDescription& description,
                                                       const std::vector<RoutingProfileRef>& profiles,
                                                       const std::vector<DatabaseRef>& databases,
                                                       const std::list<PostprocessorRef>& processors,
                                                       const std::set<std::string,std::less<>>& motorwayTypeNames,
                                                       const std::set<std::string,std::less<>>& motorwayLinkTypeNames,
                                                       const std::set<std::string,std::less<>>& junctionTypeNames,
                                                       const std::string& miniRoundaboutTypeName)
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
      clockwiseDirectionReaders[dbId]=new ClockwiseDirectionFeatureReader(*typeConfig);
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

      miniRoundaboutTypes[dbId]=typeConfig->GetTypeInfo(miniRoundaboutTypeName);

      // load objects
      if (!ResolveAllPathObjects(description,
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

  bool RoutePostprocessor::SectionsPostprocessor::Process([[maybe_unused]] const PostprocessorContext& postprocessor,
                                                          RouteDescription& description)
  {
    int nbSections = this->sectionLengths.size();
    if (nbSections < 2) {
      return true;
    }

    int sectionCount = 0;
    int nodeCount = 1;
    RouteDescription::Node *previousNode = nullptr;
    for (auto &node : description.Nodes()) {
        if (--nodeCount == 0) {
            
            if (previousNode) {
                RouteDescription::ViaDescriptionRef desc=std::make_shared<RouteDescription::ViaDescription>(sectionCount, sectionLengths[sectionCount - 1]);
                previousNode->AddDescription(RouteDescription::NODE_VIA_DESC, desc);
            }

            previousNode = &node;
            nodeCount = sectionLengths[sectionCount++];
            
            if (sectionCount >= nbSections) {
                break;
            }
        }
    }
      
    if (previousNode) {
        RouteDescription::ViaDescriptionRef desc=std::make_shared<RouteDescription::ViaDescription>(nbSections, sectionLengths[nbSections - 1]);
        previousNode->AddDescription(RouteDescription::NODE_VIA_DESC, desc);
    }
      
    return true;
  }
}
