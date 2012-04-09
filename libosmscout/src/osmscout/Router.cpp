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

#include <osmscout/Router.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>

#include <osmscout/ObjectRef.h>
#include <osmscout/RoutingProfile.h>
#include <osmscout/TypeConfigLoader.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/private/Math.h>

#include <iomanip>
#include <limits>

//#define DEBUG_ROUTING

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

      for (std::vector<Id>::const_iterator id=node->GetWays().begin();
          id!=node->GetWays().end();
          ++id) {
        wayIds.insert(*id);
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

    RouteDescription::NameDescriptionRef lastDescription;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      // The last node does not have a pathWayId set, since we are not going anywhere from the target node!
      if (!node->HasPathWay()) {
        break;
      }

      WayRef                               way=wayMap[node->GetPathWayId()];
      RouteDescription::NameDescriptionRef description=new RouteDescription::NameDescription(way->GetName(),
                                                                                             way->GetRefName());

      node->AddDescription(RouteDescription::WAY_NAME_DESC,
                           description);
    }

    return true;
  }

  bool RoutePostprocessor::WayNameChangedPostprocessor::Process(const RoutingProfile& profile,
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

      for (std::vector<Id>::const_iterator id=node->GetWays().begin();
          id!=node->GetWays().end();
          ++id) {
        wayIds.insert(*id);
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
    // Collect (relevant) way name changes
    //

    WayRef lastInterestingWay;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      // The last node does not have a pathWayId set, since we are not going anywhere from the target node!
      if (!node->HasPathWay()) {
        break;
      }

      RouteDescription::NameDescriptionRef originDescription;
      RouteDescription::NameDescriptionRef targetDescription;
      WayRef                               way=wayMap[node->GetPathWayId()];

      if (lastInterestingWay.Valid()) {
        // We didn't change street name and ref, so we do not create a new entry...
        if (lastInterestingWay->GetName()==way->GetName() &&
            lastInterestingWay->GetRefName()==way->GetRefName()) {
          continue;
        }

        // We skip steps where street does not have any names and silently
        // assume they still have the old name (currently this happens for
        // motorway links that no have a name.
        if (way->GetName().empty() &&
            way->GetRefName().empty()) {
          continue;
        }

        // If the ref name is still the same but the way name changes and the new way is a bridge
        // we assume that the name changed just because of the bridge and do not see this as
        // relevant name change.
        // TODO: Check if this is because of some import error
        if (lastInterestingWay->GetName().empty() &&
            !way->GetName().empty() &&
            lastInterestingWay->GetRefName()==way->GetRefName() &&
            way->IsBridge()) {
          continue;
        }

        originDescription=new RouteDescription::NameDescription(lastInterestingWay->GetName(),
                                                                lastInterestingWay->GetRefName());
      }

      targetDescription=new RouteDescription::NameDescription(way->GetName(),
                                                              way->GetRefName());


      node->AddDescription(RouteDescription::WAY_NAME_CHANGED_DESC,
                           new RouteDescription::NameChangedDescription(originDescription,
                                                                        targetDescription));
      lastInterestingWay=way;
    }

    return true;
  }

  void RoutePostprocessor::CrossingWaysPostprocessor::AddCrossingWaysDescriptions(RouteDescription::CrossingWaysDescription* description,
                                                                                  const RouteDescription::Node& node,
                                                                                  const WayRef& originWay,
                                                                                  const WayRef& targetWay,
                                                                                  const std::map<Id,WayRef>& wayMap)
  {
    for (std::vector<Id>::const_iterator id=node.GetWays().begin();
        id!=node.GetWays().end();
        ++id) {
      std::map<Id,WayRef>::const_iterator way=wayMap.find(*id);

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

  void RoutePostprocessor::CrossingWaysPostprocessor::AddMotorwayType(TypeId type)
  {
    motorwayTypes.insert(type);
  }

  void RoutePostprocessor::CrossingWaysPostprocessor::AddMotorwayLinkType(TypeId type)
  {
    motorwayLinkTypes.insert(type);
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

      for (std::vector<Id>::const_iterator id=node->GetWays().begin();
          id!=node->GetWays().end();
          ++id) {
        wayIds.insert(*id);
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

    std::list<RouteDescription::Node>::iterator lastNode=description.Nodes().end();
    std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
    while (node!=description.Nodes().end()) {
      if (node->GetWays().empty()) {
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

      RouteDescription::CrossingWaysDescriptionRef desc=new RouteDescription::CrossingWaysDescription(node->GetPaths().size(),
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


    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      std::list<RouteDescription::Node>::iterator prevNode=node;
      std::list<RouteDescription::Node>::iterator nextNode=node;

      prevNode--;
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
                                                                      std::list<RouteDescription::Node>::iterator& node,
                                                                      const std::map<Id,WayRef>& wayMap)
  {
    std::list<RouteDescription::Node>::const_iterator lastNode=node;
    RouteDescription::NameDescriptionRef              nextName;
    RouteDescription::NameDescriptionRef              lastName;

    lastNode--;

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

    RouteDescription::DescriptionRef     desc=node->GetDescription(RouteDescription::DIRECTION_DESC);
    RouteDescription::DirectionDescriptionRef turnDesc=dynamic_cast<RouteDescription::DirectionDescription*>(desc.Get());

    if (lastName->GetName()==nextName->GetName() &&
        lastName->GetRef()==nextName->GetRef()) {
      if (turnDesc->GetCurve()!=RouteDescription::DirectionDescription::slightlyLeft &&
          turnDesc->GetCurve()!=RouteDescription::DirectionDescription::straightOn &&
          turnDesc->GetCurve()!=RouteDescription::DirectionDescription::slightlyRight) {

          node->AddDescription(RouteDescription::TURN_DESC,
                               new RouteDescription::TurnDescription());

          return true;
      }
    }
    else if (turnDesc->GetCurve()!=RouteDescription::DirectionDescription::straightOn) {

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

      for (std::vector<Id>::const_iterator id=node->GetWays().begin();
          id!=node->GetWays().end();
          ++id) {
        wayIds.insert(*id);
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

    std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
    while (node!=description.Nodes().end()) {
      std::list<RouteDescription::Node>::iterator lastNode=node;

      lastNode--;

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
        node++;
        continue;
      }

      if (!originWay->IsRoundabout() &&
          targetWay->IsRoundabout()) {
        HandleRoundaboutEnter(*node);
        inRoundabout=true;
        roundaboutCrossingCounter=0;
        node++;
        continue;
      }

      if (originWay->IsRoundabout() &&
               !targetWay->IsRoundabout()) {
        HandleRoundaboutNode(*node);
        HandleRoundaboutLeave(*node);
        inRoundabout=false;

        node++;
        continue;
      }

      // Non-Link, non-motorway to motorway
      if (motorwayLinkTypes.find(originWay->GetType())==motorwayLinkTypes.end() &&
          motorwayTypes.find(originWay->GetType())==motorwayTypes.end() &&
          motorwayTypes.find(targetWay->GetType())!=motorwayTypes.end()) {
        HandleDirectMotorwayEnter(*node,
                                  targetName);

        node++;
        continue;
      }

      // Motorway to Non-Link, non-motorway
      if (motorwayTypes.find(originWay->GetType())!=motorwayTypes.end() &&
          motorwayLinkTypes.find(targetWay->GetType())==motorwayLinkTypes.end() &&
          motorwayTypes.find(targetWay->GetType())==motorwayTypes.end()) {
        HandleDirectMotorwayLeave(*node,
                                  originName);

        node++;
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
          node++;
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
          node++;

          continue;
        }

        if (!originIsMotorway && targetIsMotorway) {
          RouteDescription::MotorwayEnterDescriptionRef desc=new RouteDescription::MotorwayEnterDescription(nextName);

          node->AddDescription(RouteDescription::MOTORWAY_ENTER_DESC,
                               desc);

          node=next;
          node++;

          continue;
        }

        node++;
        continue;
      }

      if (inRoundabout) {
        HandleRoundaboutNode(*node);
      }
      else if (HandleDirectionChange(description.Nodes(),
                                     node,
                                     wayMap)) {
        node++;
        continue;
      }

      if (HandleNameChange(description.Nodes(),
                           node,
                           wayMap)) {
        node++;
        continue;
      }

      node++;
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

  RouterParameter::RouterParameter()
  : wayIndexCacheSize(10000),
    wayCacheSize(0),
    debugPerformance(false)
  {
    // no code
  }

  void RouterParameter::SetWayIndexCacheSize(unsigned long wayIndexCacheSize)
  {
    this->wayIndexCacheSize=wayIndexCacheSize;
  }

  void RouterParameter::SetWayCacheSize(unsigned long wayCacheSize)
  {
    this->wayCacheSize=wayCacheSize;
  }

  void RouterParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  unsigned long RouterParameter::GetWayIndexCacheSize() const
  {
    return wayIndexCacheSize;
  }

  unsigned long RouterParameter::GetWayCacheSize() const
  {
    return wayCacheSize;
  }

  bool RouterParameter::IsDebugPerformance() const
  {
    return debugPerformance;
  }

  Router::Router(const RouterParameter& parameter)
   : isOpen(false),
     debugPerformance(parameter.IsDebugPerformance()),
     wayDataFile("ways.dat",
                 "way.idx",
                  parameter.GetWayCacheSize(),
                  parameter.GetWayIndexCacheSize()),
     routeNodeDataFile("route.dat",
                       "route.idx",
                       0,
                       6000),
     typeConfig(NULL)
  {
    // no code
  }

  Router::~Router()
  {
    delete typeConfig;
  }

  bool Router::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    typeConfig=new TypeConfig();

    if (!LoadTypeData(path,*typeConfig)) {
      std::cerr << "Cannot load 'types.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!wayDataFile.Open(path,true,false)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!routeNodeDataFile.Open(path,true,false)) {
      std::cerr << "Cannot open 'route.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    isOpen=true;

    return true;
  }

  bool Router::IsOpen() const
  {
    return isOpen;
  }


  void Router::Close()
  {
    routeNodeDataFile.Close();
    wayDataFile.Close();

    isOpen=false;
  }

  void Router::FlushCache()
  {
    wayDataFile.FlushCache();
  }

  TypeConfig* Router::GetTypeConfig() const
  {
    return typeConfig;
  }

  void Router::GetClosestForwardRouteNode(const WayRef& way,
                                          Id nodeId,
                                          RouteNodeRef& routeNode,
                                          size_t& pos)
  {
    routeNode=NULL;

    size_t index=0;
    while (index<way->nodes.size()) {
      if (way->nodes[index].GetId()==nodeId) {
        break;
      }

      index++;
    }

    for (size_t i=index; i<way->nodes.size(); i++) {
      routeNodeDataFile.Get(way->nodes[i].GetId(),routeNode);

      if (routeNode.Valid()) {
        pos=i;
        return;
      }
    }
  }

  void Router::GetClosestBackwardRouteNode(const WayRef& way,
                                           Id nodeId,
                                           RouteNodeRef& routeNode,
                                           size_t& pos)
  {
    routeNode=NULL;

    size_t index=0;
    while (index<way->nodes.size()) {
      if (way->nodes[index].GetId()==nodeId) {
        break;
      }

      index++;
    }

    if (index>=way->nodes.size()) {
      return;
    }

    if (!way->IsOneway()) {
      for (int i=index-1; i>=0; i--) {
        routeNodeDataFile.Get(way->nodes[i].GetId(),routeNode);

        if (routeNode.Valid()) {
          pos=(size_t)i;
          return;
        }
      }
    }
  }

  bool Router::ResolveRNodesToList(const RNodeRef& end,
                                   const CloseMap& closeMap,
                                   std::list<RNodeRef>& nodes)
  {
    CloseMap::const_iterator current=closeMap.find(end->nodeId);
    RouteNodeRef                     routeNode;

    while (current->second->prev!=0) {
      CloseMap::const_iterator prev=closeMap.find(current->second->prev);

      nodes.push_back(current->second);

      current=prev;
    }

    nodes.push_back(current->second);

    std::reverse(nodes.begin(),nodes.end());

    return true;
  }

  bool Router::AddNodes(RouteData& route,
                        const std::vector<Id>& startCrossingWaysIds,
                        const std::vector<Path>& startPaths,
                        Id startNodeId,
                        Id wayId,
                        Id targetNodeId)
  {
    WayRef way;

    // Load the way to get all nodes between current route node and the next route node
    if (!wayDataFile.Get(wayId,way)) {
      std::cerr << "Cannot load way " << wayId << std::endl;
      return false;
    }

    size_t start=0;
    while (start<way->nodes.size() &&
        way->nodes[start].GetId()!=startNodeId) {
      start++;
    }

    size_t end=0;
    while (end<way->nodes.size() &&
        way->nodes[end].GetId()!=targetNodeId) {
      end++;
    }

    if (start>=way->nodes.size() ||
        end>=way->nodes.size()) {
      return false;
    }

    if (std::max(start,end)-std::min(start,end)==1) {
      route.AddEntry(way->nodes[start].GetId(),
                     startCrossingWaysIds,
                     startPaths,
                     way->GetId(),
                     targetNodeId);
    }
    else if (start<end) {
      route.AddEntry(way->nodes[start].GetId(),
                     startCrossingWaysIds,
                     startPaths,
                     way->GetId(),
                     way->nodes[start+1].GetId());

      for (size_t i=start+1; i<end-1; i++) {
        route.AddEntry(way->nodes[i].GetId(),
                       way->GetId(),
                       way->nodes[i+1].GetId());
      }

      route.AddEntry(way->nodes[end-1].GetId(),
                     way->GetId(),
                     targetNodeId);
    }
    else if (way->IsOneway()) {
      size_t pos=start+1;
      size_t next;

      if (pos>=way->nodes.size()) {
        pos=0;
      }

      next=pos+1;
      if (next>=way->nodes.size()) {
        next=0;
      }

      route.AddEntry(startNodeId,
                     startCrossingWaysIds,
                     startPaths,
                     way->GetId(),
                     way->nodes[pos].GetId());

      while (way->nodes[next].GetId()!=way->nodes[end].GetId()) {
        route.AddEntry(way->nodes[pos].GetId(),
                       way->GetId(),
                       way->nodes[next].GetId());

        pos++;
        if (pos>=way->nodes.size()) {
          pos=0;
        }

        next=pos+1;
        if (next>=way->nodes.size()) {
          next=0;
        }
      }

      route.AddEntry(way->nodes[pos].GetId(),
                     way->GetId(),
                     targetNodeId);
    }
    else {
      route.AddEntry(way->nodes[start].GetId(),
                     startCrossingWaysIds,
                     startPaths,
                     way->GetId(),
                     way->nodes[start-1].GetId());

      for (int i=start-1; i>(int)end+1; i--) {
        route.AddEntry(way->nodes[i].GetId(),
                       way->GetId(),
                       way->nodes[i-1].GetId());
      }

      route.AddEntry(way->nodes[end+1].GetId(),
                     way->GetId(),
                     targetNodeId);
    }

    return true;
  }

  bool Router::ResolveRNodesToRouteData(const std::list<RNodeRef>& nodes,
                                        Id startWayId,
                                        Id startNodeId,
                                        Id targetWayId,
                                        Id targetNodeId,
                                        RouteData& route)
  {
    if (nodes.empty()) {
      AddNodes(route,
               std::vector<Id>(),
               std::vector<Path>(),
               startNodeId,
               startWayId,
               targetNodeId);

      route.AddEntry(targetNodeId,
                     0,
                     0);
      return true;
    }

    if (startNodeId!=nodes.front()->nodeId) {
      // Start node to initial route node
      AddNodes(route,
               std::vector<Id>(),
               std::vector<Path>(),
               startNodeId,
               startWayId,
               nodes.front()->nodeId);
    }

    for (std::list<RNodeRef>::const_iterator n=nodes.begin();
        n!=nodes.end();
        n++) {
      std::list<RNodeRef>::const_iterator nn=n;

      nn++;

      RouteNodeRef node;

      // TODO: Optimize node=nextNode of the last step!
      if (!routeNodeDataFile.Get((*n)->nodeId,node)) {
        std::cerr << "Cannot load route node with id " << (*n)->nodeId << std::endl;
        return false;
      }


      // We do not have any follower node, push the final entry (leading nowhere)
      // to the route
      if (nn==nodes.end()) {
        if ((*n)->nodeId!=targetNodeId) {
          AddNodes(route,
                   node->ways,
                   node->GetPaths(),
                   (*n)->nodeId,
                   targetWayId,
                   targetNodeId);

          route.AddEntry(targetNodeId,
                         0,
                         0);
        }
        else {
          route.AddEntry((*n)->nodeId,
                         0,
                         0);
        }

        break;
      }

      RouteNodeRef nextNode;
      WayRef       way;
      size_t       pathIndex=0;

      if (!routeNodeDataFile.Get((*nn)->nodeId,nextNode)) {
        std::cerr << "Cannot load route node with id " << (*nn)->nodeId << std::endl;
        return false;
      }

      // Find the path with need to go to reach the next route node
      for (size_t i=0; i<node->paths.size(); i++) {
        if (node->ways[node->paths[i].wayIndex]==(*nn)->wayId) {
          pathIndex=i;
          break;
        }
      }

      AddNodes(route,
               node->ways,
               node->GetPaths(),
               node->id,
               node->ways[node->paths[pathIndex].wayIndex],
               nextNode->id);
    }

    return true;
  }

  bool Router::CalculateRoute(const RoutingProfile& profile,
                              Id startWayId, Id startNodeId,
                              Id targetWayId, Id targetNodeId,
                              RouteData& route)
  {
    WayRef                   startWay;
    double                   startLon=0.0L,startLat=0.0L;

    RouteNodeRef             startForwardRouteNode;
    size_t                   startForwardNodePos;
    RouteNodeRef             startBackwardRouteNode;
    size_t                   startBackwardNodePos;

    WayRef                   targetWay;
    double                   targetLon=0.0L,targetLat=0.0L;

    RouteNodeRef             targetForwardRouteNode;
    size_t                   targetForwardNodePos;
    RouteNodeRef             targetBackwardRouteNode;
    size_t                   targetBackwardNodePos;

    WayRef                   currentWay;

    // Sorted list (smallest cost first) of ways to check (we are using a std::set)
    OpenList                 openList;
    // Map routing nodes by id
    OpenMap                  openMap;
    CloseMap                 closeMap;

    size_t                   nodesLoadedCount=0;
    size_t                   nodesIgnoredCount=0;
    size_t                   maxOpenList=0;
    size_t                   maxCloseMap=0;

    StopClock clock;

    route.Clear();

#if defined(OSMSCOUT_HASHMAP_HAS_RESERVE)
    openMap.reserve(10000);
    closeMap.reserve(250000);
#endif

    if (!wayDataFile.Get(startWayId,
                         startWay)) {
      std::cerr << "Cannot get start way!" << std::endl;
      return false;
    }

    if (!wayDataFile.Get(targetWayId,
                         targetWay)) {
      std::cerr << "Cannot get end way!" << std::endl;
      return false;
    }

    size_t index=0;
    while (index<startWay->nodes.size()) {
      if (startWay->nodes[index].GetId()==startNodeId) {
        startLon=startWay->nodes[index].GetLon();
        startLat=startWay->nodes[index].GetLat();
        break;
      }

      index++;
    }

    if (index>=startWay->nodes.size()) {
      std::cerr << "Given start node is not part of start way" << std::endl;
      return false;
    }

    GetClosestForwardRouteNode(startWay,startNodeId,
                               startForwardRouteNode,startForwardNodePos);
    GetClosestBackwardRouteNode(startWay,startNodeId,
                                startBackwardRouteNode,startBackwardNodePos);

    if (startForwardRouteNode.Invalid() &&
        startBackwardRouteNode.Invalid()) {
      std::cerr << "No route node found for start way" << std::endl;
      return false;
    }

    index=0;
    while (index<targetWay->nodes.size()) {
      if (targetWay->nodes[index].GetId()==targetNodeId) {
        targetLon=targetWay->nodes[index].GetLon();
        targetLat=targetWay->nodes[index].GetLat();
        break;
      }

      index++;
    }

    if (index>=targetWay->nodes.size()) {
      std::cerr << "Given target node is not part of target way" << std::endl;
      return false;
    }

    GetClosestForwardRouteNode(targetWay,targetNodeId,
                               targetForwardRouteNode,targetForwardNodePos);
    GetClosestBackwardRouteNode(targetWay,targetNodeId,
                                targetBackwardRouteNode,targetBackwardNodePos);

    if (targetForwardRouteNode.Invalid() &&
        targetBackwardRouteNode.Invalid()) {
      std::cerr << "No route node found for target way" << std::endl;
      return false;
    }

    if (startForwardRouteNode.Valid()) {
      RNodeRef node=new RNode(startForwardRouteNode->id,
                              startWay->GetId());

      node->currentCost=profile.GetCosts(startWay,
                                         GetSphericalDistance(startLon,
                                                              startLat,
                                                              startWay->nodes[startForwardNodePos].GetLon(),
                                                              startWay->nodes[startForwardNodePos].GetLat()));
      node->estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                               startLat,
                                                               targetLon,
                                                               targetLat));

      node->overallCost=node->currentCost+node->estimateCost;

      std::pair<OpenListRef,bool> result=openList.insert(node);
      openMap[node->nodeId]=result.first;
    }

    if (startBackwardRouteNode.Valid()) {
      RNodeRef node=new RNode(startBackwardRouteNode->id,
                              startWay->GetId());

      node->currentCost=profile.GetCosts(startWay,
                                         GetSphericalDistance(startLon,
                                                              startLat,
                                                              startWay->nodes[startBackwardNodePos].GetLon(),
                                                              startWay->nodes[startBackwardNodePos].GetLat()));
      node->estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                               startLat,
                                                               targetLon,
                                                               targetLat));

      node->overallCost=node->currentCost+node->estimateCost;

      std::pair<OpenListRef,bool> result=openList.insert(node);
      openMap[node->nodeId]=result.first;
    }

    currentWay=NULL;

    RNodeRef     current;
    RouteNodeRef currentRouteNode;

    do {
      //
      // Take entry from open list with lowest cost
      //

      current=*openList.begin();

      openMap.erase(current->nodeId);
      openList.erase(openList.begin());

      if (!routeNodeDataFile.Get(current->nodeId,currentRouteNode) ||
          !currentRouteNode.Valid()) {
        std::cerr << "Cannot load route node with id " << current->nodeId << std::endl;
        return false;
      }

      nodesLoadedCount++;

      // Get potential follower in the current way

#if defined(DEBUG_ROUTING)
      std::cout << "Analysing follower of node " << currentRouteNode->GetId() << " " << current->currentCost << " " << current->estimateCost << " " << current->overallCost << std::endl;
#endif
      for (size_t i=0; i<currentRouteNode->paths.size(); i++) {
        if (!profile.CanUse(*currentRouteNode,i)) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route from " << currentRouteNode->id << " to " << currentRouteNode->paths[i].id << " (wrong type " << typeConfig->GetTypeInfo(currentRouteNode->paths[i].type).GetName()  << ")" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!current->access && currentRouteNode->paths[i].HasAccess()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route from " << currentRouteNode->id << " to " << currentRouteNode->paths[i].id << " (moving from non-accessible way back to accessible way)" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!currentRouteNode->excludes.empty()) {
          bool canTurnedInto=true;
          for (size_t e=0; e<currentRouteNode->excludes.size(); e++) {
            if (currentRouteNode->excludes[e].sourceWay==current->wayId &&
                currentRouteNode->excludes[e].targetPath==i) {
#if defined(DEBUG_ROUTING)
              WayRef sourceWay;
              WayRef targetWay;

              wayDataFile.Get(current->wayId,sourceWay);
              wayDataFile.Get(currentRouteNode->ways[currentRouteNode->paths[i].wayIndex],targetWay);

              std::cout << "  Node " <<  currentRouteNode->id << ": ";
              std::cout << "Cannot turn from " << current->wayId << " " << sourceWay->GetName() << " (" << sourceWay->GetRefName()  << ")";
              std::cout << " into ";
              std::cout << currentRouteNode->ways[currentRouteNode->paths[i].wayIndex] << " " << targetWay->GetName() << " (" << targetWay->GetRefName()  << ")" << std::endl;
#endif
              canTurnedInto=false;
              break;
            }
          }

          if (!canTurnedInto) {
            nodesIgnoredCount++;
            continue;
          }
        }

        CloseMap::const_iterator closeEntry=closeMap.find(currentRouteNode->paths[i].id);

        if (closeEntry!=closeMap.end()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route node " << currentRouteNode->paths[i].id << " (closed)" << std::endl;
#endif
          continue;
        }

        double currentCost=current->currentCost+
                           profile.GetCosts(*currentRouteNode,i);

        // TODO: Turn costs

        OpenMap::iterator openEntry=openMap.find(currentRouteNode->paths[i].id);

        // Check, if we already have a cheaper path to the new node. If yes, do not put the new path
        // into the open list
        if (openEntry!=openMap.end() &&
            (*openEntry->second)->currentCost<=currentCost) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route node " << currentRouteNode->paths[i].id << " (cheaper route exists " << currentCost << "<=>" << (*openEntry->second)->currentCost << ")" << std::endl;
#endif
          continue;
        }

        double distanceToTarget=GetSphericalDistance(currentRouteNode->paths[i].lon,
                                                     currentRouteNode->paths[i].lat,
                                                     targetLon,
                                                     targetLat);
        // Estimate costs for the rest of the distance to the target
        double estimateCost=profile.GetCosts(distanceToTarget);
        double overallCost=currentCost+estimateCost;

        // If we already have the node in the open list, but the new path is cheaper,
        // update the existing entry
        if (openEntry!=openMap.end()) {
          RNodeRef node=*openEntry->second;

          node->prev=currentRouteNode->id;
          node->wayId=currentRouteNode->ways[currentRouteNode->paths[i].wayIndex];

          node->currentCost=currentCost;
          node->estimateCost=estimateCost;
          node->overallCost=overallCost;
          node->access=currentRouteNode->paths[i].HasAccess();

#if defined(DEBUG_ROUTING)
          std::cout << "  Updating route " << node->nodeId << " via way " << node->wayId << " " << currentCost << " " << estimateCost << " " << overallCost << std::endl;
#endif

          openList.erase(openEntry->second);

          std::pair<OpenListRef,bool> result=openList.insert(node);
          openEntry->second=result.first;
        }
        else {
          RNodeRef node=new RNode(currentRouteNode->paths[i].id,
                                  currentRouteNode->ways[currentRouteNode->paths[i].wayIndex],
                                  currentRouteNode->id);

          node->currentCost=currentCost;
          node->estimateCost=estimateCost;
          node->overallCost=overallCost;
          node->access=currentRouteNode->paths[i].HasAccess();

#if defined(DEBUG_ROUTING)
          std::cout << "  Inserting route " << node->nodeId <<  " via way " << node->wayId  << " " << currentCost << " " << estimateCost << " " << overallCost << std::endl;
#endif

          std::pair<OpenListRef,bool> result=openList.insert(node);
          openMap[node->nodeId]=result.first;
        }
      }

      //
      // Added current node to close map
      //

      closeMap[current->nodeId]=current;

      maxOpenList=std::max(maxOpenList,openMap.size());
      maxCloseMap=std::max(maxCloseMap,closeMap.size());

    } while (!openList.empty() &&
             (targetForwardRouteNode.Invalid() || current->nodeId!=targetForwardRouteNode->id) &&
             (targetBackwardRouteNode.Invalid() || current->nodeId!=targetBackwardRouteNode->id));

    clock.Stop();

    std::cout << "From:                " << startWayId << "[" << startNodeId << "]" << std::endl;
    std::cout << "To:                  " << targetWayId << "[" << targetNodeId << "]" << std::endl;
    std::cout << "Time:                " << clock << std::endl;
#if defined(DEBUG_ROUTING)
    std::cout << "Cost:                " << current->currentCost << " " << current->estimateCost << " " << current->overallCost << std::endl;
#endif
    std::cout << "Route nodes loaded:  " << nodesLoadedCount << std::endl;
    std::cout << "Route nodes ignored: " << nodesIgnoredCount << std::endl;
    std::cout << "Max. OpenList size:  " << maxOpenList << std::endl;
    std::cout << "Max. CloseMap size:  " << maxCloseMap << std::endl;

    if (!((targetForwardRouteNode.Invalid() || current->nodeId==targetForwardRouteNode->id) ||
          (targetBackwardRouteNode.Invalid() || current->nodeId==targetBackwardRouteNode->id))) {
      std::cout << "No route found!" << std::endl;
      return false;
    }

    std::list<RNodeRef> nodes;

    if (!ResolveRNodesToList(current,
                             closeMap,
                             nodes)) {
      std::cerr << "Cannot resolve route nodes from routed path" << std::endl;
      return false;
    }

    if (!ResolveRNodesToRouteData(nodes,
                                  startWayId,startNodeId,
                                  targetWayId,targetNodeId,
                                  route)) {
      //std::cerr << "Cannot convert routing result to route data" << std::endl;
      return false;
    }

    return true;
  }

  bool Router::TransformRouteDataToWay(const RouteData& data,
                                       Way& way)
  {
    TypeId routeType;
    Way    tmp;

    routeType=typeConfig->GetWayTypeId("_route");

    assert(routeType!=typeIgnore);

    way=tmp;

    way.SetId(0);
    way.SetType(routeType);
    way.nodes.reserve(data.Entries().size());

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      if (iter->GetPathWayId()!=0) {
        WayRef w;

        if (!wayDataFile.Get(iter->GetPathWayId(),w)) {
          return false;
        }

        // Initial starting point
        if (iter==data.Entries().begin()) {
          for (size_t i=0; i<w->nodes.size(); i++) {
            if (w->nodes[i].GetId()==iter->GetCurrentNodeId()) {
              way.nodes.push_back(w->nodes[i]);
              break;
            }
          }
        }

        for (size_t i=0; i<w->nodes.size(); i++) {
          if (w->nodes[i].GetId()==iter->GetTargetNodeId()) {
            way.nodes.push_back(w->nodes[i]);
            break;
          }
        }
      }
    }

    return true;
  }

  bool Router::TransformRouteDataToPoints(const RouteData& data,
                                          std::list<Point>& points)
  {
    WayRef w;

    points.clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      if (iter->GetPathWayId()!=0) {

        if (w.Invalid() ||
            w->GetId()!=iter->GetPathWayId()) {
          if (!wayDataFile.Get(iter->GetPathWayId(),w)) {
            std::cerr << "Cannot load way with id " << iter->GetPathWayId() << std::endl;
            return false;
          }
        }

        // Initial starting point
        if (iter==data.Entries().begin()) {
          for (size_t i=0; i<w->nodes.size(); i++) {
            if (w->nodes[i].GetId()==iter->GetCurrentNodeId()) {
              Point point;

              point.SetId(w->nodes[i].GetId());
              point.SetCoordinates(w->nodes[i].GetLat(),
                                   w->nodes[i].GetLon());

              points.push_back(point);
              break;
            }
          }
        }

        // target node of current path
        for (size_t i=0; i<w->nodes.size(); i++) {
          if (w->nodes[i].GetId()==iter->GetTargetNodeId()) {
            Point point;

            point.SetId(w->nodes[i].GetId());
            point.SetCoordinates(w->nodes[i].GetLat(),
                                 w->nodes[i].GetLon());

            points.push_back(point);
            break;
          }
        }
      }
    }

    return true;
  }

  bool Router::TransformRouteDataToRouteDescription(const RouteData& data,
                                                    RouteDescription& description)
  {
    TypeId routeType;
    Way    tmp;

    routeType=typeConfig->GetWayTypeId("_route");

    assert(routeType!=typeIgnore);

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      description.AddNode(iter->GetCurrentNodeId(),
                          iter->GetWays(),
                          iter->GetPaths(),
                          iter->GetPathWayId(),
                          iter->GetTargetNodeId());
    }

    return true;
  }

  bool PostprocessRouteDescription(RouteDescription& description,
                                   const std::string& startDescription,
                                   const std::string& targetDescription)
  {
    return true;
  }

  void Router::DumpStatistics()
  {
    wayDataFile.DumpStatistics();
    routeNodeDataFile.DumpStatistics();
  }
}

