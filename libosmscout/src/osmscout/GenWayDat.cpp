/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscout/GenWayDat.h>

#include <algorithm>
#include <cassert>
#include <iostream>

#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>
#include <osmscout/Way.h>

static size_t distributionGranuality = 1000000;
static size_t waysLoadSize           = 1000000;

bool GenerateWayDat(const TypeConfig& typeConfig)
{
  //
  // Analysing distribution of nodes in the given interval size
  //

  std::cout << "Generate ways.dat..." << std::endl;

  FileScanner                                 scanner;
  FileWriter                                  writer;
  TypeId                                      restrictionPosId;
  TypeId                                      restrictionNegId;
  std::map<Id,std::vector<Way::Restriction> > restrictions;

  restrictionPosId=typeConfig.GetRelationTypeId(tagRestriction,"only_straight_on");
  assert(restrictionPosId!=typeIgnore);

  restrictionNegId=typeConfig.GetRelationTypeId(tagRestriction,"no_straight_on");
  assert(restrictionNegId!=typeIgnore);

  std::cout << "Scanning for restriction relations..." << std::endl;

  if (!scanner.Open("rawrels.dat")) {
    return false;
  }

  while (!scanner.HasError()) {
    RawRelation relation;

    relation.Read(scanner);

    if (!scanner.HasError()) {
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
  }

  scanner.Close();

  std::cout << "Found " << restrictions.size() << " restrictions." << std::endl;

  std::vector<size_t> wayDistribution;
  size_t              wayCount=0;
  size_t              sum=0;

  std::cout << "Analysing distribution..." << std::endl;

  if (!scanner.Open("rawways.dat")) {
    return false;
  }

  while (!scanner.HasError()) {
    RawWay way;

    way.Read(scanner);

    if (way.type!=typeIgnore) {
      size_t index=way.id/distributionGranuality;

      if (index>=wayDistribution.size()) {
        wayDistribution.resize(index+1,0);
      }

      wayDistribution[index]++;
      wayCount++;
    }
  }

  scanner.Close();

  std::cout << "Ways: " << wayCount << std::endl;
  for (size_t i=0; i<wayDistribution.size(); i++) {
    sum+=wayDistribution[i];
    //std::cout << "Nodes " << i*distributionGranuality << "-" << (i+1)*distributionGranuality << ": "  << nodeDistribution[i] << std::endl;
  }

  if (sum!=wayCount) {
    std::cerr << "Number of ways over all does not match sum over distribution (" << wayCount << "!=" << sum << ")!" << std::endl;
    return false;
  }

  if (!writer.Open("ways.dat")) {
    std::cerr << "Cannot create ways.dat" << std::endl;
    return false;
  }

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

    std::cout << "Loading way id " << start << ">=id<" << end << "..." << std::endl;

    std::map<Id,RawWay>  ways;
    std::set<Id>         nodeIds;
    std::map<Id,uint8_t> nodeUses;

    if (!scanner.Open("rawways.dat")) {
      return false;
    }

    while (!scanner.HasError()) {
      RawWay way;

      way.Read(scanner);

      if (way.type!=typeIgnore && way.id>=start && way.id<end) {
        ways[way.id]=way;

        for (size_t j=0; j<way.nodes.size(); j++) {
          nodeIds.insert(way.nodes[j]);
          nodeUses[way.nodes[j]]=0;
        }
      }
    }

    scanner.Close();

    if (bucketSize!=ways.size()) {
      std::cerr << "Number of loaded ways does not match expected number of ways (" << ways.size() << "!=" << bucketSize << ")!" << std::endl;
    }

    std::cout << "Ways loaded: " << ways.size() << std::endl;

    std::cout << "Scanning for matching nodes..." << std::endl;

    std::map<Id,RawNode> nodes;

    if (!scanner.Open("rawnodes.dat")) {
      return false;
    }

    while (!scanner.HasError()) {
      RawNode node;

      node.Read(scanner);

      if (nodeIds.find(node.id)!=nodeIds.end()) {
        nodes[node.id]=node;
      }
    }

    scanner.Close();

    std::cout << "Scanning way node usage..." << std::endl;

    if (!scanner.Open("rawways.dat")) {
      return false;
    }

    while (!scanner.HasError()) {
      RawWay way;

      way.Read(scanner);

      if (way.type!=typeIgnore) {
        for (size_t j=0; j<way.nodes.size(); j++) {
          std::map<Id,uint8_t>::iterator nodeUse=nodeUses.find(way.nodes[j]);

          if (nodeUse!=nodeUses.end()) {
            nodeUse->second++;
          }
        }
      }
    }

    scanner.Close();

    std::cout << "Writing ways..." << std::endl;

    for (std::map<Id,RawWay>::iterator w=ways.begin();
         w!=ways.end();
         ++w) {
      RawWay rawWay=w->second;
      Way    way;
      int8_t layer=0;
      bool   isBridge=false;
      bool   isTunnel=false;
      bool   isBuilding=false;
      bool   isOneway=false;
      bool   reverseNodes=false;

      way.id=rawWay.id;
      way.type=rawWay.type;

      std::vector<Tag>::iterator tag=rawWay.tags.begin();
      while (tag!=rawWay.tags.end()) {
        if (tag->key==tagName) {
          way.name=tag->value;
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagRef) {
          way.ref=tag->value;
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagLayer) {
          if (sscanf(tag->value.c_str(),"%hhd",&layer)!=1) {
            std::cerr << "Layer tag value '" << tag->value << "' for way " << rawWay.id << " is not numeric!" << std::endl;
          }
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagBridge) {
          isBridge=(tag->value=="yes" || tag->value=="true" || tag->value=="1") &&
                  !(tag->value=="no" || tag->value=="false" || tag->value=="0");
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagTunnel) {
          isTunnel=(tag->value=="yes" || tag->value=="true" || tag->value=="1") &&
                  !(tag->value=="no" || tag->value=="false" || tag->value=="0");
          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagBuilding) {
          isBuilding=(tag->value=="yes" || tag->value=="true" || tag->value=="1") &&
                     !(tag->value=="no" || tag->value=="false" || tag->value=="0");

          tag=rawWay.tags.erase(tag);
        }
        else if (tag->key==tagOneway) {
          if (tag->value=="-1") {
            isOneway=true;
            reverseNodes=true;
          }
          else {
            isOneway=(tag->value=="yes" || tag->value=="true" || tag->value=="1") &&
                    !(tag->value=="no" || tag->value=="false" || tag->value=="0");
          }

          tag=rawWay.tags.erase(tag);
        }
        else {
          ++tag;
        }
      }

      // Flags

      if (rawWay.IsArea()) {
        way.flags|=Way::isArea;
      }

      if (!way.name.empty()) {
        way.flags|=Way::hasName;
      }

      if (!way.ref.empty()) {
        way.flags|=Way::hasRef;
      }

      if (layer!=0) {
        way.flags|=Way::hasLayer;
        way.layer=layer;
      }

      if (isBridge) {
        way.flags|=Way::isBridge;
      }

      if (isTunnel) {
        way.flags|=Way::isTunnel;
      }

      if (isBuilding) {
        way.flags|=Way::isBuilding;
      }

      if (isOneway) {
        way.flags|=Way::isOneway;
      }

      // tags

      way.tags=rawWay.tags;
      if (way.tags.size()>0) {
        way.flags|=Way::hasTags;
      }

      // Nodes

      way.nodes.resize(rawWay.nodes.size());
      for (size_t i=0; i<rawWay.nodes.size(); i++) {
        std::map<Id,RawNode>::iterator node=nodes.find(rawWay.nodes[i]);

        assert(node!=nodes.end());

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
          way.flags|=Way::startIsJoint;
        }

        nodeUse=nodeUses.find(way.nodes[way.nodes.size()-1].id);

        if (nodeUse!=nodeUses.end() && nodeUse->second>=2) {
          way.flags|=Way::endIsJoint;
        }
      }

      // Restrictions

      std::map<Id,std::vector<Way::Restriction> >::iterator iter=restrictions.find(way.id);

      if (iter!=restrictions.end()) {
        way.flags|=Way::hasRestrictions;

        way.restrictions=iter->second;
      }

      way.Write(writer);
    }

    index=newIndex;
  }

  writer.Close();

  // Cleaning up...

  restrictions.clear();

  return true;
}

