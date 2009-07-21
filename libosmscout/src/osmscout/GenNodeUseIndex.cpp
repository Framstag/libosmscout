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

#include <osmscout/GenNodeUseIndex.h>

#include <fstream>
#include <iostream>
#include <map>

#include <osmscout/RawNode.h>
#include <osmscout/Way.h>

static size_t distributionGranuality =  1000000;
static size_t nodesLoadSize          = 1000000;

bool GenerateNodeUseIndex(const TypeConfig& typeConfig, size_t intervalSize)
{
  std::cout << "Analysing distribution..." << std::endl;

  std::set<TypeId>    types;
  size_t              nodeCount=0;
  std::vector<size_t> nodeDistribution;
  std::ifstream       in;
  std::ofstream       out;

  typeConfig.GetRoutables(types);

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

  std::cout << "Scanning node usage..." << std::endl;

  std::map<Id,std::set<Id> > wayWayMap;
  size_t                     index=0;

  while (index<nodeDistribution.size()) {
    size_t bucketSize=0;
    size_t newIndex=index;

    while (newIndex<nodeDistribution.size() &&
           bucketSize+nodeDistribution[newIndex]<nodesLoadSize) {
      bucketSize+=nodeDistribution[newIndex];
      newIndex++;
    }

    size_t                      start=index*distributionGranuality;
    size_t                      end=newIndex*distributionGranuality;
    std::map<Id,std::list<Id> > nodeWayMap;

    std::cout << "Scanning for node ids " << start << ">=id<" << end << "..." << std::endl;

    std::cout << "Scanning areas/ways..." << std::endl;

    in.open("ways.dat",std::ios::in|std::ios::binary);

    if (!in) {
      std::cerr << "Error while opening ways.dat!" << std::endl;
      return false;
    }

    while (in) {
      Way way;

      way.Read(in);

      if (in)  {
        if (types.find(way.type)!=types.end()) {
          for (size_t i=0; i<way.nodes.size(); i++) {
            if (way.nodes[i].id>=start && way.nodes[i].id<end) {
              nodeWayMap[way.nodes[i].id].push_back(way.id);
            }
          }
        }
      }
    }

    in.close();

    std::cout << "Resolving area/way references " << start << ">=id<" << end << "..." << std::endl;

    std::cout << "Scanning areas/ways..." << std::endl;

    in.open("ways.dat",std::ios::in|std::ios::binary);

    if (!in) {
      std::cerr << "Error while opening ways.dat!" << std::endl;
      return false;
    }

    while (in) {
      Way way;

      way.Read(in);

      if (in)  {
        if (types.find(way.type)!=types.end()) {
          for (size_t i=0; i<way.nodes.size(); i++) {
            if (way.nodes[i].id>=start && way.nodes[i].id<end) {
              std::map<Id, std::list<Id> >::const_iterator ways;

              ways=nodeWayMap.find(way.nodes[i].id);

              if (ways!=nodeWayMap.end()) {
                for (std::list<Id>::const_iterator w=ways->second.begin();
                     w!=ways->second.end();
                     ++w) {
                  if (*w!=way.id) {
                    // TODO: Optimize performance
                    wayWayMap[way.id].insert(*w);
                  }
                }
              }
            }
          }
        }
      }
    }

    in.close();

    index=newIndex;
  }

  std::vector<size_t> resultDistribution; // Number of entries in interval
  std::vector<size_t> resultOffset;       // Offset for each interval
  std::vector<size_t> resultOffsetCount;  // Number of entries for each interval
  size_t              indexOffset;        // Start of the offset table in file

  out.close();

  for (std::map<Id, std::set<Id> >::const_iterator res=wayWayMap.begin();
       res!=wayWayMap.end();
       ++res) {
    size_t index=res->first/intervalSize;

    if (index>=resultDistribution.size()) {
      resultDistribution.resize(index+1,0);
    }

    resultDistribution[index]++;
  }

  resultOffset.resize(resultDistribution.size(),0);
  resultOffsetCount.resize(resultDistribution.size(),0);

  std::cout << "Writing 'nodeuse.idx'..." << std::endl;

  out.open("nodeuse.idx",std::ios::out|std::ios::trunc|std::ios::binary);

  if (!out) {
    std::cerr<< "Error while opening to 'nodeuse.idx'!" << std::endl;
    return false;
  }

  size_t intervalCount=0;

  for (size_t i=0; i<resultDistribution.size(); i++) {
    if (resultDistribution[i]>0) {
      intervalCount++;
    }
  }

  out.write((const char*)&intervalSize,sizeof(intervalSize)); // The size of the interval
  out.write((const char*)&intervalCount,sizeof(intervalCount)); // The number of intervals we have

  indexOffset=out.tellp();

  for (size_t i=0; i<intervalCount; i++) {
    size_t offset=0;
    size_t offsetCount=0;

    out.write((const char*)&i,sizeof(i)); // The interval
    out.write((const char*)&offset,sizeof(offset)); // The offset to the interval
    out.write((const char*)&offsetCount,sizeof(offsetCount)); // The number of entries in the interval
  }

  for (size_t i=0; i<resultDistribution.size(); i++) {
    if (resultDistribution[i]>0) {
      size_t entryCount=0;

      resultOffset[i]=out.tellp();

      for (std::map<Id, std::set<Id> >::const_iterator id=wayWayMap.lower_bound(i*intervalSize);
           id!=wayWayMap.end() && id->first<(i+1)*intervalSize;
           ++id) {
        size_t refCount=id->second.size();

        out.write((const char*)&id->first,sizeof(id->first)); // The id of the way/area
        out.write((const char*)&refCount,sizeof(refCount));   // The number of references

        for (std::set<Id>::const_iterator ref=id->second.begin();
             ref!=id->second.end();
             ++ref) {
          Id i=*ref;
          out.write((const char*)&i,sizeof(i));   // The id of the references
        }

        entryCount++;
      }

      resultOffsetCount[i]=entryCount;
    }
  }

  out.seekp(indexOffset);

  for (size_t i=0; i<resultDistribution.size(); i++) {
    if (resultDistribution[i]>0) {
      size_t offset=resultOffset[i];
      size_t count=resultOffsetCount[i];

      out.write((const char*)&i,sizeof(i));           // The interval (already written, but who caes
      out.write((const char*)&offset,sizeof(offset)); // offset for this interval
      out.write((const char*)&count,sizeof(count));   // offset for this interval
    }
  }

  if (!out) {
    std::cerr<< "Error while writing to 'nodeuse.idx'!" << std::endl;
    return false;
  }

  out.close();

  return true;
}
