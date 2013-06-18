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

#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>

namespace osmscout {

  RoutePostprocessor::Postprocessor::~Postprocessor()
  {
    // no code
  }

  bool RoutePostprocessor::Postprocessor::ResolveAllAreasAndWays(RouteDescription& description,
                                                                 Database& database,
                                                                 OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areaMap,
                                                                 OSMSCOUT_HASHMAP<FileOffset,WayRef>& wayMap)
  {
    std::set<FileOffset>         areaOffsets;
    std::vector<AreaRef>         areas;

    std::set<FileOffset>         wayOffsets;
    std::vector<WayRef>          ways;

    for (std::list<RouteDescription::Node>::iterator node=description.Nodes().begin();
        node!=description.Nodes().end();
        ++node) {
      if (node->HasPathObject()) {
        switch (node->GetPathObject().GetType()) {
        case refNone:
        case refNode:
          assert(false);
          break;
        case refArea:
          areaOffsets.insert(node->GetPathObject().GetFileOffset());
        break;
        case refWay:
          wayOffsets.insert(node->GetPathObject().GetFileOffset());
          break;
        }
      }

      for (std::vector<Path>::const_iterator path=node->GetPaths().begin();
          path!=node->GetPaths().end();
          ++path) {
        switch (path->GetObject().GetType()) {
        case refNone:
        case refNode:
          assert(false);
          break;
        case refArea:
          areaOffsets.insert(path->GetObject().GetFileOffset());
        break;
        case refWay:
          wayOffsets.insert(path->GetObject().GetFileOffset());
          break;
        }
      }
    }

    if (!database.GetAreasByOffset(areaOffsets,areas)) {
      std::cerr << "Cannot retrieve crossing areas" << std::endl;
      return false;
    }

    if (!database.GetWaysByOffset(wayOffsets,ways)) {
      std::cerr << "Cannot retrieve crossing ways" << std::endl;
      return false;
    }

    for (std::vector<WayRef>::const_iterator way=ways.begin();
         way!=ways.end();
         ++way) {
      wayMap[(*way)->GetFileOffset()]=*way;
    }

    wayOffsets.clear();
    ways.clear();

    for (std::vector<AreaRef>::const_iterator area=areas.begin();
         area!=areas.end();
         ++area) {
      areaMap[(*area)->GetFileOffset()]=*area;
    }

    areaOffsets.clear();
    areas.clear();

    return true;
  }

  RouteDescription::NameDescriptionRef RoutePostprocessor::Postprocessor::GetNameDescription(const ObjectFileRef& object,
                                                                                             const OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areaMap,
                                                                                             const OSMSCOUT_HASHMAP<FileOffset,WayRef>& wayMap)
  {
    RouteDescription::NameDescriptionRef description;

    if (object.GetType()==refArea) {
      OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find(object.GetFileOffset());

      assert(entry!=areaMap.end());

      description=new RouteDescription::NameDescription(entry->second->rings.front().GetName());
    }
    else if (object.GetType()==refWay) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(object.GetFileOffset());

      assert(entry!=wayMap.end());

      description=new RouteDescription::NameDescription(entry->second->GetName(),
                                                        entry->second->GetRefName());
    }
    else {
      assert(false);
    }

