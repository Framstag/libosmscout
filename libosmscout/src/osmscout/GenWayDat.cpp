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
#include <osmscout/Way.h>

#include <osmscout/Util.h>

#include <osmscout/util/StopClock.h>
#include <osmscout/util/String.h>

namespace osmscout {

  static size_t distributionGranuality =  100000;

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
    TypeId                                      restrictionPosId=typeConfig.GetRelationTypeId("restriction_only_straight_on");
    TypeId                                      restrictionNegId=typeConfig.GetRelationTypeId("restriction_no_straight_on");
    std::map<Id,std::vector<Way::Restriction> > restrictions;
    /*
    DataFile<RawNode>                           nodeDataFile("rawnodes.dat",
                                                             "rawnode.idx",
                                                             10,
                                                             100000);*/

    assert(restrictionPosId!=typeIgnore);
    assert(restrictionNegId!=typeIgnore);

    progress.SetAction("Scanning for restriction relations");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawrels.dat"))) {
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

      if (relation.GetType()==restrictionPosId ||
          relation.GetType()==restrictionNegId) {
        Id               from=0;
        Way::Restriction restriction;

        restriction.members.resize(1,0);

        if (relation.GetType()==restrictionPosId) {
          restriction.type=Way::rstrAllowTurn;
        }
        else if (relation.GetType()==restrictionNegId) {
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

/*
    if (!nodeDataFile.Open(".")) {
      std::cerr << "Cannot open raw nodes data file!" << std::endl;
      return false;
    }

    StopClock ac;

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
    }

    ac.Stop();

    //std::cout << ac <<  std::endl;*/

    StopClock bc;

    std::vector<size_t> wayDistribution;
    size_t              wayCount=0;
    size_t              sum=0;
    uint32_t            writtenWayCount=0;

    progress.SetAction("Analysing distribution");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawways.dat"))) {
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

      if (way.GetType()!=typeIgnore) {
        size_t index=way.GetId()/distributionGranuality;

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

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "ways.dat"))) {
      progress.Error("Canot create 'ways.dat'");
      return false;
    }

    writer.Write(writtenWayCount);

    size_t index=0;
    while (index<wayDistribution.size()) {
      size_t bucketSize=0;
      size_t newIndex=index;

      while (newIndex<wayDistribution.size() &&
             bucketSize+wayDistribution[newIndex]<parameter.GetWaysLoadSize()) {
        bucketSize+=wayDistribution[newIndex];
        newIndex++;
      }

      size_t start=index*distributionGranuality;
      size_t end=newIndex*distributionGranuality;

      progress.Info(std::string("Loading way id ")+NumberToString(start)+">=id<"+NumberToString(end)+" (interval "+NumberToString(index+1)+" of "+NumberToString(wayDistribution.size())+")");

      std::map<Id,RawWay>  ways;
      std::set<Id>         nodeIds;
      std::map<Id,uint8_t> nodeUses;

      if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawways.dat"))) {
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

        if (way.GetType()!=typeIgnore &&
            way.GetId()>=start &&
            way.GetId()<end) {
          ways[way.GetId()]=way;

          for (size_t j=0; j<way.GetNodeCount(); j++) {
            nodeIds.insert(way.GetNodeId(j));
            nodeUses[way.GetNodeId(j)]=0;
          }
        }

        if (way.GetId()>end) {
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

      if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawnodes.dat"))) {
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

        if (nodeIds.find(node.GetId())!=nodeIds.end()) {
          nodes[node.GetId()]=node;
        }
      }

      if (!scanner.Close()) {
        progress.Error("Cannot close file 'rawnodes.dat'");
        return false;
      }

      progress.Info("Scanning way node usage");

      if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawways.dat"))) {
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

        if (way.GetType()!=typeIgnore) {
          for (size_t j=0; j<way.GetNodeCount(); j++) {
            std::map<Id,uint8_t>::iterator nodeUse=nodeUses.find(way.GetNodeId(j));

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
        RawWay           rawWay=w->second;
        std::vector<Tag> tags(rawWay.GetTags());
        Way              way;
        bool             reverseNodes=false;
        bool             error=false;

        way.SetId(rawWay.GetId());
        way.SetType(rawWay.GetType());

        if (!way.SetTags(progress,
                         typeConfig,
                         rawWay.IsArea(),
                         tags,
                         reverseNodes)) {
          continue;
        }

        // Nodes

        if (rawWay.GetNodeCount()<2) {
          progress.Error(std::string("Way ")+
                         NumberToString(rawWay.GetId())+" has only "+
                         NumberToString(rawWay.GetNodeCount())+
                         " node(s) but requires at least 2 nodes");
          continue;
        }

        way.nodes.resize(rawWay.GetNodeCount());
        for (size_t i=0; i<rawWay.GetNodeCount() && !error; i++) {
          std::map<Id,RawNode>::iterator node=nodes.find(rawWay.GetNodeId(i));

          if (node==nodes.end()) {
            progress.Error(std::string("Cannot find node ")+
                           NumberToString(rawWay.GetNodeId(i))+" for way "+
                           NumberToString(rawWay.GetId()));
            error=true;
            continue;
          }

          way.nodes[i].id=rawWay.GetNodeId(i);
          way.nodes[i].lat=node->second.GetLat();
          way.nodes[i].lon=node->second.GetLon();
        }

        if (error) {
          continue;
        }

        if (reverseNodes) {
          std::reverse(way.nodes.begin(),way.nodes.end());
        }

        // startIsJoint/endIsJoint

        if (!way.IsArea()) {
          std::map<Id,uint8_t>::const_iterator nodeUse;

          nodeUse=nodeUses.find(way.nodes[0].id);

          way.SetStartIsJoint(nodeUse!=nodeUses.end() && nodeUse->second>=2);

          nodeUse=nodeUses.find(way.nodes[way.nodes.size()-1].id);

          way.SetEndIsJoint(nodeUse!=nodeUses.end() && nodeUse->second>=2);
        }

        // Restrictions

        std::map<Id,std::vector<Way::Restriction> >::iterator iter=restrictions.find(way.GetId());

        if (iter!=restrictions.end()) {
          way.SetRestrictions(iter->second);
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

