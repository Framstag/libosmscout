/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/GenWayDat.h>

#include <algorithm>
#include <cassert>

#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>
#include <osmscout/Util.h>
#include <osmscout/Way.h>

#include <osmscout/DataFile.h>
namespace osmscout {

  static size_t distributionGranuality =  100000;
  static size_t waysLoadSize           = 1000000;

  std::string WayDataGenerator::GetDescription() const
  {
    return "Generate 'ways.dat'";
  }

  bool WayDataGenerator::Import(const ImportParameter& parameter,
                                Progress& progress,
                                const TypeConfig& typeConfig)
  {
    //
    // Analysing distribution of nodes in the given interval size
    //

    progress.SetAction("Generate ways.dat");

    FileScanner                                 scanner;
    FileWriter                                  writer;
    uint32_t                                    rawNodeCount=0;
    uint32_t                                    rawRelCount=0;
    uint32_t                                    rawWayCount=0;
    TypeId                                      restrictionPosId;
    TypeId                                      restrictionNegId;
    std::map<Id,std::vector<Way::Restriction> > restrictions;
    DataFile<RawNode>                           nodeDataFile("rawnodes.dat","rawnode.idx",10);

    restrictionPosId=typeConfig.GetRelationTypeId(tagRestriction,"only_straight_on");
    assert(restrictionPosId!=typeIgnore);

    restrictionNegId=typeConfig.GetRelationTypeId(tagRestriction,"no_straight_on");
    assert(restrictionNegId!=typeIgnore);

    progress.SetAction("Scanning for restriction relations");

    if (!scanner.Open("rawrels.dat")) {
      progress.Error("Canot open 'rawrels.dat'");
      return false;
    }

    if (!scanner.Read(rawRelCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t r=1; r<=rawRelCount; r++) {
      progress.SetProgress(r,rawRelCount);

      RawRelation relation;

      if (!relation.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(rawRelCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (relation.type==restrictionPosId || relation.type==restrictionNegId) {
        Id               from=0;
        Way::Restriction restriction;

        restriction.members.resize(1,0);

        if (relation.type==restrictionPosId) {
          restriction.type=Way::rstrAllowTurn;
        }
        else if (relation.type==restrictionNegId) {
          restriction.type=Way::rstrForbitTurn;
        }
        else {
          continue;
        }

        for (size_t i=0; i<relation.members.size(); i++) {
          if (relation.members[i].type==RawRelation::memberWay &&
              relation.members[i].role=="from") {
            from=relation.members[i].id;
          }
          else if (relation.members[i].type==RawRelation::memberWay &&
                   relation.members[i].role=="to") {
            restriction.members[0]=relation.members[i].id;
          }
          else if (relation.members[i].type==RawRelation::memberNode &&
                   relation.members[i].role=="via") {
            restriction.members.push_back(relation.members[i].id);
          }
        }

        if (from!=0 &&
            restriction.members[1]!=0 &&
            restriction.members.size()>1) {
          restrictions[from].push_back(restriction);
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawrels.dat'");
      return false;
    }

    progress.Info(std::string("Found ")+NumberToString(restrictions.size())+" restrictions");

    if (!nodeDataFile.Open(".")) {
      std::cerr << "Cannot open raw nodes data file!" << std::endl;
      return false;
    }

    StopClock ac;
/*
    progress.Info("Resolving ways using rawnodes.idx");

    if (!scanner.Open("rawways.dat")) {
      progress.Error("Canot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=rawWayCount; w++) {
      progress.SetProgress(w,rawWayCount);

      RawWay way;

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(rawWayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (way.type!=typeIgnore) {
        std::vector<RawNode> nodes;

        if (!nodeDataFile.Get(way.nodes,nodes)) {
          std::cerr << "Cannot read nodes for way " << way.id << "!" << std::endl;
          return false;
        }
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }*/

    ac.Stop();

    //std::cout << ac <<  std::endl;

    StopClock bc;

    std::vector<size_t> wayDistribution;
    size_t              wayCount=0;
    size_t              sum=0;
    uint32_t            writtenWayCount=0;

    progress.SetAction("Analysing distribution");

    if (!scanner.Open("rawways.dat")) {
      progress.Error("Canot open 'rawways.dat'");
      return false;
    }

    if (!scanner.Read(rawWayCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (uint32_t w=1; w<=rawWayCount; w++) {
      progress.SetProgress(w,rawWayCount);

      RawWay way;

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(rawWayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      if (way.type!=typeIgnore) {
        size_t index=way.id/distributionGranuality;

        if (index>=wayDistribution.size()) {
          wayDistribution.resize(index+1,0);
        }

        wayDistribution[index]++;
        wayCount++;
      }
    }

    if (!scanner.Close()) {
      progress.Error("Cannot close file 'rawways.dat'");
      return false;
    }

    progress.Info(std::string("Ways: ")+NumberToString(wayCount));
    for (size_t i=0; i<wayDistribution.size(); i++) {
      sum+=wayDistribution[i];
      //std::cout << "Nodes " << i*distributionGranuality << "-" << (i+1)*distributionGranuality << ": "  << nodeDistribution[i] << std::endl;
    }

    if (sum!=wayCount) {
      progress.Error(std::string("Number of ways over all does not match sum over distribution (")+NumberToString(wayCount)+"!="+NumberToString(sum )+")!");
      return false;
    }

    if (!writer.Open("ways.dat")) {
      progress.Error("Canot create 'ways.dat'");
      return false;
    }

    writer.Write(writtenWayCount);

    size_t index=0;
    while (index<wayDistribution.size()) {
      size_t bucketSize=0;
      size_t newIndex=index;

      while (newIndex<wayDistribution.size() &&
             bucketSize+wayDistribution[newIndex]<waysLoadSize) {
        bucketSize+=wayDistribution[newIndex];
        newIndex++;
      }

      size_t start=index*distributionGranuality;
      size_t end=newIndex*distributionGranuality;

      progress.Info(std::string("Loading way id ")+NumberToString(start)+">=id<"+NumberToString(end)+" (interval "+NumberToString(index+1)+" of "+NumberToString(wayDistribution.size())+")");

      std::map<Id,RawWay>  ways;
      std::set<Id>         nodeIds;
      std::map<Id,uint8_t> nodeUses;

      if (!scanner.Open("rawways.dat")) {
        progress.Error("Canot open 'rawways.dat'");
        return false;
      }

      if (!scanner.Read(rawWayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t w=1; w<=rawWayCount; w++) {
        progress.SetProgress(w,rawWayCount);

        RawWay way;

        if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(w)+" of "+
                       NumberToString(rawWayCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
          return false;
        }

        if (way.type!=typeIgnore && way.id>=start && way.id<end) {
          ways[way.id]=way;

          for (size_t j=0; j<way.nodes.size(); j++) {
            nodeIds.insert(way.nodes[j]);
            nodeUses[way.nodes[j]]=0;
          }
        }

        if (way.id>end) {
          // ways are stored in increasing id order, so we can stop after we have reached
          // the last id for this interval.
          break;
        }
      }

      if (!scanner.Close()) {
        progress.Error("Cannot close file 'rawways.dat'");
        return false;
      }

      if (bucketSize!=ways.size()) {
        progress.Info(std::string("Number of loaded ways does not match expected number of ways (")+NumberToString(ways.size())+"!="+NumberToString(bucketSize)+")!");
      }

      progress.Info(std::string("Ways loaded: ")+NumberToString(ways.size()));

      progress.Info("Scanning for matching nodes");

      std::map<Id,RawNode> nodes;

      if (!scanner.Open("rawnodes.dat")) {
        progress.Error("Canot open 'rawnodes.dat'");
        return false;
      }

      if (!scanner.Read(rawNodeCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t n=1; n<=rawNodeCount; n++) {
        progress.SetProgress(n,rawNodeCount);

        RawNode node;

        if (!node.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(n)+" of "+
                       NumberToString(rawNodeCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
          return false;
        }

        if (nodeIds.find(node.id)!=nodeIds.end()) {
          nodes[node.id]=node;
        }
      }

      if (!scanner.Close()) {
        progress.Error("Cannot close file 'rawnodes.dat'");
        return false;
      }

      progress.Info("Scanning way node usage");

      if (!scanner.Open("rawways.dat")) {
        progress.Error("Canot open 'rawways.dat'");
        return false;
      }

      if (!scanner.Read(rawWayCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      for (uint32_t w=1; w<=rawWayCount; w++) {
        progress.SetProgress(w,rawWayCount);

        RawWay way;

        if (!way.Read(scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(w)+" of "+
                         NumberToString(rawWayCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        if (way.type!=typeIgnore) {
          for (size_t j=0; j<way.nodes.size(); j++) {
            std::map<Id,uint8_t>::iterator nodeUse=nodeUses.find(way.nodes[j]);

            if (nodeUse!=nodeUses.end()) {
              nodeUse->second++;
            }
          }
        }
      }

      if (!scanner.Close()) {
        progress.Error("Cannot close file 'rawways.dat'");
        return false;
      }

      progress.Info("Writing ways");

      for (std::map<Id,RawWay>::iterator w=ways.begin();
           w!=ways.end();
           ++w) {
        RawWay rawWay=w->second;
        Way    way;

        bool   reverseNodes=false;

        way.id=rawWay.id;

        if (!way.attributes.Assign(progress,
                                   rawWay.id,
                                   rawWay.type,
                                   rawWay.IsArea(),
                                   rawWay.tags,
                                   reverseNodes)) {
          continue;
        }

        // Nodes

        if (rawWay.nodes.size()<2) {
          progress.Error(std::string("Way ")+
                         NumberToString(rawWay.id)+" has only "+
                         NumberToString(rawWay.nodes.size())+
                         " node(s) but requires at least 2 nodes");
          continue;
        }

        way.nodes.resize(rawWay.nodes.size());
        for (size_t i=0; i<rawWay.nodes.size(); i++) {
          std::map<Id,RawNode>::iterator node=nodes.find(rawWay.nodes[i]);

          if (node==nodes.end()) {
            progress.Error(std::string("Cannot find node ")+
                           NumberToString(rawWay.nodes[i])+" for way "+
                           NumberToString(rawWay.id));
            continue;
          }

          way.nodes[i].id=rawWay.nodes[i];
          way.nodes[i].lat=node->second.lat;
          way.nodes[i].lon=node->second.lon;
        }

        if (reverseNodes) {
          std::reverse(way.nodes.begin(),way.nodes.end());
        }

        // startIsJoint/endIsJoint

        if (!way.IsArea()) {
          std::map<Id,uint8_t>::const_iterator nodeUse;

          nodeUse=nodeUses.find(way.nodes[0].id);

          if (nodeUse!=nodeUses.end() && nodeUse->second>=2) {
            way.attributes.flags|=SegmentAttributes::startIsJoint;
          }

          nodeUse=nodeUses.find(way.nodes[way.nodes.size()-1].id);

          if (nodeUse!=nodeUses.end() && nodeUse->second>=2) {
            way.attributes.flags|=SegmentAttributes::endIsJoint;
          }
        }

        // Restrictions

        std::map<Id,std::vector<Way::Restriction> >::iterator iter=restrictions.find(way.id);

        if (iter!=restrictions.end()) {
          way.attributes.flags|=SegmentAttributes::hasRestrictions;

          way.restrictions=iter->second;
        }

        way.Write(writer);
        writtenWayCount++;
      }

      index=newIndex;
    }


    writer.SetPos(0);
    writer.Write(writtenWayCount);

    if (!writer.Close()) {
      return false;
    }

    // Cleaning up...

    restrictions.clear();

    bc.Stop();

    //std::cout << ac << " <=> " << bc << std::endl;

    return true;
  }
}