    return description;
  }

  bool RoutePostprocessor::Postprocessor::IsRoundabout(const ObjectFileRef& object,
                                                       const OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areaMap,
                                                       const OSMSCOUT_HASHMAP<FileOffset,WayRef>& wayMap)
  {
    if (object.GetType()==refArea) {
      OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find(object.GetFileOffset());

      assert(entry!=areaMap.end());

      return false;
    }
    else if (object.GetType()==refWay) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(object.GetFileOffset());

      assert(entry!=wayMap.end());

      return entry->second->IsRoundabout();
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::Postprocessor::IsOfType(const ObjectFileRef& object,
                                                   const OSMSCOUT_HASHSET<TypeId>& types,
                                                   const OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areaMap,
                                                   const OSMSCOUT_HASHMAP<FileOffset,WayRef>& wayMap)
  {
    if (object.GetType()==refArea) {
      OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find(object.GetFileOffset());

      assert(entry!=areaMap.end());

      return types.find(entry->second->GetType())!=types.end();
    }
    else if (object.GetType()==refWay) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(object.GetFileOffset());

      assert(entry!=wayMap.end());

      return types.find(entry->second->GetType())!=types.end();
    }
    else {
      assert(false);

      return false;
    }
  }

  bool RoutePostprocessor::Postprocessor::IsNodeStartOrEndOfObject(const ObjectFileRef& nodeObject,
                                                                   size_t nodeIndex,
                                                                   const ObjectFileRef& object,
                                                                   const OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areaMap,
                                                                   const OSMSCOUT_HASHMAP<FileOffset,WayRef>& wayMap)
  {
    Id nodeId;

    if (object.GetType()==refArea) {
      OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find(nodeObject.GetFileOffset());

      assert(entry!=areaMap.end());

      nodeId=entry->second->rings.front().ids[nodeIndex];
    }
    else if (object.GetType()==refWay) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(nodeObject.GetFileOffset());

      assert(entry!=wayMap.end());

      nodeId=entry->second->ids[nodeIndex];
    }
    else {
      assert(false);
    }

    if (object.GetType()==refArea) {
      return false;
    }
    else if (object.GetType()==refWay) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(object.GetFileOffset());

      assert(entry!=wayMap.end());

      return entry->second->ids.front()==nodeId ||
             entry->second->ids.back()==nodeId;
    }
    else {
      assert(false);

      return false;
    }
  }

  void RoutePostprocessor::Postprocessor::GetCoordinates(const ObjectFileRef& object,
                                                         size_t nodeIndex,
                                                         const OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areaMap,
                                                         const OSMSCOUT_HASHMAP<FileOffset,WayRef>& wayMap,
                                                         double& lat,
                                                         double& lon)
  {
    if (object.GetType()==refArea) {
      OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find(object.GetFileOffset());

      assert(entry!=areaMap.end());

      lat=entry->second->rings.front().nodes[nodeIndex].lat;
      lon=entry->second->rings.front().nodes[nodeIndex].lon;
    }
    else if (object.GetType()==refWay) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(object.GetFileOffset());

      assert(entry!=wayMap.end());

      entry->second->GetCoordinates(nodeIndex,
                                    lat,
                                    lon);
    }
    else {
      assert(false);
    }
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
    if (!description.Nodes().empty()) {
      description.Nodes().front().AddDescription(RouteDescription::NODE_START_DESC,
                                                 new RouteDescription::StartDescription(startDescription));
    }

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
    if (!description.Nodes().empty()) {
      description.Nodes().back().AddDescription(RouteDescription::NODE_TARGET_DESC,
                                                new RouteDescription::TargetDescription(targetDescription));
    }

    return true;
  }

  bool RoutePostprocessor::DistanceAndTimePostprocessor::Process(const RoutingProfile& profile,
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

    for (std::list<RouteDescription::Node>::iterator iter=description.Nodes().begin();
         iter!=description.Nodes().end();
         ++iter) {
      // The last node does not have a pathWayId set, since we are not going anywhere!
      if (iter->HasPathObject()) {
        // Only load the next way, if it is different from the old one
        curObject=iter->GetPathObject();

        if (curObject!=prevObject) {
          switch (iter->GetPathObject().GetType()) {
          case refNone:
          case refNode:
            assert(false);
            break;
          case refArea:
            if (!database.GetAreaByOffset(curObject.GetFileOffset(),area)) {
              std::cout << "Error while loading area with offset " << curObject.GetFileOffset() << std::endl;
              return false;
            }
            break;
          case refWay:
            if (!database.GetWayByOffset(curObject.GetFileOffset(),way)) {
              std::cout << "Error while loading way with offset " << curObject.GetFileOffset() << std::endl;
              return false;
            }
            break;
          }
        }

        switch (iter->GetPathObject().GetType()) {
        case refNone:
        case refNode:
          assert(false);
          break;
        case refArea:
          curCoord=area->rings.front().nodes[iter->GetCurrentNodeIndex()];
          break;
        case refWay:
          curCoord=way->nodes[iter->GetCurrentNodeIndex()];
        }

        // There is no delta for the first route node
        if (prevObject.Valid()) {
          double deltaDistance=GetEllipsoidalDistance(prevCoord.GetLon(),
                                                      prevCoord.GetLat(),
                                                      curCoord.GetLon(),
                                                      curCoord.GetLat());

          double deltaTime=0.0;

          if (iter->GetPathObject().GetType()==refArea) {
            deltaTime=profile.GetTime(area,
                                      deltaDistance);
          }
          else if (iter->GetPathObject().GetType()==refWay) {
            deltaTime=profile.GetTime(way,
                                      deltaDistance);
          }

          distance+=deltaDistance;
          time+=deltaTime;
        }
      }

      iter->SetDistance(distance);
      iter->SetTime(time);

      prevObject=curObject;
      prevCoord=curCoord;
    }

    return true;
  }

  bool RoutePostprocessor::WayNamePostprocessor::Process(const RoutingProfile& profile,
                                                         RouteDescription& description,
                                                         Database& database)
  {
    OSMSCOUT_HASHMAP<FileOffset,AreaRef> areaMap;
    OSMSCOUT_HASHMAP<FileOffset,WayRef>  wayMap;

    if (!ResolveAllAreasAndWays(description,
                                database,
                                areaMap,
                                wayMap)) {
      return false;
    }

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
        AreaRef                              area=areaMap[node->GetPathObject().GetFileOffset()];
        RouteDescription::NameDescriptionRef nameDesc=new RouteDescription::NameDescription(area->rings.front().GetName());

        node->AddDescription(RouteDescription::WAY_NAME_DESC,
                             nameDesc);
      }
      else if (node->GetPathObject().GetType()==refWay) {
        WayRef                               way=wayMap[node->GetPathObject().GetFileOffset()];
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
    }

    return true;
  }

  void RoutePostprocessor::CrossingWaysPostprocessor::AddCrossingWaysDescriptions(RouteDescription::CrossingWaysDescription* description,
                                                                                  const RouteDescription::Node& node,
                                                                                  const ObjectFileRef& originObject,
                                                                                  const ObjectFileRef& targetObject,
                                                                                  const OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areaMap,
                                                                                  const OSMSCOUT_HASHMAP<FileOffset,WayRef>& wayMap)
  {
    for (std::vector<Path>::const_iterator path=node.GetPaths().begin();
        path!=node.GetPaths().end();
        ++path) {
      if (path->GetObject().Valid()) {
        // Way is origin way and starts or ends here so it is not an additional crossing way
        if (originObject.Valid() &&
            path->GetObject()==originObject &&
            IsNodeStartOrEndOfObject(node.GetPathObject(),
                                     node.GetCurrentNodeIndex(),
                                     originObject,
                                     areaMap,
                                     wayMap)) {
          continue;
        }

        // Way is target way and starts or ends here so it is not an additional crossing way
        if (targetObject.Valid() &&
            path->GetObject()==targetObject &&
            IsNodeStartOrEndOfObject(node.GetPathObject(),
                                     node.GetCurrentNodeIndex(),
                                     targetObject,
                                     areaMap,
                                     wayMap)) {
          continue;
        }

        // ways is origin way and target way so it is not an additional crossing way
        if (originObject.Valid() &&
            targetObject.Valid() &&
            path->GetObject()==originObject &&
            path->GetObject()==targetObject) {
          continue;
        }

        description->AddDescription(GetNameDescription(path->GetObject(),
                                                       areaMap,
                                                       wayMap));
      }
    }
  }

  bool RoutePostprocessor::CrossingWaysPostprocessor::Process(const RoutingProfile& profile,
                                                              RouteDescription& description,
                                                              Database& database)
  {
    OSMSCOUT_HASHMAP<FileOffset,AreaRef> areaMap;
    OSMSCOUT_HASHMAP<FileOffset,WayRef>  wayMap;

    if (!ResolveAllAreasAndWays(description,
                                database,
                                areaMap,
                                wayMap)) {
      return false;
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
          !node->HasPathObject()) {
        lastNode=node;
        node++;

        continue;
      }

      // Count existing exits
      size_t exitCount=0;

      for (size_t i=0; i<node->GetPaths().size(); i++) {
        // Leave out the path back to the last crossing (if it exists)
        if (lastCrossing!=description.Nodes().end() &&
            node->GetPaths()[i].GetObject()==lastCrossing->GetPathObject() &&
            node->GetPaths()[i].GetTargetNodeIndex()==lastCrossing->GetCurrentNodeIndex()) {
          continue;
        }

        if (!node->GetPaths()[i].IsTraversable()) {
          continue;
        }

        exitCount++;
      }

      RouteDescription::CrossingWaysDescriptionRef desc=new RouteDescription::CrossingWaysDescription(exitCount,
                                                                                                      GetNameDescription(lastNode->GetPathObject(),
                                                                                                                         areaMap,
                                                                                                                         wayMap),
                                                                                                      GetNameDescription(node->GetPathObject(),
                                                                                                                         areaMap,
                                                                                                                         wayMap));
      AddCrossingWaysDescriptions(desc,
                                  *node,
                                  lastNode->GetPathObject(),
                                  node->GetPathObject(),
                                  areaMap,
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
    OSMSCOUT_HASHMAP<FileOffset,AreaRef> areaMap;
    OSMSCOUT_HASHMAP<FileOffset,WayRef>  wayMap;

    if (!ResolveAllAreasAndWays(description,
                                database,
                                areaMap,
                                wayMap)) {
      return false;
    }

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

        GetCoordinates(prevNode->GetPathObject(),
                       prevNode->GetCurrentNodeIndex(),
                       areaMap,
                       wayMap,
                       prevLat,prevLon);

        GetCoordinates(node->GetPathObject(),
                       node->GetCurrentNodeIndex(),
                       areaMap,
                       wayMap,
                       lat,lon);

        GetCoordinates(nextNode->GetPathObject(),
                       nextNode->GetCurrentNodeIndex(),
                       areaMap,
                       wayMap,
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

            GetCoordinates(curveB->GetPathObject(),
                           curveB->GetCurrentNodeIndex(),
                           areaMap,
                           wayMap,
                           curveBLat,curveBLon);

            GetCoordinates(lookup->GetPathObject(),
                           lookup->GetCurrentNodeIndex(),
                           areaMap,
                           wayMap,
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
                             new RouteDescription::DirectionDescription(turnAngle,curveAngle));
      }
    }

    return true;
  }

  RoutePostprocessor::InstructionPostprocessor::State RoutePostprocessor::InstructionPostprocessor::GetInitialState(RouteDescription::Node& node,
                                                                                                                    const OSMSCOUT_HASHMAP<FileOffset,AreaRef>& areaMap,
                                                                                                                    const OSMSCOUT_HASHMAP<FileOffset,WayRef>& wayMap)
  {
    if (!node.HasPathObject()) {
      return street;
    }

    if (IsRoundabout(node.GetPathObject(),
                     areaMap,
                     wayMap)) {
      return roundabout;
    }

    if (node.GetPathObject().GetType()==refArea) {
      OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find(node.GetPathObject().GetFileOffset());

      assert(entry!=areaMap.end());

      if (motorwayLinkTypes.find(entry->second->GetType())!=motorwayLinkTypes.end()) {
        return link;
      }
      else if (motorwayTypes.find(entry->second->GetType())!=motorwayTypes.end()) {
        return motorway;
      }
    }
    else if (node.GetPathObject().GetType()==refWay) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(node.GetPathObject().GetFileOffset());

      assert(entry!=wayMap.end());

      if (motorwayLinkTypes.find(entry->second->GetType())!=motorwayLinkTypes.end()) {
        return link;
      }
      else if (motorwayTypes.find(entry->second->GetType())!=motorwayTypes.end()) {
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
    RouteDescription::RoundaboutEnterDescriptionRef desc=new RouteDescription::RoundaboutEnterDescription();

    node.AddDescription(RouteDescription::ROUNDABOUT_ENTER_DESC,
                        desc);
  }

  void RoutePostprocessor::InstructionPostprocessor::HandleRoundaboutNode(RouteDescription::Node& node)
  {
    if (node.HasDescription(RouteDescription::CROSSING_WAYS_DESC)) {
      RouteDescription::CrossingWaysDescriptionRef crossing=dynamic_cast<RouteDescription::CrossingWaysDescription*>(node.GetDescription(RouteDescription::CROSSING_WAYS_DESC));
#ifdef DEBUG
      std::cout << "Exits at node: " << node.GetCurrentNodeIndex() << " " << crossing->GetExitCount() << std::endl;
#endif
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
                                                                      std::list<RouteDescription::Node>::iterator& node)
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
                                                                           std::list<RouteDescription::Node>::iterator& node)
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
    OSMSCOUT_HASHMAP<FileOffset,AreaRef> areaMap;
    OSMSCOUT_HASHMAP<FileOffset,WayRef>  wayMap;

    if (!ResolveAllAreasAndWays(description,
                                database,
                                areaMap,
                                wayMap)) {
      return false;
    }

   //
    // Detect initial state
    //

    State  state=GetInitialState(description.Nodes().front(),
                                 areaMap,
                                 wayMap);

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
/*
      WayRef                               originWay;
      WayRef                               targetWay;*/
      RouteDescription::NameDescriptionRef originName;
      RouteDescription::NameDescriptionRef targetName;

      // First or last node
      if (lastNode==description.Nodes().end() ||
          lastNode->GetPathObject().Invalid() ||
          node->GetPathObject().Invalid()) {
        lastNode=node++;
        continue;
      }

      originName=dynamic_cast<RouteDescription::NameDescription*>(lastNode->GetDescription(RouteDescription::WAY_NAME_DESC));
      //originWay=wayMap[lastNode->GetPathObject()];

      if (node->HasPathObject()) {
        targetName=dynamic_cast<RouteDescription::NameDescription*>(node->GetDescription(RouteDescription::WAY_NAME_DESC));

        //targetWay=wayMap[node->GetPathObject()];
      }

      if (!IsRoundabout(lastNode->GetPathObject(),
                        areaMap,
                        wayMap) &&
          IsRoundabout(node->GetPathObject(),
                       areaMap,
                       wayMap)) {
        HandleRoundaboutEnter(*node);
        inRoundabout=true;
        roundaboutCrossingCounter=0;
        lastNode=node++;
        continue;
      }

      if (IsRoundabout(lastNode->GetPathObject(),
                       areaMap,
                       wayMap) &&
          !IsRoundabout(node->GetPathObject(),
                        areaMap,
                        wayMap)) {
        HandleRoundaboutNode(*node);
        HandleRoundaboutLeave(*node);
        inRoundabout=false;

        lastNode=node++;
        continue;
      }

      // Non-Link, non-motorway to motorway (enter motorway))
      if (!IsOfType(lastNode->GetPathObject(),
                    motorwayLinkTypes,
                    areaMap,
                    wayMap) &&
          !IsOfType(lastNode->GetPathObject(),
                    motorwayTypes,
                    areaMap,
                    wayMap) &&
          IsOfType(node->GetPathObject(),
                   motorwayTypes,
                   areaMap,
                   wayMap)) {
        HandleDirectMotorwayEnter(*node,
                                  targetName);

        lastNode=node++;
        continue;
      }

      // motorway to Non-Link, non-motorway /leave motorway)
      if (IsOfType(lastNode->GetPathObject(),
                   motorwayTypes,
                   areaMap,
                   wayMap) &&
          !IsOfType(node->GetPathObject(),
                    motorwayLinkTypes,
                    areaMap,
                    wayMap) &&
          !IsOfType(node->GetPathObject(),
                    motorwayTypes,
                    areaMap,
                    wayMap)) {
        HandleDirectMotorwayLeave(*node,
                                  originName);

        lastNode=node++;
        continue;
      }

      else if (!IsOfType(lastNode->GetPathObject(),
                         motorwayLinkTypes,
                         areaMap,
                         wayMap) &&
               IsOfType(node->GetPathObject(),
                        motorwayLinkTypes,
                        areaMap,
                        wayMap)) {
        bool                                        originIsMotorway=IsOfType(lastNode->GetPathObject(),
                                                                              motorwayTypes,
                                                                              areaMap,
                                                                              wayMap);
        bool                                        targetIsMotorway=false;
        std::list<RouteDescription::Node>::iterator next=node;
        RouteDescription::NameDescriptionRef        nextName;

        next++;
        while (next!=description.Nodes().end() &&
               next->HasPathObject()) {

          nextName=dynamic_cast<RouteDescription::NameDescription*>(next->GetDescription(RouteDescription::WAY_NAME_DESC));

          if (!IsOfType(next->GetPathObject(),
                        motorwayLinkTypes,
                        areaMap,
                        wayMap)) {
            break;
          }

          next++;
        }

        if (next->GetPathObject().Valid()) {
          targetIsMotorway=IsOfType(next->GetPathObject(),
                                    motorwayTypes,
                                    areaMap,
                                    wayMap);
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
                                next);

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

