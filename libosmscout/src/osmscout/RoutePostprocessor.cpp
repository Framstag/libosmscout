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

#include <iostream>

#include <osmscout/util/Geometry.h>

#include <osmscout/private/Math.h>

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

  bool RoutePostprocessor::StartPostprocessor::Process(const RoutingProfile& profile,
                                                       RouteDescription& description,
                                                       Database& database)
  {
    description.Nodes().front().AddDescription(RouteDescription::NODE_START_DESC,
                                               new RouteDescription::StartDescription(startDescription));

    return true;
  }

  RoutePostprocessor::TargetPostprocessor::TargetPostprocessor(const std::string& targetDescription)
  : targetDescription(targetDescription)
  {
    // no code
  }

  bool RoutePostprocessor::TargetPostprocessor::Process(const RoutingProfile& profile,
                                                        RouteDescription& description,
                                                        Database& database)
  {
    description.Nodes().back().AddDescription(RouteDescription::NODE_TARGET_DESC,
                                              new RouteDescription::TargetDescription(targetDescription));

    return true;
  }

  bool RoutePostprocessor::DistanceAndTimePostprocessor::Process(const RoutingProfile& profile,
                                                                 RouteDescription& description,
                                                                 Database& database)
  {
    WayRef prevWay;
    WayRef nextWay;
    Id     prevNode=0;
    Id     nextNode=0;
    double distance=0.0;
    double time=0.0;

    for (std::list<RouteDescription::Node>::iterator iter=description.Nodes().begin();
         iter!=description.Nodes().end();
         ++iter) {
      // The last node does not have a pathWayId set, since we are not going anywhere!
      if (iter->HasPathWay()) {
        // Only load the next way, if it is different from the old one
        if (prevWay.Invalid() || prevWay->GetId()!=iter->GetPathWayId()) {
          if (!database.GetWay(iter->GetPathWayId(),nextWay)) {
            std::cout << "Error while loading way " << iter->GetPathWayId() << std::endl;
            return false;
          }
        }
        else {
          nextWay=prevWay;
        }

        for (size_t i=0; i<nextWay->nodes.size(); i++) {
          if (nextWay->nodes[i].GetId()==iter->GetCurrentNodeId()) {
            nextNode=i;
            break;
          }
        }

        if (prevWay.Valid()) {
          double deltaDistance=GetEllipsoidalDistance(prevWay->nodes[prevNode].GetLon(),
                                                      prevWay->nodes[prevNode].GetLat(),
                                                      nextWay->nodes[nextNode].GetLon(),
                                                      nextWay->nodes[nextNode].GetLat());
          double deltaTime=profile.GetTime(nextWay,
                                           deltaDistance);

          distance+=deltaDistance;
          time+=deltaTime;
        }
      }

      iter->SetDistance(distance);
      iter->SetTime(time);

      prevWay=nextWay;
      prevNode=nextNode;
    }

    return true;
  }

  bool RoutePostprocessor::WayNamePostprocessor::Process(const RoutingProfile& profile,
                                                         RouteDescription& description,
                                                         Database& database)
  {
    //
    // Load all ways in one go and put them into a map
    //

    std::set<Id>        wayIds;
    std::vector<WayRef> ways;
    std::map<Id,WayRef> wayMap;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      if (node->HasPathWay()) {
        wayIds.insert(node->GetPathWayId());
      }

      for (std::vector<Path>::const_iterator path=node->GetPaths().begin();
          path!=node->GetPaths().end();
          ++path) {
        wayIds.insert(path->GetWayId());
      }
    }

    if (!database.GetWays(wayIds,ways)) {
      std::cerr << "Cannot retrieve crossing ways" << std::endl;
      return false;
    }

    for (std::vector<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        ++w) {
      WayRef way=*w;

      wayMap[way->GetId()]=way;
    }

    //
    // Store the name of each way
    //

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      // The last node does not have a pathWayId set, since we are not going anywhere from the target node!
      if (!node->HasPathWay()) {
        break;
      }

      WayRef                               way=wayMap[node->GetPathWayId()];
      RouteDescription::NameDescriptionRef nameDesc=new RouteDescription::NameDescription(way->GetName(),
                                                                                          way->GetRefName());

      if (way->IsBridge() &&
          node!=description.Nodes().begin()) {
        std::list<RouteDescription::Node>::iterator lastNode=node;

        lastNode--;

        RouteDescription::NameDescriptionRef lastDesc=dynamic_cast<RouteDescription::NameDescription*>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));


        if (lastDesc.Valid() &&
            lastDesc->GetRef()==nameDesc->GetRef() &&
            lastDesc->GetName()!=nameDesc->GetName()) {
          nameDesc=lastDesc;
        }
      }

      node->AddDescription(RouteDescription::WAY_NAME_DESC,
                           nameDesc);
    }

    return true;
  }

  void RoutePostprocessor::CrossingWaysPostprocessor::AddCrossingWaysDescriptions(RouteDescription::CrossingWaysDescription* description,
                                                                                  const RouteDescription::Node& node,
                                                                                  const WayRef& originWay,
                                                                                  const WayRef& targetWay,
                                                                                  const std::map<Id,WayRef>& wayMap)
  {
    for (std::vector<Path>::const_iterator path=node.GetPaths().begin();
        path!=node.GetPaths().end();
        ++path) {
      std::map<Id,WayRef>::const_iterator way=wayMap.find(path->GetWayId());

      if (way!=wayMap.end()) {
        // Way is origin way and starts or end here so it is not an additional crossing way
        if (originWay.Valid() &&
            way->second->GetId()==originWay->GetId() &&
            (way->second->nodes.front().GetId()==node.GetCurrentNodeId() ||
             way->second->nodes.back().GetId()==node.GetCurrentNodeId())) {
          continue;
        }

        // Way is target way and starts or end here so it is not an additional crossing way
        if (targetWay.Valid() &&
            way->second->GetId()==targetWay->GetId() &&
            (way->second->nodes.front().GetId()==node.GetCurrentNodeId() ||
             way->second->nodes.back().GetId()==node.GetCurrentNodeId())) {
          continue;
        }

        // ways is origin way and way is target way so it is not an additional crossing way
        if (originWay.Valid() &&
            targetWay.Valid() &&
            way->second->GetId()==originWay->GetId() &&
            way->second->GetId()==targetWay->GetId()) {
          continue;
        }

        description->AddDescription(new RouteDescription::NameDescription(way->second->GetName(),
                                                                          way->second->GetRefName()));
      }
    }
  }

  bool RoutePostprocessor::CrossingWaysPostprocessor::Process(const RoutingProfile& profile,
                                                              RouteDescription& description,
                                                              Database& database)
  {
    //
    // Load all ways in one go and put them into a map
    //

    std::set<Id>        wayIds;
    std::vector<WayRef> ways;
    std::map<Id,WayRef> wayMap;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      if (node->HasPathWay()) {
        wayIds.insert(node->GetPathWayId());
      }

      for (std::vector<Path>::const_iterator path=node->GetPaths().begin();
          path!=node->GetPaths().end();
          ++path) {
        wayIds.insert(path->GetWayId());
      }
    }

    if (!database.GetWays(wayIds,ways)) {
      std::cerr << "Cannot retrieve crossing ways" << std::endl;
      return false;
    }

    for (std::vector<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        ++w) {
      WayRef way=*w;

      wayMap[way->GetId()]=way;
    }

    //
    // Analyze crossing
    //

    std::list<RouteDescription::Node>::iterator lastCrossing=description.Nodes().end();
    std::list<RouteDescription::Node>::iterator lastNode=description.Nodes().end();
    std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
    while (node!=description.Nodes().end()) {
      if (node->GetPaths().empty()) {
        lastNode=node;
        node++;

        continue;
      }

      if (lastNode==description.Nodes().end() ||
          !node->HasPathWay()) {
        lastNode=node;
        node++;

        continue;
      }

      WayRef originWay=wayMap[lastNode->GetPathWayId()];
      WayRef targetWay=wayMap[node->GetPathWayId()];

      // Count existing exits
      size_t exitCount=0;

      for (size_t i=0; i<node->GetPaths().size(); i++) {
        // Leave out the path back to the last crossing (if it exists)
        if (lastCrossing!=description.Nodes().end() &&
            node->GetPaths()[i].GetTargetNodeId()==lastCrossing->GetCurrentNodeId() &&
            node->GetPaths()[i].GetWayId()==lastCrossing->GetPathWayId()) {
          continue;
        }

        if (!node->GetPaths()[i].IsTraversable()) {
          continue;
        }

        exitCount++;
      }

      RouteDescription::CrossingWaysDescriptionRef desc=new RouteDescription::CrossingWaysDescription(exitCount,
                                                                                                      new RouteDescription::NameDescription(originWay->GetName(),
                                                                                                                                            originWay->GetRefName()),
                                                                                                      new RouteDescription::NameDescription(targetWay->GetName(),
                                                                                                                                            targetWay->GetRefName()));

      AddCrossingWaysDescriptions(desc,
                                  *node,
                                  originWay,
                                  targetWay,
                                  wayMap);

      node->AddDescription(RouteDescription::CROSSING_WAYS_DESC,
                           desc);

      lastNode=node;
      lastCrossing=node;

      node++;
    }

    return true;
  }

  const double RoutePostprocessor::DirectionPostprocessor::curveMinInitialAngle=5.0;
  const double RoutePostprocessor::DirectionPostprocessor::curveMaxInitialAngle=10.0;
  const double RoutePostprocessor::DirectionPostprocessor::curveMaxNodeDistance=0.020;
  const double RoutePostprocessor::DirectionPostprocessor::curveMaxDistance=0.300;
  const double RoutePostprocessor::DirectionPostprocessor::curveMinAngle=5.0;

  bool RoutePostprocessor::DirectionPostprocessor::Process(const RoutingProfile& profile,
                                                           RouteDescription& description,
                                                           Database& database)
  {
    //
    // Load all ways in one go and put them into a map
    //

    std::set<Id>        wayIds;
    std::vector<WayRef> ways;
    std::map<Id,WayRef> wayMap;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      if (node->HasPathWay()) {
        wayIds.insert(node->GetPathWayId());
      }
    }

    if (!database.GetWays(wayIds,ways)) {
      std::cerr << "Cannot retrieve crossing ways" << std::endl;
      return false;
    }

    for (std::vector<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        ++w) {
      WayRef way=*w;

      wayMap[way->GetId()]=way;
    }


    std::list<RouteDescription::Node>::const_iterator prevNode=description.Nodes().end();
    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
         node!=description.Nodes().end();
         prevNode=node++) {
      std::list<RouteDescription::Node>::iterator nextNode=node;

      nextNode++;

      if (prevNode!=description.Nodes().end() &&
          nextNode!=description.Nodes().end() &&
          nextNode->HasPathWay()) {
        WayRef prevWay=wayMap[prevNode->GetPathWayId()];
        double prevLat=0.0;
        double prevLon=0.0;

        WayRef way=wayMap[node->GetPathWayId()];
        double lat=0.0;
        double lon=0.0;

        WayRef nextWay=wayMap[nextNode->GetPathWayId()];
        double nextLat=0.0;
        double nextLon=0.0;

        prevWay->GetCoordinates(prevNode->GetCurrentNodeId(),prevLat,prevLon);
        way->GetCoordinates(node->GetCurrentNodeId(),lat,lon);
        nextWay->GetCoordinates(nextNode->GetCurrentNodeId(),nextLat,nextLon);

        double inBearing=GetSphericalBearingFinal(prevLon,prevLat,lon,lat)*180/M_PI;
        double outBearing=GetSphericalBearingInitial(lon,lat,nextLon,nextLat)*180/M_PI;

        double turnAngle=NormalizeRelativeAngel(outBearing-inBearing);

        double curveAngle=turnAngle;

        if (abs(turnAngle)>=curveMinInitialAngle && abs(turnAngle)<=curveMaxInitialAngle) {
          std::list<RouteDescription::Node>::iterator curveB=nextNode;
          double                                      currentBearing=outBearing;
          double                                      forwardDistance=nextNode->GetDistance()-node->GetDistance();
          std::list<RouteDescription::Node>::iterator lookup=nextNode;

          lookup++;
          while (true) {
            // Next node does not exists or does not have a path?
            if (lookup==description.Nodes().end() ||
                !lookup->HasPathWay()) {
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

            WayRef wayB=wayMap[curveB->GetPathWayId()];
            WayRef wayLookup=wayMap[lookup->GetPathWayId()];
            double curveBLat;
            double curveBLon;
            double lookupLat;
            double lookupLon;

            wayB->GetCoordinates(curveB->GetCurrentNodeId(),curveBLat,curveBLon);
            wayLookup->GetCoordinates(lookup->GetCurrentNodeId(),lookupLat,lookupLon);

            double lookupBearing=GetSphericalBearingInitial(curveBLon,curveBLat,lookupLon,lookupLat)*180/M_PI;


            double lookupAngle=NormalizeRelativeAngel(lookupBearing-currentBearing);

            // The next node does not have enough direction change to be still part of a turn?
            if (abs(lookupAngle)<curveMinAngle) {
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
                             new RouteDescription::DirectionDescription(turnAngle,curveAngle));
      }
    }

    return true;
  }

  RoutePostprocessor::InstructionPostprocessor::State RoutePostprocessor::InstructionPostprocessor::GetInitialState(RouteDescription::Node& node,
                                                                                                                    std::map<Id,WayRef>& wayMap)
  {
    if (!node.HasPathWay()) {
      return street;
    }

    const WayRef way=wayMap[node.GetPathWayId()];

    if (way->IsRoundabout()) {
      return roundabout;
    }
    else if (motorwayLinkTypes.find(way->GetType())!=motorwayLinkTypes.end()) {
      return link;
    }
    else if (motorwayTypes.find(way->GetType())!=motorwayTypes.end()) {
      return motorway;
    }

    return street;
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutEnter(RouteDescription::Node& node)
  {
    RouteDescription::RoundaboutEnterDescriptionRef desc=new RouteDescription::RoundaboutEnterDescription();

    node.AddDescription(RouteDescription::ROUNDABOUT_ENTER_DESC,
                        desc);
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutNode(RouteDescription::Node& node)
  {
    if (node.HasDescription(RouteDescription::CROSSING_WAYS_DESC)) {
      RouteDescription::CrossingWaysDescriptionRef crossing=dynamic_cast<RouteDescription::CrossingWaysDescription*>(node.GetDescription(RouteDescription::CROSSING_WAYS_DESC));

      std::cout << "Exits at node: " << node.GetCurrentNodeId() << " " << crossing->GetExitCount() << std::endl;

      if (crossing->GetExitCount()>1) {
        roundaboutCrossingCounter+=crossing->GetExitCount()-1;
      }
    }
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutLeave(RouteDescription::Node& node)
  {
    RouteDescription::RoundaboutLeaveDescriptionRef desc=new RouteDescription::RoundaboutLeaveDescription(roundaboutCrossingCounter);

    node.AddDescription(RouteDescription::ROUNDABOUT_LEAVE_DESC,
                        desc);
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleDirectMotorwayEnter(RouteDescription::Node& node,
                                                                               const RouteDescription::NameDescriptionRef& toName)
  {
    RouteDescription::MotorwayEnterDescriptionRef desc=new RouteDescription::MotorwayEnterDescription(toName);

    node.AddDescription(RouteDescription::MOTORWAY_ENTER_DESC,
                        desc);
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleDirectMotorwayLeave(RouteDescription::Node& node,
                                                                               const RouteDescription::NameDescriptionRef& fromName)
  {
    RouteDescription::MotorwayLeaveDescriptionRef desc=new RouteDescription::MotorwayLeaveDescription(fromName);

    node.AddDescription(RouteDescription::MOTORWAY_LEAVE_DESC,
                        desc);
  }

  bool RoutePostprocessor::InstructionPostprocessor::HandleNameChange(const std::list<RouteDescription::Node>& path,
                                                                      std::list<RouteDescription::Node>::const_iterator& lastNode,
                                                                      std::list<RouteDescription::Node>::iterator& node,
                                                                      const std::map<Id,WayRef>& wayMap)
  {
    RouteDescription::NameDescriptionRef nextName;
    RouteDescription::NameDescriptionRef lastName;

    if (lastNode==path.end()) {
      return false;
    }

    lastName=dynamic_cast<RouteDescription::NameDescription*>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));
    nextName=dynamic_cast<RouteDescription::NameDescription*>(node->GetDescription(RouteDescription::WAY_NAME_DESC));

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
                         new RouteDescription::NameChangedDescription(lastName,
                                                                      nextName));

    return true;
  }

  bool RoutePostprocessor::InstructionPostprocessor::HandleDirectionChange(const std::list<RouteDescription::Node>& path,
                                                                           std::list<RouteDescription::Node>::iterator& node,
                                                                           const std::map<Id,WayRef>& wayMap)
  {
    if (node->GetPaths().size()<=1){
      return false;
    }

    std::list<RouteDescription::Node>::const_iterator lastNode=node;
    RouteDescription::NameDescriptionRef              nextName;
    RouteDescription::NameDescriptionRef              lastName;

    lastNode--;

    if (lastNode==path.end()) {
      return false;
    }

    lastName=dynamic_cast<RouteDescription::NameDescription*>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));
    nextName=dynamic_cast<RouteDescription::NameDescription*>(node->GetDescription(RouteDescription::WAY_NAME_DESC));

    RouteDescription::DescriptionRef          desc=node->GetDescription(RouteDescription::DIRECTION_DESC);
    RouteDescription::DirectionDescriptionRef directionDesc=dynamic_cast<RouteDescription::DirectionDescription*>(desc.Get());

    if (lastName.Valid() &&
        nextName.Valid() &&
        directionDesc.Valid() &&
        lastName->GetName()==nextName->GetName() &&
        lastName->GetRef()==nextName->GetRef()) {
      if (directionDesc->GetCurve()!=RouteDescription::DirectionDescription::slightlyLeft &&
          directionDesc->GetCurve()!=RouteDescription::DirectionDescription::straightOn &&
          directionDesc->GetCurve()!=RouteDescription::DirectionDescription::slightlyRight) {

          node->AddDescription(RouteDescription::TURN_DESC,
                               new RouteDescription::TurnDescription());

          return true;
      }
    }
    else if (directionDesc.Valid() &&
        directionDesc->GetCurve()!=RouteDescription::DirectionDescription::straightOn) {

      node->AddDescription(RouteDescription::TURN_DESC,
                           new RouteDescription::TurnDescription());

      return true;
    }

    return false;
  }

  void RoutePostprocessor::InstructionPostprocessor::AddMotorwayType(TypeId type)
  {
    motorwayTypes.insert(type);
  }

  void RoutePostprocessor::InstructionPostprocessor::AddMotorwayLinkType(TypeId type)
  {
    motorwayLinkTypes.insert(type);
  }

  bool RoutePostprocessor::InstructionPostprocessor::Process(const RoutingProfile& profile,
                                                             RouteDescription& description,
                                                             Database& database)
  {
    //
    // Load all ways in one go and put them into a map
    //

    std::set<Id>        wayIds;
    std::vector<WayRef> ways;
    std::map<Id,WayRef> wayMap;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      if (node->HasPathWay()) {
        wayIds.insert(node->GetPathWayId());
      }

      for (std::vector<Path>::const_iterator path=node->GetPaths().begin();
          path!=node->GetPaths().end();
          ++path) {
        wayIds.insert(path->GetWayId());
      }
    }

    if (!database.GetWays(wayIds,ways)) {
      std::cerr << "Cannot retrieve crossing ways" << std::endl;
      return false;
    }

    for (std::vector<WayRef>::const_iterator w=ways.begin();
        w!=ways.end();
        ++w) {
      WayRef way=*w;

      wayMap[way->GetId()]=way;
    }

    //
    // Detect initial state
    //

    State  state=GetInitialState(description.Nodes().front(),wayMap);

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

      WayRef                               originWay;
      WayRef                               targetWay;
      RouteDescription::NameDescriptionRef originName;
      RouteDescription::NameDescriptionRef targetName;

      if (lastNode!=description.Nodes().end()) {
        originName=dynamic_cast<RouteDescription::NameDescription*>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));

        originWay=wayMap[lastNode->GetPathWayId()];
      }

      if (node->HasPathWay()) {
        targetName=dynamic_cast<RouteDescription::NameDescription*>(node->GetDescription(RouteDescription::WAY_NAME_DESC));

        targetWay=wayMap[node->GetPathWayId()];
      }

      // First or last node
      if (originWay.Invalid() || targetWay.Invalid()) {
        lastNode=node++;
        continue;
      }

      if (!originWay->IsRoundabout() &&
          targetWay->IsRoundabout()) {
        HandleRoundaboutEnter(*node);
        inRoundabout=true;
        roundaboutCrossingCounter=0;
        lastNode=node++;
        continue;
      }

      if (originWay->IsRoundabout() &&
               !targetWay->IsRoundabout()) {
        HandleRoundaboutNode(*node);
        HandleRoundaboutLeave(*node);
        inRoundabout=false;

        lastNode=node++;
        continue;
      }

      // Non-Link, non-motorway to motorway
      if (motorwayLinkTypes.find(originWay->GetType())==motorwayLinkTypes.end() &&
          motorwayTypes.find(originWay->GetType())==motorwayTypes.end() &&
          motorwayTypes.find(targetWay->GetType())!=motorwayTypes.end()) {
        HandleDirectMotorwayEnter(*node,
                                  targetName);

        lastNode=node++;
        continue;
      }

      // Motorway to Non-Link, non-motorway
      if (motorwayTypes.find(originWay->GetType())!=motorwayTypes.end() &&
          motorwayLinkTypes.find(targetWay->GetType())==motorwayLinkTypes.end() &&
          motorwayTypes.find(targetWay->GetType())==motorwayTypes.end()) {
        HandleDirectMotorwayLeave(*node,
                                  originName);

        lastNode=node++;
        continue;
      }

      else if (motorwayLinkTypes.find(originWay->GetType())==motorwayLinkTypes.end() &&
               motorwayLinkTypes.find(targetWay->GetType())!=motorwayLinkTypes.end()) {
        bool                                        originIsMotorway=motorwayTypes.find(originWay->GetType())!=motorwayTypes.end();
        bool                                        targetIsMotorway=false;
        std::list<RouteDescription::Node>::iterator next=node;
        WayRef                                      nextWay;
        RouteDescription::NameDescriptionRef        nextName;

        next++;
        while (next!=description.Nodes().end() &&
               next->HasPathWay()) {

          nextName=dynamic_cast<RouteDescription::NameDescription*>(next->GetDescription(RouteDescription::WAY_NAME_DESC));
          nextWay=wayMap[next->GetPathWayId()];

          if (motorwayLinkTypes.find(nextWay->GetType())==motorwayLinkTypes.end()) {
            break;
          }

          next++;
        }

        if (nextWay.Valid()) {
          targetIsMotorway=motorwayTypes.find(nextWay->GetType())!=motorwayTypes.end();
        }

        if (originIsMotorway && targetIsMotorway) {
          RouteDescription::MotorwayChangeDescriptionRef desc=new RouteDescription::MotorwayChangeDescription(originName,
                                                                                                              nextName);

          node->AddDescription(RouteDescription::MOTORWAY_CHANGE_DESC,
                               desc);

          node=next;
          lastNode=node++;
          continue;

        }

        if (originIsMotorway && !targetIsMotorway) {
          RouteDescription::MotorwayLeaveDescriptionRef desc=new RouteDescription::MotorwayLeaveDescription(originName);

          node->AddDescription(RouteDescription::MOTORWAY_LEAVE_DESC,
                               desc);

          HandleDirectionChange(description.Nodes(),
                                next,
                                wayMap);

          node=next;
          lastNode=node++;

          continue;
        }

        if (!originIsMotorway && targetIsMotorway) {
          RouteDescription::MotorwayEnterDescriptionRef desc=new RouteDescription::MotorwayEnterDescription(nextName);

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
                                     node,
                                     wayMap)) {
        lastNode=node++;
        continue;
      }

      if (HandleNameChange(description.Nodes(),
                           lastNode,
                           node,
                           wayMap)) {
        lastNode=node++;
        continue;
      }

      lastNode=node++;
    }

    return true;
  }

  bool RoutePostprocessor::PostprocessRouteDescription(RouteDescription& description,
                                                       const RoutingProfile& profile,
                                                       Database& database,
                                                       std::list<PostprocessorRef> processors)
  {
    size_t pos=1;
    for (std::list<PostprocessorRef>::iterator p=processors.begin();
        p!=processors.end();
        ++p) {
      PostprocessorRef processor=*p;

      if (!processor->Process(profile,
                              description,
                              database)) {
        std::cerr << "Error during execution of postprocessor " << pos << std::endl;
        return false;
      }

      pos++;
    }

    return true;
  }
}

