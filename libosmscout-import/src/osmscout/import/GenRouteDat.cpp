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

#include <osmscout/import/GenRouteDat.h>

#include <algorithm>
#include <cassert>

#include <osmscout/private/Math.h>

#include <osmscout/DataFile.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  static uint8_t CopyFlags(const Way& way)
  {
    uint8_t flags=0;

    if (way.HasAccess()) {
      flags|=RouteNode::hasAccess;
    }

    return flags;
  }


  std::string RouteDataGenerator::GetDescription() const
  {
    return "Generate 'route.dat'";
  }

  bool RouteDataGenerator::ReadTurnRestrictions(const ImportParameter& parameter,
                                                Progress& progress,
                                                std::map<Id,std::vector<TurnRestrictionData> >& restrictions)
  {
    FileScanner             scanner;
    uint32_t                restrictionCount=0;
    uint32_t                wayCount=0;
    std::map<Id,FileOffset> wayIdOffsetMap;

    //
    // Just read the way ids
    //

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "turnrestr.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(restrictionCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t r=1; r<=restrictionCount; r++) {
      progress.SetProgress(r,restrictionCount);

      TurnRestrictionRef restriction=new TurnRestriction();

      if (!restriction->Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(restrictionCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      wayIdOffsetMap.insert(std::make_pair(restriction->GetFrom(),0));
      wayIdOffsetMap.insert(std::make_pair(restriction->GetTo(),0));
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'turnrestr.dat'");
      return false;
    }

    //
    // Now map way ids to file offsets
    //

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");

      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      progress.SetProgress(w,wayCount);

      FileOffset wayOffset;
      Way        way;

      if (!scanner.GetPos(wayOffset)) {
        progress.Error(std::string("Error while reading file offset of way for entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      std::map<Id,FileOffset>::iterator idOffsetEntry;


      idOffsetEntry=wayIdOffsetMap.find(way.GetId());

      if (idOffsetEntry!=wayIdOffsetMap.end()) {
        idOffsetEntry->second=wayOffset;
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'ways.dat'");
      return false;
    }

    //
    // Finally read restrictions again and replace way ids with way offsets

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "turnrestr.dat"),
                      FileScanner::Sequential,
                      true)) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(restrictionCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t r=1; r<=restrictionCount; r++) {
      progress.SetProgress(r,restrictionCount);

      TurnRestrictionRef restriction=new TurnRestriction();

      if (!restriction->Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(restrictionCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      TurnRestrictionData                      data;
      std::map<Id,FileOffset>::const_iterator  idOffsetEntry;


      idOffsetEntry=wayIdOffsetMap.find(restriction->GetFrom());

      if (idOffsetEntry==wayIdOffsetMap.end()) {
        progress.Error(std::string("Error while retrieving way offset for way id ")+
                       NumberToString(restriction->GetFrom()));
        return false;
      }

      data.fromWayOffset=idOffsetEntry->second;

      data.viaNodeId=restriction->GetVia();

      idOffsetEntry=wayIdOffsetMap.find(restriction->GetTo());

      if (idOffsetEntry==wayIdOffsetMap.end()) {
        progress.Error(std::string("Error while retrieving way offset for way id ")+
                       NumberToString(restriction->GetTo()));
        return false;
      }

      data.toWayOffset=idOffsetEntry->second;

      switch (restriction->GetType()) {
      case TurnRestriction::Allow:
        data.type=TurnRestrictionData::Allow;
        break;
      case TurnRestriction::Forbit:
        data.type=TurnRestrictionData::Forbit;
        break;
      }

      restrictions[restriction->GetVia()].push_back(data);
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'turnrestr.dat'");
      return false;
    }

    progress.Info(std::string("Read ")+NumberToString(restrictionCount)+" turn restrictions");

    return true;
  }

  bool RouteDataGenerator::CanTurn(const std::vector<TurnRestrictionData>& restrictions,
                                   FileOffset from,
                                   FileOffset to) const
  {
    bool defaultReturn=true;

    if (restrictions.empty()) {
      return true;
    }

    for (std::vector<TurnRestrictionData>::const_iterator restriction=restrictions.begin();
         restriction!=restrictions.end();
         ++restriction) {
      if (restriction->fromWayOffset==from) {
        if (restriction->type==TurnRestrictionData::Allow) {
          if (restriction->toWayOffset==to) {
            return true;
          }

          // If there are allow restrictions,everything else is forbidden
          defaultReturn=false;
        }
        else if (restriction->type==TurnRestrictionData::Forbit) {
          if (restriction->toWayOffset==to) {
            return false;
          }

          // If there are forbid restrictions, everything else is allowed
          defaultReturn=true;
        }
      }
    }

    // Now lets hope nobody is mixing allow and forbid!
    return defaultReturn;
  }

  bool RouteDataGenerator::ReadJunctions(const ImportParameter& parameter,
                                         Progress& progress,
                                         const TypeConfig& typeConfig,
                                         OSMSCOUT_HASHSET<Id>& junctions)
  {
    FileScanner         scanner;
    uint32_t            wayCount=0;
    std::map<Id,size_t> nodeWayCountMap;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");

      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      progress.SetProgress(w,wayCount);

      Way way;

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (way.GetType()==typeIgnore) {
        continue;
      }

      if (typeConfig.GetTypeInfo(way.GetType()).GetIgnore()) {
        continue;
      }

      if (!typeConfig.GetTypeInfo(way.GetType()).CanBeRoute()) {
        continue;
      }

      std::set<Id> nodeIds;

      for (std::vector<Point>::const_iterator node=way.nodes.begin();
          node!=way.nodes.end();
          node++) {
        if (nodeIds.find(node->GetId())==nodeIds.end()) {
          std::map<Id,size_t>::iterator entry=nodeWayCountMap.find(node->GetId());

          if (entry!=nodeWayCountMap.end()) {
            entry->second++;
          }
          else {
            nodeWayCountMap[node->GetId()]=1;
          }

          nodeIds.insert(node->GetId());
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'ways.dat'");
      return false;
    }

    for (std::map<Id,size_t>::iterator node=nodeWayCountMap.begin();
        node!=nodeWayCountMap.end();) {
      if (node->second>=2) {
        junctions.insert(node->first);
      }

      nodeWayCountMap.erase(node++);
    }

    return true;
  }

  bool RouteDataGenerator::ReadWayEndpoints(const ImportParameter& parameter,
                                            Progress& progress,
                                            const TypeConfig& typeConfig,
                                            const OSMSCOUT_HASHSET<Id>& junctions,
                                            NodeIdWayOffsetMap& endPointWayMap)
  {
    FileScanner scanner;
    uint32_t    wayCount=0;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");

      return false;
    }

    if (!scanner.Read(wayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=wayCount; w++) {
      progress.SetProgress(w,wayCount);

      Way        way;
      FileOffset fileOffset;

      if (!scanner.GetPos(fileOffset)) {
        progress.Error(std::string("Error while reading file offset for data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(wayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (way.GetType()==typeIgnore) {
        continue;
      }

      if (typeConfig.GetTypeInfo(way.GetType()).GetIgnore()) {
        continue;
      }

      if (!typeConfig.GetTypeInfo(way.GetType()).CanBeRoute()) {
        continue;
      }

      for (std::vector<Point>::const_iterator node=way.nodes.begin();
          node!=way.nodes.end();
          node++) {
        if ((node==way.nodes.begin() ||
            node->GetId()!=way.nodes.front().GetId()) &&
            junctions.find(node->GetId())!=junctions.end()) {
          endPointWayMap[node->GetId()].push_back(fileOffset);
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'ways.dat'");
      return false;
    }

    return true;
  }

  bool RouteDataGenerator::LoadWays(Progress& progress,
                                    FileScanner& scanner,
                                    const std::set<FileOffset>& fileOffsets,
                                    OSMSCOUT_HASHMAP<FileOffset,WayRef>& waysMap)
  {
    FileOffset oldPos;

    if (!scanner.GetPos(oldPos)) {
      progress.Error("Error while loading current file position");
      return false;
    }

    for (std::set<FileOffset>::const_iterator offset=fileOffsets.begin();
         offset!=fileOffsets.end();
         offset++) {
      if (!scanner.SetPos(*offset)) {
        progress.Error("Error while moving to way at offset " + NumberToString(*offset));
        return false;
      }

      WayRef way=new Way();

      way->Read(scanner);

      if (scanner.HasError()) {
        progress.Error("Error while loading way at offset " + NumberToString(*offset));
        return false;
      }

      waysMap[*offset]=way;
    }

    if (!scanner.SetPos(oldPos)) {
      progress.Error("Error while resetting current file position");
      return false;
    }

    return true;
  }

  uint8_t RouteDataGenerator::CalculateEncodedBearing(const Way& way,
                                                      size_t currentNode,
                                                      size_t nextNode,
                                                      bool clockwise) const
  {
    size_t currentNodeFollower;
    size_t nextNodePrecursor;

    if (clockwise) {
      currentNodeFollower=currentNode==way.nodes.size()-1 ? 0 : currentNode+1;
      nextNodePrecursor=nextNode>0 ? nextNode-1 :way.nodes.size()-1;
    }
    else {
      currentNodeFollower=currentNode>0 ? currentNode-1 :way.nodes.size()-1;
      nextNodePrecursor=nextNode==way.nodes.size()-1 ? 0 : nextNode+1;
    }

    double initialBearing=GetSphericalBearingInitial(way.nodes[currentNode].GetLon(),
                                                     way.nodes[currentNode].GetLat(),
                                                     way.nodes[currentNodeFollower].GetLon(),
                                                     way.nodes[currentNodeFollower].GetLat());
    double finalBearing=GetSphericalBearingInitial(way.nodes[nextNodePrecursor].GetLon(),
                                                   way.nodes[nextNodePrecursor].GetLat(),
                                                   way.nodes[nextNode].GetLon(),
                                                   way.nodes[nextNode].GetLat());

    // Transform in 0..360 Degree

    initialBearing=fmod(initialBearing*180.0/M_PI+360.0,360.0);
    finalBearing=fmod(initialBearing*180.0/M_PI+360.0,360.0);

    uint8_t bearing=initialBearing/10+100+finalBearing/10;

    return bearing;
  }

  void RouteDataGenerator::CalculateAreaPaths(RouteNode& routeNode,
                                              const Way& way,
                                              const NodeIdWayOffsetMap& nodeWayMap)
  {
    int    currentNode=0;
    double distance;

    // Find current route node in area
    while (currentNode<(int)way.nodes.size() &&
          way.nodes[currentNode].GetId()!=routeNode.id) {
      currentNode++;
    }

    // Make sure we found it
    assert(currentNode<(int)way.nodes.size());

    // Find next routing node in order

    int nextNode=currentNode+1;

    if (nextNode>=(int)way.nodes.size()) {
      nextNode=0;
    }

    distance=GetSphericalDistance(way.nodes[currentNode].GetLon(),
                                  way.nodes[currentNode].GetLat(),
                                  way.nodes[nextNode].GetLon(),
                                  way.nodes[nextNode].GetLat());

    while (nextNode!=currentNode &&
           nodeWayMap.find(way.nodes[nextNode].GetId())==nodeWayMap.end()) {
      int lastNode=nextNode;
      nextNode++;

      if (nextNode>=(int)way.nodes.size()) {
        nextNode=0;
      }

      if (nextNode!=currentNode) {
        distance+=GetSphericalDistance(way.nodes[lastNode].GetLon(),
                                       way.nodes[lastNode].GetLat(),
                                       way.nodes[nextNode].GetLon(),
                                       way.nodes[nextNode].GetLat());
      }
    }

    // Found next routing node in order
    if (nextNode!=currentNode &&
        way.nodes[nextNode].GetId()!=routeNode.id) {
      RouteNode::Path path;

      path.id=way.nodes[nextNode].GetId();
      path.wayIndex=routeNode.ways.size()-1;
      path.type=way.GetType();
      path.maxSpeed=way.GetMaxSpeed();
      path.grade=way.GetGrade();
      path.bearing=CalculateEncodedBearing(way,currentNode,nextNode,true);
      path.flags=CopyFlags(way);
      path.lat=way.nodes[nextNode].GetLat();
      path.lon=way.nodes[nextNode].GetLon();
      path.distance=distance;

      routeNode.paths.push_back(path);
    }

    // Find next routing node against order

    int prevNode=currentNode-1;

    if (prevNode<0) {
      prevNode=(int)(way.nodes.size()-1);
    }

    distance=GetSphericalDistance(way.nodes[currentNode].GetLon(),
                                  way.nodes[currentNode].GetLat(),
                                  way.nodes[prevNode].GetLon(),
                                  way.nodes[prevNode].GetLat());

    while (prevNode!=currentNode &&
        nodeWayMap.find(way.nodes[prevNode].GetId())==nodeWayMap.end()) {
      int lastNode=prevNode;
      prevNode--;

      if (prevNode<0) {
        prevNode=(int)(way.nodes.size()-1);
      }

      if (prevNode!=currentNode) {
        distance+=GetSphericalDistance(way.nodes[lastNode].GetLon(),
                                       way.nodes[lastNode].GetLat(),
                                       way.nodes[prevNode].GetLon(),
                                       way.nodes[prevNode].GetLat());
      }
    }

    // Found next routing node against order

    if (prevNode!=currentNode &&
        prevNode!=nextNode &&
        way.nodes[prevNode].GetId()!=routeNode.id) {
      RouteNode::Path path;

      path.id=way.nodes[prevNode].GetId();
      path.wayIndex=routeNode.ways.size()-1;
      path.type=way.GetType();
      path.maxSpeed=way.GetMaxSpeed();
      path.grade=way.GetGrade();
      path.bearing=CalculateEncodedBearing(way,currentNode,prevNode,false);
      path.flags=CopyFlags(way);
      path.lat=way.nodes[prevNode].GetLat();
      path.lon=way.nodes[prevNode].GetLon();
      path.distance=distance;

      routeNode.paths.push_back(path);
    }
  }

  void RouteDataGenerator::CalculateCircularWayPaths(RouteNode& routeNode,
                                                     const Way& way,
                                                     const NodeIdWayOffsetMap& nodeWayMap)
  {
    int    currentNode=0;
    double distance;

    // In path direction

    while (currentNode<(int)way.nodes.size() &&
          way.nodes[currentNode].GetId()!=routeNode.id) {
      currentNode++;
    }

    assert(currentNode<(int)way.nodes.size());

    int nextNode=currentNode+1;

    if (nextNode>=(int)way.nodes.size()) {
      nextNode=0;
    }

    distance=GetSphericalDistance(way.nodes[currentNode].GetLon(),
                                  way.nodes[currentNode].GetLat(),
                                  way.nodes[nextNode].GetLon(),
                                  way.nodes[nextNode].GetLat());

    while (nextNode!=currentNode &&
        nodeWayMap.find(way.nodes[nextNode].GetId())==nodeWayMap.end()) {
      int lastNode=nextNode;
      nextNode++;

      if (nextNode>=(int)way.nodes.size()) {
        nextNode=0;
      }

      if (nextNode!=currentNode) {
        distance+=GetSphericalDistance(way.nodes[lastNode].GetLon(),
                                       way.nodes[lastNode].GetLat(),
                                       way.nodes[nextNode].GetLon(),
                                       way.nodes[nextNode].GetLat());
      }
    }

    if (nextNode!=currentNode &&
        way.nodes[nextNode].GetId()!=routeNode.id) {
      RouteNode::Path path;

      path.id=way.nodes[nextNode].GetId();
      path.wayIndex=routeNode.ways.size()-1;
      path.type=way.GetType();
      path.maxSpeed=way.GetMaxSpeed();
      path.grade=way.GetGrade();
      path.bearing=CalculateEncodedBearing(way,currentNode,nextNode,true);
      path.flags=CopyFlags(way);
      path.lat=way.nodes[nextNode].GetLat();
      path.lon=way.nodes[nextNode].GetLon();
      path.distance=distance;

      routeNode.paths.push_back(path);
    }

    // Against path direction

    int prevNode=currentNode-1;

    if (prevNode<0) {
      prevNode=(int)(way.nodes.size()-1);
    }

    distance=GetSphericalDistance(way.nodes[currentNode].GetLon(),
                                  way.nodes[currentNode].GetLat(),
                                  way.nodes[prevNode].GetLon(),
                                  way.nodes[prevNode].GetLat());

    while (prevNode!=currentNode &&
        nodeWayMap.find(way.nodes[prevNode].GetId())==nodeWayMap.end()) {
      int lastNode=prevNode;
      prevNode--;

      if (prevNode<0) {
        prevNode=(int)(way.nodes.size()-1);
      }

      if (prevNode!=currentNode) {
        distance+=GetSphericalDistance(way.nodes[lastNode].GetLon(),
                                       way.nodes[lastNode].GetLat(),
                                       way.nodes[prevNode].GetLon(),
                                       way.nodes[prevNode].GetLat());
      }
    }

    if (prevNode!=currentNode &&
        prevNode!=nextNode &&
        way.nodes[prevNode].GetId()!=routeNode.id) {
      RouteNode::Path path;

      path.id=way.nodes[prevNode].GetId();
      path.wayIndex=routeNode.ways.size()-1;
      path.type=way.GetType();
      path.maxSpeed=way.GetMaxSpeed();
      path.grade=way.GetGrade();
      path.bearing=CalculateEncodedBearing(way,prevNode,nextNode,false);
      path.flags=CopyFlags(way);

      if (way.IsOneway()) {
        path.flags|=RouteNode::wrongDirectionOneway;
      }

      path.lat=way.nodes[prevNode].GetLat();
      path.lon=way.nodes[prevNode].GetLon();
      path.distance=distance;

      routeNode.paths.push_back(path);
    }
  }

  void RouteDataGenerator::CalculateWayPaths(RouteNode& routeNode,
                                             const Way& way,
                                             const NodeIdWayOffsetMap& nodeWayMap)
  {
    for (size_t i=0; i<way.nodes.size(); i++) {
      if (way.nodes[i].GetId()==routeNode.id) {
        if (i>0) {
          int j=i-1;

          while (j>=0) {
            if (nodeWayMap.find(way.nodes[j].GetId())!=nodeWayMap.end()) {
              break;
            }

            j--;
          }

          if (j>=0 &&
              way.nodes[j].GetId()!=routeNode.id) {
            RouteNode::Path path;

            path.id=way.nodes[j].GetId();
            path.wayIndex=routeNode.ways.size()-1;
            path.type=way.GetType();
            path.maxSpeed=way.GetMaxSpeed();
            path.grade=way.GetGrade();
            path.bearing=CalculateEncodedBearing(way,i,j,false);
            path.flags=CopyFlags(way);

            if (way.IsOneway()) {
              path.flags|=RouteNode::wrongDirectionOneway;
            }

            path.lat=way.nodes[j].GetLat();
            path.lon=way.nodes[j].GetLon();

            path.distance=0.0;
            for (size_t d=j;d<i; d++) {
              path.distance+=GetSphericalDistance(way.nodes[d].GetLon(),
                                                  way.nodes[d].GetLat(),
                                                  way.nodes[d+1].GetLon(),
                                                  way.nodes[d+1].GetLat());
            }

            routeNode.paths.push_back(path);
          }
        }

        if (i+1<way.nodes.size()) {
          size_t j=i+1;

          while (j<way.nodes.size()) {
            if (nodeWayMap.find(way.nodes[j].GetId())!=nodeWayMap.end()) {
              break;
            }

            j++;
          }

          if (j<way.nodes.size() &&
              way.nodes[j].GetId()!=routeNode.id) {
            RouteNode::Path path;

            path.id=way.nodes[j].GetId();
            path.wayIndex=routeNode.ways.size()-1;
            path.type=way.GetType();
            path.maxSpeed=way.GetMaxSpeed();
            path.grade=way.GetGrade();
            path.bearing=CalculateEncodedBearing(way,i,j,true);
            path.flags=CopyFlags(way);
            path.lat=way.nodes[j].GetLat();
            path.lon=way.nodes[j].GetLon();

            path.distance=0.0;
            for (size_t d=i;d<j; d++) {
              path.distance+=GetSphericalDistance(way.nodes[d].GetLon(),
                                                  way.nodes[d].GetLat(),
                                                  way.nodes[d+1].GetLon(),
                                                  way.nodes[d+1].GetLat());
            }

            routeNode.paths.push_back(path);
          }
        }
      }
    }
  }

  bool RouteDataGenerator::AddFileOffsetsToRouteNodes(const ImportParameter& parameter,
                                                      Progress& progress,
                                                      FileWriter& writer,
                                                      uint32_t routeNodeCount)
  {
    uint32_t   currentRouteNode=0;
    FileOffset nextOffset=0;

    while (currentRouteNode<routeNodeCount) {
      FileScanner                     scanner;
      OSMSCOUT_HASHMAP<Id,FileOffset> idOffsetMap;

      if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "route.dat"),
                        FileScanner::Sequential,
                        parameter.GetWayDataMemoryMaped())) {
        progress.Error("Cannot open '"+scanner.GetFilename()+"'");
        return false;
      }

      if (nextOffset==0) {
        uint32_t routeNodeCount;

        if (!scanner.Read(routeNodeCount)) {
          progress.Error("Error while reading number of entries in file");
          return false;
        }

        nextOffset=4;
      }
      else {
        if (!scanner.SetPos(nextOffset)) {
          progress.Error("Error while moving to way at offset " + NumberToString(nextOffset));
          return false;
        }
      }

      progress.SetAction("Loading up to " + NumberToString(1500000) + " route nodes");

      uint32_t blockCount=0;
      while (currentRouteNode<routeNodeCount &&
             blockCount<1500000) {
        RouteNode  node;

        currentRouteNode++;
        blockCount++;

        progress.SetProgress(blockCount,1500000);

        if (!node.Read(scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(currentRouteNode)+" of "+
                         NumberToString(routeNodeCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        idOffsetMap[node.GetId()]=nextOffset;

        if (!scanner.GetPos(nextOffset)) {
          progress.Error(std::string("Error while reading offset of data entry ")+
                         NumberToString(currentRouteNode)+" of "+
                         NumberToString(routeNodeCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }
      }

      if (!scanner.SetPos(4)) {
          progress.Error("Error while moving to way at offset " + NumberToString(4));
          return false;
      }

      progress.SetAction("Updating route nodes");

      for (size_t r=1; r<=routeNodeCount; r++) {
        progress.SetProgress(r,routeNodeCount);

        FileOffset offset;
        RouteNode  node;

        if (!scanner.GetPos(offset)) {
          progress.Error(std::string("Error while reading offset of data entry ")+
                         NumberToString(r)+" of "+
                         NumberToString(routeNodeCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        if (!node.Read(scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(currentRouteNode)+" of "+
                         NumberToString(routeNodeCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        bool hasChanged=false;

        for (size_t i=0; i<node.paths.size(); i++) {
          OSMSCOUT_HASHMAP<Id,FileOffset>::const_iterator offset=idOffsetMap.find(node.paths[i].id);

          if (offset!=idOffsetMap.end()) {
            node.paths[i].offset=offset->second;

            hasChanged=true;
          }
        }

        if (hasChanged) {
          if (!writer.SetPos(offset)) {
            progress.Error("Error while moving to way at offset " + NumberToString(offset));
            return false;
          }

          if (!node.Write(writer)) {
            progress.Error("Error while updating route node with id "+NumberToString(node.GetId()));
          }
        }
      }
    }

    return true;
  }

  bool RouteDataGenerator::Import(const ImportParameter& parameter,
                                  Progress& progress,
                                  const TypeConfig& typeConfig)
  {
    progress.SetAction("Generate route.dat");

    FileScanner                            scanner;
    FileWriter                             writer;

    // List of restrictions for a way
    std::map<Id,std::vector<TurnRestrictionData> > restrictions;

    uint32_t                               handledRouteNodeCount=0;
    uint32_t                               writtenRouteNodeCount=0;
    uint32_t                               writtenRoutePathCount=0;

    OSMSCOUT_HASHSET<Id>                   junctions;
    NodeIdWayOffsetMap                     nodeWayMap;

    //
    // Handling of restriction relations
    //

    progress.SetAction("Scanning for restriction relations");

    if (!ReadTurnRestrictions(parameter,
                              progress,
                              restrictions)) {
      return false;
    }

    //
    // Building a map of nodes and the number of ways that contain this way
    //

    progress.SetAction("Scanning for junctions");

    if (!ReadJunctions(parameter,
                       progress,
                       typeConfig,
                       junctions)) {
      return false;
    }

    progress.Info(NumberToString(junctions.size())+ " junctions found");

    //
    // Building a map of way endpoint ids and list of ways having this point as endpoint
    //

    progress.SetAction("Collecting ways intersecting junctions");

    if (!ReadWayEndpoints(parameter,
                          progress,
                          typeConfig,
                          junctions,
                          nodeWayMap)) {
      return false;
    }

    junctions.clear();

    progress.Info(NumberToString(nodeWayMap.size())+ " route nodes collected");

    //
    // Writing route nodes
    //

    progress.SetAction("Writing route nodes");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.dat"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open '"+scanner.GetFilename()+"'");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "route.dat"))) {
      progress.Error("Cannot create 'route.dat'");
      return false;
    }

    writer.Write(writtenRouteNodeCount);

    std::vector<NodeIdWayOffsetMap::iterator> block(parameter.GetRouteNodeBlockSize());

    NodeIdWayOffsetMap::iterator node=nodeWayMap.begin();
    while (node!=nodeWayMap.end()) {

      // Fill the current block of nodes to be processed

      size_t blockCount=0;

      progress.SetAction("Loading up to " + NumberToString(block.size()) + " route nodes");
      while (blockCount<block.size() &&
             node!=nodeWayMap.end()) {
        progress.SetProgress(writtenRouteNodeCount,nodeWayMap.size());

        block[blockCount]=node;

        blockCount++;
        node++;
      }

      progress.SetAction("Loading intersecting ways");

      // Collect way ids of all ways in current block and load them

      std::set<FileOffset> wayOffsets;

      for (size_t w=0; w<blockCount; w++) {
        for (std::list<FileOffset>::const_iterator wayOffset=block[w]->second.begin();
            wayOffset!=block[w]->second.end();
            wayOffset++) {
          wayOffsets.insert(*wayOffset);
        }
      }

      if (wayOffsets.empty()) {
        continue;
      }

      OSMSCOUT_HASHMAP<Id,WayRef> waysMap;

      if (!LoadWays(progress,
                    scanner,
                    wayOffsets,
                    waysMap)) {
        return false;
      }

      wayOffsets.clear();

      progress.SetAction("Storing route nodes");

      for (size_t w=0; w<blockCount; w++) {
        NodeIdWayOffsetMap::iterator node=block[w];
        RouteNode                    routeNode;

        routeNode.id=node->first;

        handledRouteNodeCount++;
        progress.SetProgress(handledRouteNodeCount,nodeWayMap.size());

        // We sort ways by increasing id, for more efficient storage
        // in route node
        node->second.sort();

        for (std::list<FileOffset>::const_iterator wayOffset=node->second.begin();
            wayOffset!=node->second.end();
            wayOffset++) {
          const WayRef& way=waysMap[*wayOffset];

          if (!way.Valid()) {
            progress.Error("Error while loading way at offset "+
                           NumberToString(*wayOffset) +
                           " (Internal error?)");
            continue;
          }

          routeNode.ways.push_back(*wayOffset);

          // Area routing
          if (way->IsArea()) {
            CalculateAreaPaths(routeNode,
                               *way,
                               nodeWayMap);
          }
          // Circular way routing (similar to current area routing, but respecting isOneway())
          else if (way->nodes.front().GetId()==way->nodes.back().GetId()) {
            CalculateCircularWayPaths(routeNode,
                                      *way,
                                      nodeWayMap);
          }
          // Normal way routing
          else {
            CalculateWayPaths(routeNode,
                              *way,
                              nodeWayMap);
          }
        }

        std::map<Id,std::vector<TurnRestrictionData> >::const_iterator turnConstraints=restrictions.find(routeNode.GetId());

        if (turnConstraints!=restrictions.end()) {
          for (std::list<FileOffset>::const_iterator sourceWayOffset=node->second.begin();
              sourceWayOffset!=node->second.end();
              sourceWayOffset++) {
            for (std::list<FileOffset>::const_iterator destWayOffset=node->second.begin();
                destWayOffset!=node->second.end();
                destWayOffset++) {
              if (*sourceWayOffset==*destWayOffset) {
                continue;
              }

              if (!CanTurn(turnConstraints->second,*sourceWayOffset,*destWayOffset)) {
                RouteNode::Exclude exclude;

                exclude.sourceWay=*sourceWayOffset;
                exclude.targetPath=0;

                while (exclude.targetPath<routeNode.paths.size() &&
                    routeNode.ways[routeNode.paths[exclude.targetPath].wayIndex]!=*destWayOffset) {
                  exclude.targetPath++;
                }

                if (exclude.targetPath<routeNode.paths.size()) {
                  routeNode.excludes.push_back(exclude);
                }
              }
            }
          }
        }

        if (!routeNode.Write(writer)) {
          return false;
        }

        writtenRouteNodeCount++;
        writtenRoutePathCount+=routeNode.paths.size();

        node++;
      }
    }

    // Cleaning up...

    nodeWayMap.clear();
    restrictions.clear();

    if (!scanner.Close()) {
      progress.Error("Cannot close file '"+scanner.GetFilename()+"'");
      return false;
    }

    writer.SetPos(0);
    writer.Write(writtenRouteNodeCount);

    progress.Info(NumberToString(writtenRouteNodeCount) + " route node(s) and " + NumberToString(writtenRoutePathCount)+ " paths written");

    progress.SetAction("Setting path offsets in route nodes");

    if (!AddFileOffsetsToRouteNodes(parameter,
                                    progress,
                                    writer,
                                    writtenRouteNodeCount)) {
      return false;
    }

    if (!writer.Close()) {
      return false;
    }

    return true;
  }
}

