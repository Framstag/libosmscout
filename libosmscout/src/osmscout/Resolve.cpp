/*
  Import/TravelJinni - Openstreetmap offline viewer
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

#include <osmscout/Resolve.h>

#include <fstream>
#include <iostream>
#include <map>

#include <osmscout/RawNode.h>
#include <osmscout/Way.h>

static size_t distributionGranuality =  1000000;
static size_t nodesLoadSize          = 1000000;

bool Resolve()
{
  std::cout << "Analysing distribution..." << std::endl;

  size_t              nodeCount=0;
  size_t              sum=0;
  std::vector<size_t> nodeDistribution;
  std::ifstream       in;

  in.open("rawnodes.dat",std::ios::in|std::ios::binary);

  if (!in) {
    return false;
  }

  while (in) {
    RawNode node;

    node.Read(in);

    if (in) {
      size_t index=node.id/distributionGranuality;

      if (index>=nodeDistribution.size()) {
        nodeDistribution.resize(index+1,0);
      }

      nodeDistribution[index]++;
      nodeCount++;
    }
  }

  in.close();

  std::cout << "Nodes: " << nodeCount << std::endl;
  for (size_t i=0; i<nodeDistribution.size(); i++) {
    sum+=nodeDistribution[i];
    //std::cout << "Nodes " << i*distributionGranuality << "-" << (i+1)*distributionGranuality << ": "  << nodeDistribution[i] << std::endl;
  }

  if (sum!=nodeCount) {
    std::cerr << "Number of nodes over all does not match sum over distribution (" << nodeCount << "!=" << sum << ")!" << std::endl;
    return false;
  }

  std::cout << "Loading nodes..." << std::endl;

  size_t index=0;

  while (index<nodeDistribution.size()) {
    size_t bucketSize=0;
    size_t newIndex=index;

    while (newIndex<nodeDistribution.size() &&
           bucketSize+nodeDistribution[newIndex]<nodesLoadSize) {
      bucketSize+=nodeDistribution[newIndex];
      newIndex++;
    }

    size_t start=index*distributionGranuality;
    size_t end=newIndex*distributionGranuality;

    std::cout << "Loading node id " << start << ">=id<" << end << "..." << std::endl;

    std::map<Id,RawNode> nodes;
    std::map<Id,uint8_t> nodeUses;

    in.open("rawnodes.dat",std::ios::in|std::ios::binary);

    if (!in) {
      return false;
    }

    for (size_t i=0; i<nodeCount; i++) {
      RawNode node;

      node.Read(in);

      if (!in)  {
        std::cerr << "Error while loading nodes!" << std::endl;
        in.close();
        return false;
      }

      if (node.id>=start && node.id<end) {
        nodes[node.id]=node;
        nodeUses[node.id]=0;
      }
    }

    in.close();

    if (bucketSize!=nodes.size()) {
      std::cerr << "Number of loaded nodes does not match expected number of nodes (" << nodes.size() << "!=" << bucketSize << ")!" << std::endl;
    }

    std::cout << "Nodes loaded: " << nodes.size() << std::endl;

    size_t wayCount=0;
    size_t nodesResolved=0;

    {
      std::cout << "Resolving way nodes..." << std::endl;

      in.open("ways.dat",std::ios::in|std::ios::out|std::ios::binary);
      std::ostream out(in.rdbuf());

      if (!in) {
        return false;
      }

      while (in) {
        Way    way;
        size_t oldPos=in.tellg();
        size_t newPos;
        bool   resolved=false;

        way.Read(in);

        if (!in) {
          continue;
        }

        newPos=in.tellg();

        for (size_t i=0; i<way.nodes.size(); i++) {
          std::map<Id,RawNode>::const_iterator node=nodes.find(way.nodes[i].id);

          if (node!=nodes.end()) {
            way.nodes[i].lat=node->second.lat;
            way.nodes[i].lon=node->second.lon;

            nodeUses[way.nodes[i].id]++;

            resolved=true;

            nodesResolved++;
          }
        }

        if (resolved) {
          out.seekp(oldPos);
          way.Write(out);
          in.seekg(newPos);
        }

        wayCount++;
      }

      in.close();

      std::cout << "Ways scanned: " << wayCount << std::endl;
    }

    std::cout << "Nodes resolved: " << nodesResolved << std::endl;

    std::cout << "Detecting way jointed ends..." << std::endl;

    {
      in.open("ways.dat",std::ios::in|std::ios::out|std::ios::binary);
      std::ostream out(in.rdbuf());

      if (!in) {
        return false;
      }

      while (in) {
        Way    way;
        size_t oldPos=in.tellg();
        size_t newPos;
        bool   resolved=false;

        way.Read(in);

        if (!in) {
          continue;
        }

        newPos=in.tellg();

        if (!way.IsArea()) {
          std::map<Id,uint8_t>::const_iterator nodeUse;

          nodeUse=nodeUses.find(way.nodes[0].id);

          if (nodeUse!=nodeUses.end() && nodeUse->second>=2) {
            way.flags|=Way::startIsJoint;
            resolved=true;
          }

          nodeUse=nodeUses.find(way.nodes[way.nodes.size()-1].id);

          if (nodeUse!=nodeUses.end() && nodeUse->second>=2) {
            way.flags|=Way::endIsJoint;
            resolved=true;
          }
        }

        if (resolved) {
          out.seekp(oldPos);
          way.Write(out);
          in.seekg(newPos);
        }
      }

      in.close();
    }

    index=newIndex;
  }

  return true;
}
