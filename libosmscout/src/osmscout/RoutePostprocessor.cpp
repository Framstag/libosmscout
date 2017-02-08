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

#include <osmscout/RoutePostprocessor.h>

#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Logger.h>

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
                                                       const RoutingProfile& /*profile*/,
                                                       RouteDescription& description,
                                                       Database& /*database*/)
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
                                                        const RoutingProfile& /*profile*/,
                                                        RouteDescription& description,
                                                        Database& /*database*/)
  {
    if (!description.Nodes().empty()) {
      description.Nodes().back().AddDescription(RouteDescription::NODE_TARGET_DESC,
                                                std::make_shared<RouteDescription::TargetDescription>(targetDescription));
    }

    return true;
  }

  RoutePostprocessor::DistanceAndTimePostprocessor::DistanceAndTimePostprocessor()
  {
    // no code
  }

  bool RoutePostprocessor::DistanceAndTimePostprocessor::Process(const RoutePostprocessor& /*postprocessor*/,
                                                                 const RoutingProfile& profile,
                                                                 RouteDescription& description,
                                                                 Database& database)
  {
    ObjectFileRef prevObject;
    GeoCoord      prevCoord(0.0,0.0);

    ObjectFileRef curObject;
    GeoCoord      curCoord(0.0,0.0);

    AreaRef       area;
    WayRef        way;

    double        distance=0.0;
    double        time=0.0;

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
            if (!database.GetAreaByOffset(curObject.GetFileOffset(),area)) {
              log.Error() << "Error while loading area with offset " << curObject.GetFileOffset();
              return false;
            }
            break;
          case refWay:
            if (!database.GetWayByOffset(curObject.GetFileOffset(),way)) {
              log.Error() << "Error while loading way with offset " << curObject.GetFileOffset();
              return false;
            }
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
          double deltaDistance=GetEllipsoidalDistance(prevCoord,
                                                      curCoord);

          double deltaTime=0.0;

          if (node.GetPathObject().GetType()==refArea) {
            deltaTime=profile.GetTime(*area,
                                      deltaDistance);
          }
          else if (node.GetPathObject().GetType()==refWay) {
            deltaTime=profile.GetTime(*way,
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

  RoutePostprocessor::WayNamePostprocessor::WayNamePostprocessor()
  {
    // no code
  }

  bool RoutePostprocessor::WayNamePostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                         const RoutingProfile& /*profile*/,
                                                         RouteDescription& description,
                                                         Database& /*database*/)
  {
    //
    // Store the name of each way
    //

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
         node!=description.Nodes().end();
         ++node) {
      // The last node does not have a pathWayId set, since we are not going anywhere from the target node!
      if (!node->HasPathObject()) {
        break;
      }

      if (node->GetPathObject().GetType()==refArea) {
        AreaRef                              area=postprocessor.GetArea(node->GetPathObject().GetFileOffset());
        RouteDescription::NameDescriptionRef nameDesc=postprocessor.GetNameDescription(*area);

        node->AddDescription(RouteDescription::WAY_NAME_DESC,
                             nameDesc);
      }
      else if (node->GetPathObject().GetType()==refWay) {
        WayRef                               way=postprocessor.GetWay(node->GetPathObject().GetFileOffset());
        RouteDescription::NameDescriptionRef nameDesc=postprocessor.GetNameDescription(*way);

        if (postprocessor.IsBridge(*way) &&
            node!=description.Nodes().begin()) {
          std::list<RouteDescription::Node>::iterator lastNode=node;

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

  RoutePostprocessor::CrossingWaysPostprocessor::CrossingWaysPostprocessor()
  {
    // no code
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
          postprocessor.IsNodeStartOrEndOfObject(node.GetPathObject(),
                                                 node.GetCurrentNodeIndex(),
                                                 originObject)) {
        continue;
      }

      // Way is target way and starts or ends here so it is not an additional crossing way
      if (targetObject.Valid() &&
          object==targetObject &&
          postprocessor.IsNodeStartOrEndOfObject(node.GetPathObject(),
                                                 node.GetCurrentNodeIndex(),
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

      description->AddDescription(postprocessor.GetNameDescription(object));
    }
  }

  bool RoutePostprocessor::CrossingWaysPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                              const RoutingProfile& profile,
                                                              RouteDescription& description,
                                                              Database& /*database*/)
  {
    //
    // Analyze crossing
    //

    std::list<RouteDescription::Node>::iterator lastJunction=description.Nodes().end();
    std::list<RouteDescription::Node>::iterator lastNode=description.Nodes().end();
    std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();

    while (node!=description.Nodes().end()) {
      // We only analyze junctions, that means nodes that cross other objects
      if (node->GetObjects().empty()) {
        lastNode=node;
        node++;

        continue;
      }

      // We ignore the first and the last node
      if (lastNode==description.Nodes().end() ||
          !node->HasPathObject()) {
        lastNode=node;
        node++;

        continue;
      }

      // Count existing exits, that means, the number of objects that can be used to leave the current
      // node
      size_t exitCount=0;

      Id nodeId=postprocessor.GetNodeId(node->GetPathObject(),
                                        node->GetCurrentNodeIndex());

      size_t currentNodeIndexOnLastPath=postprocessor.GetNodeIndex(lastNode->GetPathObject(),
                                                                   nodeId);

      for (std::vector<ObjectFileRef>::const_iterator object=node->GetObjects().begin();
          object!=node->GetObjects().end();
          ++object) {
        bool canUseForward=postprocessor.CanUseForward(profile,
                                                       nodeId,
                                                       *object);
        bool canUseBackward=postprocessor.CanUseBackward(profile,
                                                         nodeId,
                                                         *object);

        // We can travel this way in the forward direction
        if (canUseForward) {
          // And it is not the way back to the last routing node
          if (lastNode->GetPathObject()!=*object ||
              postprocessor.IsRoundabout(*object) ||
              !postprocessor.IsForwardPath(lastNode->GetPathObject(),
                                           currentNodeIndexOnLastPath,
                                           lastNode->GetCurrentNodeIndex())) {
            exitCount++;
          }
        }

        // We can travel this way in the backward direction
        if (canUseBackward) {
          // And it is not the way to back the last routing node
          if (lastNode->GetPathObject()!=*object ||
              !postprocessor.IsBackwardPath(lastNode->GetPathObject(),
                                            currentNodeIndexOnLastPath,
                                            lastNode->GetCurrentNodeIndex())) {
            exitCount++;
          }
        }
      }

      RouteDescription::CrossingWaysDescriptionRef desc=std::make_shared<RouteDescription::CrossingWaysDescription>(exitCount,
                                                                                                                    postprocessor.GetNameDescription(lastNode->GetPathObject()),
                                                                                                                    postprocessor.GetNameDescription(node->GetPathObject()));
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
  const double RoutePostprocessor::DirectionPostprocessor::curveMaxNodeDistance=0.020;
  const double RoutePostprocessor::DirectionPostprocessor::curveMaxDistance=0.300;
  const double RoutePostprocessor::DirectionPostprocessor::curveMinAngle=5.0;

  RoutePostprocessor::DirectionPostprocessor::DirectionPostprocessor()
  {
    // no code
  }

  bool RoutePostprocessor::DirectionPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                           const RoutingProfile& /*profile*/,
                                                           RouteDescription& description,
                                                           Database& /*database*/)
  {
    std::list<RouteDescription::Node>::const_iterator prevNode=description.Nodes().end();
    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
         node!=description.Nodes().end();
         prevNode=node++) {
      std::list<RouteDescription::Node>::iterator nextNode=node;

      nextNode++;

      if (prevNode!=description.Nodes().end() &&
          nextNode!=description.Nodes().end() &&
          nextNode->HasPathObject()) {
        double prevLat;
        double prevLon;

        double lat;
        double lon;

        double nextLat;
        double nextLon;

        postprocessor.GetCoordinates(prevNode->GetPathObject(),
                                     prevNode->GetCurrentNodeIndex(),
                                     prevLat,prevLon);

        postprocessor.GetCoordinates(node->GetPathObject(),
                                     node->GetCurrentNodeIndex(),
                                     lat,lon);

        postprocessor.GetCoordinates(nextNode->GetPathObject(),
                                     nextNode->GetCurrentNodeIndex(),
                                     nextLat,nextLon);

        double inBearing=GetSphericalBearingFinal(prevLon,prevLat,lon,lat)*180/M_PI;
        double outBearing=GetSphericalBearingInitial(lon,lat,nextLon,nextLat)*180/M_PI;

        double turnAngle=NormalizeRelativeAngel(outBearing-inBearing);

        double curveAngle=turnAngle;

        if (fabs(turnAngle)>=curveMinInitialAngle && fabs(turnAngle)<=curveMaxInitialAngle) {
          std::list<RouteDescription::Node>::iterator curveB=nextNode;
          double                                      currentBearing=outBearing;
          double                                      forwardDistance=nextNode->GetDistance()-node->GetDistance();
          std::list<RouteDescription::Node>::iterator lookup=nextNode;

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

            double curveBLat;
            double curveBLon;
            double lookupLat;
            double lookupLon;

            postprocessor.GetCoordinates(curveB->GetPathObject(),
                                         curveB->GetCurrentNodeIndex(),
                                         curveBLat,curveBLon);

            postprocessor.GetCoordinates(lookup->GetPathObject(),
                                         lookup->GetCurrentNodeIndex(),
                                         lookupLat,lookupLon);

            double lookupBearing=GetSphericalBearingInitial(curveBLon,curveBLat,lookupLon,lookupLat)*180/M_PI;


            double lookupAngle=NormalizeRelativeAngel(lookupBearing-currentBearing);

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
            curveAngle=NormalizeRelativeAngel(currentBearing-inBearing);

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

  RoutePostprocessor::MotorwayJunctionPostprocessor::MotorwayJunctionPostprocessor()
  {
    // no code
  }

  bool RoutePostprocessor::MotorwayJunctionPostprocessor::Process(const RoutePostprocessor& /*postprocessor*/,
                                                                  const RoutingProfile& /*profile*/,
                                                                  RouteDescription& description,
                                                                  Database& database)
  {
     ObjectFileRef           prevObject;
     ObjectFileRef           curObject;
     AreaRef                 area;
     WayRef                  way;
     double                  delta=1E-7;
     std::string             junctionRef;
     std::string             junctionName;
     std::vector<FileOffset> nodeOffsets;
     std::vector<NodeRef>    nodes;
     AreaNodeIndexRef        areaNodeIndex=database.GetAreaNodeIndex();

     if (!areaNodeIndex) {
       return false;
     }

     TypeInfoRef typeInfo=database.GetTypeConfig()->GetTypeInfo("highway_motorway_junction");
     TypeInfoSet nodeTypes;

     if (!typeInfo) {
       return false;
     }

     nodeTypes.Set(typeInfo);

     RefFeatureValueReader  refReader(*database.GetTypeConfig());
     NameFeatureValueReader nameReader(*database.GetTypeConfig());

     for (auto& node: description.Nodes()) {
       junctionName="";
       junctionRef="";

       // The last node does not have a pathWayId set, since we are not going anywhere!
       if (!node.HasPathObject()) {
         continue;
       }

       // Only load the next way, if it is different from the old one
       curObject=node.GetPathObject();

       if (curObject==prevObject) {
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
         if (!database.GetAreaByOffset(curObject.GetFileOffset(),
                                       area)) {
           log.Error()<< "Error while loading area with offset " << curObject.GetFileOffset();
           return false;
         }
         break;
       case refWay:
         if (!database.GetWayByOffset(curObject.GetFileOffset(),
                                      way)) {
           log.Error() << "Error while loading way with offset " << curObject.GetFileOffset();
           return false;
         }

         GeoCoord    coord=way->GetCoord(node.GetCurrentNodeIndex());
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
           continue;
         }

         nodes.clear();
         std::sort(nodeOffsets.begin(),nodeOffsets.end());

         if (!database.GetNodesByOffset(nodeOffsets,
                                        nodes)) {
           log.Error() << "Error reading nodes in area!";
           return false;
         }

         for (size_t i=0; i<nodes.size(); i++) {
           if (fabs(nodes[i]->GetCoords().GetLat() - coord.GetLat()) < delta &&
               fabs(nodes[i]->GetCoords().GetLon() - coord.GetLon()) < delta) {
             RefFeatureValue *refFeatureValue=refReader.GetValue(nodes[i]->GetFeatureValueBuffer());

             if (refFeatureValue!=NULL) {
               junctionRef=refFeatureValue->GetRef();
             }

             NameFeatureValue *nameFeatureValue=nameReader.GetValue(nodes[i]->GetFeatureValueBuffer());

             if (nameFeatureValue!=NULL) {
               junctionName = nameFeatureValue->GetName();
             }

             break;
           }
         }

         break;
       }

       if (junctionName != "" || junctionRef != "") {
         RouteDescription::NameDescriptionRef nameDescription=std::make_shared<RouteDescription::NameDescription>(junctionName,
                                                                                                                  junctionRef);
         node.AddDescription(RouteDescription::MOTORWAY_JUNCTION_DESC,
                             std::make_shared<RouteDescription::MotorwayJunctionDescription>(nameDescription));
       }

       prevObject=curObject;
     }

     return true;
  }

  bool RoutePostprocessor::MaxSpeedPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                          const RoutingProfile& /*profile*/,
                                                          RouteDescription& description,
                                                          Database& database)
  {
    ObjectFileRef              prevObject;
    ObjectFileRef              curObject;
    AreaRef                    area;
    WayRef                     way;
    uint8_t                    speed=0;
    const TypeConfigRef        typeConfig(database.GetTypeConfig());
    MaxSpeedFeatureValueReader maxSpeedReader(*typeConfig);

    for (auto& node : description.Nodes()) {
      // The last node does not have a pathWayId set, since we are not going anywhere!
      if (node.HasPathObject()) {
        // Only load the next way, if it is different from the old one
        curObject=node.GetPathObject();

        if (curObject!=prevObject) {
          switch (node.GetPathObject().GetType()) {
          case refNone:
            assert(false);
            break;
          case refNode:
            assert(false);
            break;
          case refArea:
            area=postprocessor.GetArea(node.GetPathObject().GetFileOffset());
            speed=0;

            break;
          case refWay:
            way=postprocessor.GetWay(node.GetPathObject().GetFileOffset());

            MaxSpeedFeatureValue *maxSpeedValue=maxSpeedReader.GetValue(way->GetFeatureValueBuffer());

            if (maxSpeedValue!=NULL) {
              speed=maxSpeedValue->GetMaxSpeed();
            }
            else {
              speed=0;
            }

            break;
          }
        }

        if (speed!=0) {
          node.AddDescription(RouteDescription::WAY_MAXSPEED_DESC,
                              std::make_shared<RouteDescription::MaxSpeedDescription>(speed));
        }

        prevObject=curObject;
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

    if (postprocessor.IsRoundabout(node.GetPathObject())) {
      return roundabout;
    }

    if (node.GetPathObject().GetType()==refArea) {
      AreaRef area=postprocessor.GetArea(node.GetPathObject().GetFileOffset());

      if (motorwayLinkTypes.IsSet(area->GetType())) {
        return link;
      }
      else if (motorwayTypes.IsSet(area->GetType())) {
        return motorway;
      }
    }
    else if (node.GetPathObject().GetType()==refWay) {
      WayRef way=postprocessor.GetWay(node.GetPathObject().GetFileOffset());

      if (motorwayLinkTypes.IsSet(way->GetType())) {
        return link;
      }
      else if (motorwayTypes.IsSet(way->GetType())) {
        return motorway;
      }
    }
    else {
      return street;
    }

    return street;
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutEnter(RouteDescription::Node& node)
  {
    RouteDescription::RoundaboutEnterDescriptionRef desc=std::make_shared<RouteDescription::RoundaboutEnterDescription>();

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
    RouteDescription::RoundaboutLeaveDescriptionRef desc=std::make_shared<RouteDescription::RoundaboutLeaveDescription>(roundaboutCrossingCounter);

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

    lastNode--;

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

  void RoutePostprocessor::InstructionPostprocessor::AddMotorwayType(const TypeInfoRef& type)
  {
    motorwayTypes.Set(type);
  }

  void RoutePostprocessor::InstructionPostprocessor::AddMotorwayLinkType(const TypeInfoRef& type)
  {
    motorwayLinkTypes.Set(type);
  }

  bool RoutePostprocessor::InstructionPostprocessor::Process(const RoutePostprocessor& postprocessor,
                                                             const RoutingProfile& /*profile*/,
                                                             RouteDescription& description,
                                                             Database& /*database*/)
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

      if (!postprocessor.IsRoundabout(lastNode->GetPathObject()) &&
          postprocessor.IsRoundabout(node->GetPathObject())) {
        inRoundabout=true;
        roundaboutCrossingCounter=0;

        HandleRoundaboutEnter(*node);

        lastNode=node++;

        continue;
      }

      if (postprocessor.IsRoundabout(lastNode->GetPathObject()) &&
          !postprocessor.IsRoundabout(node->GetPathObject())) {
        HandleRoundaboutNode(*node);

        HandleRoundaboutLeave(*node);

        inRoundabout=false;

        lastNode=node++;

        continue;
      }

      // Non-Link, non-motorway to motorway (enter motorway))
      if (!postprocessor.IsOfType(lastNode->GetPathObject(),
                                  motorwayLinkTypes) &&
          !postprocessor.IsOfType(lastNode->GetPathObject(),
                                  motorwayTypes) &&
          postprocessor.IsOfType(node->GetPathObject(),
                                 motorwayTypes)) {
        HandleDirectMotorwayEnter(*node,
                                  targetName);

        lastNode=node++;

        continue;
      }

      // motorway to Non-Link, non-motorway /leave motorway)
      if (postprocessor.IsOfType(lastNode->GetPathObject(),
                                 motorwayTypes) &&
          !postprocessor.IsOfType(node->GetPathObject(),
                                  motorwayLinkTypes) &&
          !postprocessor.IsOfType(node->GetPathObject(),
                                  motorwayTypes)) {
        HandleDirectMotorwayLeave(*node,
                                  originName);

        lastNode=node++;

        continue;
      }

      else if (!postprocessor.IsOfType(lastNode->GetPathObject(),
                                       motorwayLinkTypes) &&
          postprocessor.IsOfType(node->GetPathObject(),
                                 motorwayLinkTypes)) {
        bool                                        originIsMotorway=postprocessor.IsOfType(lastNode->GetPathObject(),
                                                                                            motorwayTypes);
        bool                                        targetIsMotorway=false;
        std::list<RouteDescription::Node>::iterator next=node;
        RouteDescription::NameDescriptionRef        nextName;

        next++;
        while (next!=description.Nodes().end() &&
               next->HasPathObject()) {

          nextName=std::dynamic_pointer_cast<RouteDescription::NameDescription>(next->GetDescription(RouteDescription::WAY_NAME_DESC));

          if (!postprocessor.IsOfType(next->GetPathObject(),
                                      motorwayLinkTypes)) {
            break;
          }

          next++;
        }

        if (next->GetPathObject().Valid()) {
          targetIsMotorway=postprocessor.IsOfType(next->GetPathObject(),
                                                  motorwayTypes);
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

  RoutePostprocessor::RoutePostprocessor()
  : nameReader(NULL),
    refReader(NULL),
    bridgeReader(NULL),
    roundaboutReader(NULL)
  {

  }

  bool RoutePostprocessor::ResolveAllAreasAndWays(const RouteDescription& description,
                                                  Database& database)
  {
    std::set<FileOffset>         areaOffsets;
    std::vector<AreaRef>         areas;

    std::set<FileOffset>         wayOffsets;
    std::vector<WayRef>          ways;

    for (const auto &node : description.Nodes()) {
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
      wayMap[way->GetFileOffset()]=way;
    }

    ways.clear();

    if (!database.GetAreasByOffset(areaOffsets,areas)) {
      log.Error() << "Cannot retrieve crossing areas";
      return false;
    }

    areaOffsets.clear();

    for (const auto& area : areas) {
      areaMap[area->GetFileOffset()]=area;
    }

    areas.clear();

    return true;
  }

  void RoutePostprocessor::Cleanup()
  {
    areaMap.clear();
    wayMap.clear();


    delete nameReader;
    nameReader=NULL;

    delete refReader;
    refReader=NULL;

    delete bridgeReader;
    bridgeReader=NULL;

    delete roundaboutReader;
    roundaboutReader=NULL;
  }

  AreaRef RoutePostprocessor::GetArea(FileOffset offset) const
  {
    auto entry=areaMap.find(offset);

    assert(entry!=areaMap.end());

    return entry->second;
  }

  WayRef RoutePostprocessor::GetWay(FileOffset offset) const
  {
    auto entry=wayMap.find(offset);

    assert(entry!=wayMap.end());

    return entry->second;
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::GetNameDescription(const ObjectFileRef& object) const
  {
    RouteDescription::NameDescriptionRef description;

    if (object.GetType()==refArea) {
      AreaRef area=GetArea(object.GetFileOffset());

      return GetNameDescription(*area);
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(object.GetFileOffset());

      return GetNameDescription(*way);
    }
    else {
      assert(false);
    }

    return description;
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::GetNameDescription(const Area& area) const
  {
    NameFeatureValue *nameValue=nameReader->GetValue(area.rings.front().GetFeatureValueBuffer());
    std::string      name;

    if (nameValue!=NULL) {
      name=nameValue->GetName();
    }

    return std::make_shared<RouteDescription::NameDescription>(name);
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::GetNameDescription(const Way& way) const
  {
    NameFeatureValue *nameValue=nameReader->GetValue(way.GetFeatureValueBuffer());
    RefFeatureValue  *refValue=refReader->GetValue(way.GetFeatureValueBuffer());
    std::string      name;
    std::string      ref;

    if (nameValue!=NULL) {
      name=nameValue->GetName();
    }

    if (refValue!=NULL) {
      ref=refValue->GetRef();
    }

    return std::make_shared<RouteDescription::NameDescription>(name,
                                                               ref);
  }

  bool RoutePostprocessor::IsRoundabout(const ObjectFileRef& object) const
  {
    if (object.GetType()==refArea) {
      return false;
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(object.GetFileOffset());

      return roundaboutReader->IsSet(way->GetFeatureValueBuffer());
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::IsBridge(const Way& way) const
  {
    return bridgeReader->IsSet(way.GetFeatureValueBuffer());
  }

  bool RoutePostprocessor::IsOfType(const ObjectFileRef& object,
                                    const TypeInfoSet& types) const
  {
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(object.GetFileOffset());

      return types.IsSet(area->GetType());
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(object.GetFileOffset());

      return types.IsSet(way->GetType());
    }
    else {
      assert(false);

      return false;
    }
  }

  Id RoutePostprocessor::GetNodeId(const ObjectFileRef& object,
                                   size_t nodeIndex) const
  {
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(object.GetFileOffset());

      return area->rings.front().nodes[nodeIndex].GetId();
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(object.GetFileOffset());

      return way->GetId(nodeIndex);
    }
    else {
      assert(false);

      return 0;
    }
  }

  size_t RoutePostprocessor::GetNodeIndex(const ObjectFileRef& object,
                                          Id nodeId) const
  {
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(object.GetFileOffset());

      for (size_t i=0; i<area->rings.front().nodes.size(); i++) {
        if (area->rings.front().nodes[i].GetId()==nodeId) {
          return i;
        }

      }

      assert(false);

      return 0;
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(object.GetFileOffset());

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

  bool RoutePostprocessor::CanUseBackward(const RoutingProfile& profile,
                                          Id fromNodeId,
                                          const ObjectFileRef& object) const
  {
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(object.GetFileOffset());

      return profile.CanUse(*area);
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(object.GetFileOffset());

      size_t fromNodeIndex;

      if (!way->GetNodeIndexByNodeId(fromNodeId,
                                     fromNodeIndex)) {
        assert(false);
      }

      return fromNodeIndex>0 &&
             profile.CanUseBackward(*way);
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::CanUseForward(const RoutingProfile& profile,
                                         Id fromNodeId,
                                         const ObjectFileRef& object) const
  {
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(object.GetFileOffset());

      return profile.CanUse(*area);
    }
    else if (object.GetType()==refWay) {
      WayRef way=GetWay(object.GetFileOffset());

      size_t fromNodeIndex;

      if (!way->GetNodeIndexByNodeId(fromNodeId,
                                     fromNodeIndex)) {
        assert(false);
      }

      return fromNodeIndex!=way->nodes.size()-1 &&
             profile.CanUseForward(*way);
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

  bool RoutePostprocessor::IsNodeStartOrEndOfObject(const ObjectFileRef& nodeObject,
                                                    size_t nodeIndex,
                                                    const ObjectFileRef& object) const
  {
    Id nodeId;

    if (nodeObject.GetType()==refArea) {
      AreaRef area=GetArea(nodeObject.GetFileOffset());

      nodeId=area->rings.front().GetId(nodeIndex);
    }
    else if (nodeObject.GetType()==refWay) {
      WayRef way=GetWay(nodeObject.GetFileOffset());

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
      WayRef way=GetWay(object.GetFileOffset());

      return way->GetFrontId()==nodeId ||
             way->GetBackId()==nodeId;
    }
    else {
      assert(false);

      return false;
    }
  }

  void RoutePostprocessor::GetCoordinates(const ObjectFileRef& object,
                                          size_t nodeIndex,
                                          double& lat,
                                          double& lon) const
  {
    if (object.GetType()==refArea) {
      AreaRef area=GetArea(object.GetFileOffset());

      GeoCoord coord=area->rings.front().GetCoord(nodeIndex);

      lat=coord.GetLat();
      lon=coord.GetLon();
    }
    else if (object.GetType()==refWay) {
      WayRef   way=GetWay(object.GetFileOffset());
      GeoCoord coord=way->GetCoord(nodeIndex);

      lat=coord.GetLat();
      lon=coord.GetLon();
    }
    else {
      assert(false);
    }
  }

  bool RoutePostprocessor::PostprocessRouteDescription(RouteDescription& description,
                                                       const RoutingProfile& profile,
                                                       Database& database,
                                                       std::list<PostprocessorRef> processors)
  {
    Cleanup(); // We do not trust ourself ;-)

    nameReader=new NameFeatureValueReader(*database.GetTypeConfig());
    refReader=new RefFeatureValueReader(*database.GetTypeConfig());
    bridgeReader=new BridgeFeatureReader(*database.GetTypeConfig());
    roundaboutReader=new RoundaboutFeatureReader(*database.GetTypeConfig());

    if (!ResolveAllAreasAndWays(description,
                                database)) {
      Cleanup();

      return false;
    }

    size_t pos=1;
    for (const auto& processor : processors) {
      if (!processor->Process(*this,
                              profile,
                              description,
                              database)) {
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

