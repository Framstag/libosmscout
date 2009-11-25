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

#include <map>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/RawNode.h>
#include <osmscout/Util.h>
#include <osmscout/Way.h>

static size_t distributionGranuality = 1000000;
static size_t nodesLoadSize          = 1000000;

bool GenerateNodeUseIndex(const TypeConfig& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress)
{
  progress.SetAction("Analysing distribution");

  std::set<TypeId>    types;
  size_t              nodeCount=0;
  std::vector<size_t> nodeDistribution;
  FileScanner         scanner;
  size_t              intervalSize=parameter.GetNodeIndexIntervalSize();

  typeConfig.GetRoutables(types);

  if (!scanner.Open("rawnodes.dat")) {
    return false;
  }

  while (!scanner.HasError()) {
    RawNode node;

    node.Read(scanner);

    if (!scanner.HasError()) {
      size_t index=node.id/distributionGranuality;

      if (index>=nodeDistribution.size()) {
        nodeDistribution.resize(index+1,0);
      }

      nodeDistribution[index]++;
      nodeCount++;
    }
  }

  scanner.Close();

  //std::cout << "Nodes: " << nodeCount << std::endl;

  progress.SetAction("Scanning node usage");

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

    progress.Info(std::string("Scanning for node ids ")+NumberToString(start)+">=id<"+NumberToString(end));

    if (!scanner.Open("ways.dat")) {
      progress.Error("Error while opening ways.dat!");
      return false;
    }

    while (!scanner.HasError()) {
      Way way;

      way.Read(scanner);

      if (!scanner.HasError())  {
        if (types.find(way.type)!=types.end()) {
          for (size_t i=0; i<way.nodes.size(); i++) {
            if (way.nodes[i].id>=start && way.nodes[i].id<end) {
              nodeWayMap[way.nodes[i].id].push_back(way.id);
            }
          }
        }
      }
    }

    scanner.Close();

    progress.Info(std::string("Resolving area/way references ")+NumberToString(start)+">=id<"+NumberToString(end));

    if (!scanner.Open("ways.dat")) {
      progress.Error("Error while opening ways.dat!");
      return false;
    }

    while (!scanner.HasError()) {
      Way way;

      way.Read(scanner);

      if (!scanner.HasError())  {
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

    scanner.Close();

    index=newIndex;
  }

  std::vector<size_t> resultDistribution; // Number of entries in interval
  std::vector<size_t> resultOffset;       // Offset for each interval
  std::vector<size_t> resultOffsetCount;  // Number of entries for each interval
  long                indexOffset;        // Start of the offset table in file
  FileWriter          writer;

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

  progress.SetAction("Writing 'nodeuse.idx'");

  if (!writer.Open("nodeuse.idx")) {
    progress.Error("Error while opening 'nodeuse.idx'!");
    return false;
  }

  size_t intervalCount=0;

  for (size_t i=0; i<resultDistribution.size(); i++) {
    if (resultDistribution[i]>0) {
      intervalCount++;
    }
  }

  writer.WriteNumber(intervalSize);  // The size of the interval
  writer.WriteNumber(intervalCount); // The number of intervals

  writer.GetPos(indexOffset);

  for (size_t i=0; i<intervalCount; i++) {
    size_t offset=0;      // place holder
    size_t offsetCount=0; // place holder

    writer.Write(i);           // The interval
    writer.Write(offset);      // The offset to the interval
    writer.Write(offsetCount); // The number of entries in the interval
  }

  for (size_t i=0; i<resultDistribution.size(); i++) {
    if (resultDistribution[i]==0) {
      continue;
    }

    size_t entryCount=0;
    long   offset;

    writer.GetPos(offset);
    resultOffset[i]=offset;

    for (std::map<Id, std::set<Id> >::const_iterator id=wayWayMap.lower_bound(i*intervalSize);
         id!=wayWayMap.end() && id->first<(i+1)*intervalSize;
         ++id) {
      writer.Write(id->first);          // The id of the way/area
      writer.WriteNumber(id->second.size());  // The number of references

      for (std::set<Id>::const_iterator ref=id->second.begin();
           ref!=id->second.end();
           ++ref) {
        writer.Write(*ref); // The id of the references
      }

      entryCount++;
    }

    resultOffsetCount[i]=entryCount;
  }

  writer.SetPos(indexOffset);

  for (size_t i=0; i<resultDistribution.size(); i++) {
    if (resultDistribution[i]==0) {
      continue;
    }

    writer.Write(i);                    // The interval (already written, but who cares)
    writer.Write(resultOffset[i]);      // offset for this interval
    writer.Write(resultOffsetCount[i]); // entry count for this interval
  }

  return !writer.HasError() && writer.Close();
}
