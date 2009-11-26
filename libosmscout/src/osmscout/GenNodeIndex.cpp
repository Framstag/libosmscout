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

#include <osmscout/GenNodeIndex.h>

#include <iostream>
#include <map>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Node.h>
#include <osmscout/Tiles.h>
#include <osmscout/Util.h>

bool GenerateNodeIndex(const ImportParameter& parameter,
                       Progress& progress)
{
  //
  // Analysing distribution of nodes in the given interval size
  //

  progress.SetAction("Analysing distribution");

  size_t                     intervalSize=parameter.GetNodeIndexIntervalSize();
  FileScanner                scanner;
  size_t                     nodes=0;
  Id                         maxNodeId=0;
  std::map<size_t,NodeCount> intervalDist;
  std::map<size_t,size_t>    offsetMap;

  if (!scanner.Open("nodes.dat")) {
    progress.Error("Cannot open 'nodes.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    long pos;

    scanner.GetPos(pos);

    Node node;

    node.Read(scanner);

    if (!scanner.HasError()) {
      std::map<size_t,NodeCount>::iterator entry=intervalDist.find(node.id/intervalSize);

      if (entry!=intervalDist.end()) {
        intervalDist[node.id/intervalSize]++;
      }
      else {
        intervalDist[node.id/intervalSize]=1;
        offsetMap[node.id/intervalSize]=pos;
      }

      maxNodeId=std::max(maxNodeId,node.id);
      nodes++;
    }
  }

  scanner.Close();

  progress.Info(NumberToString(nodes)+" nodes to be indexed");
  progress.Info(std::string("Max. node id: ")+NumberToString(maxNodeId));
  progress.Info(NumberToString(intervalDist.size())+" intervals filled");

  size_t levels=1;
  size_t tmp;

  tmp=nodes;
  while (tmp/parameter.GetIndexPageSize()>0) {
    tmp=tmp/parameter.GetIndexPageSize();
    levels++;
  }

  //std::cout << "We have " << levels << " level(s)" << " for indexing " << nodes << " entries" << std::endl;

  //
  // Writing index file
  //

  progress.SetAction("Generating 'node.idx'");

  FileWriter writer;
  size_t     intervalCount=offsetMap.size();

  if (!writer.Open("node.idx")) {
    progress.Error("Cannot create 'node.idx'");
    return false;
  }

  writer.WriteNumber(intervalSize);   // Size of intervals
  writer.WriteNumber(intervalCount);  // Number of intervals

  for (std::map<size_t,size_t>::const_iterator offset=offsetMap.begin();
       offset!=offsetMap.end();
       ++offset) {
    writer.WriteNumber(offset->first);  // The interval
    writer.WriteNumber(offset->second); // The offset into the file
    writer.WriteNumber(intervalDist[offset->first]); // The number of nodes in the interval
  }

  if (writer.HasError() || !writer.Close()) {
    return false;
  }

  //
  // Writing index file
  //

  progress.SetAction("Generating 'node2.idx'");

  if (!writer.Open("node2.idx")) {
    progress.Error("Cannot create 'node2.idx'");
    return false;
  }

  if (!scanner.Open("nodes.dat")) {
    progress.Error("Cannot open 'nodes.dat'");
    return false;
  }

  std::vector<Id>            startingIds;
  std::vector<unsigned long> pageStarts;
  long                       lastLevelPageStart;

  writer.WriteNumber(levels); // Number of levels
  writer.Write(nodes);        // Number of nodes

  writer.GetPos(lastLevelPageStart);

  writer.Write((unsigned long)0); // Write the starting position of the last page

  size_t currentEntry=0;
  long   pageOffset;
  long   idOffset;

  writer.GetPos(pageOffset);

  while (!scanner.HasError()) {
    long pos;

    scanner.GetPos(pos);

    Node node;

    node.Read(scanner);

    if (!scanner.HasError()) {
      if (currentEntry%parameter.GetIndexPageSize()==0) {
        long pageStart;

        writer.GetPos(pageStart);
        pageOffset=pos;
        idOffset=node.id;

        //std::cout << levels << " " << node.id << " " << pageOffset << std::endl;
        writer.WriteNumber((unsigned long)pageOffset);
        writer.WriteNumber(node.id);
        startingIds.push_back(node.id);
        pageStarts.push_back(pageStart);
      }

      //std::cout << node.id-idOffset << " - " << pos-pageOffset << std::endl;
      writer.WriteNumber((unsigned long)(pos-pageOffset));
      writer.WriteNumber(node.id-idOffset);

      currentEntry++;
    }
  }

  levels--;

  while (levels>0) {
    std::vector<Id>            si(startingIds);
    std::vector<unsigned long> po(pageStarts);

    startingIds.clear();
    pageStarts.clear();

    for (size_t i=0; i<si.size(); i++) {
      //std::cout << levels << " " << si[i] << " - " << po[i] << std::endl;

      if (i%parameter.GetIndexPageSize()==0) {
        long pos;

        writer.GetPos(pos);

        startingIds.push_back(si[i]);
        pageStarts.push_back(pos);
      }

      if (i==0) {
        writer.WriteNumber(0);
        writer.WriteNumber(0);
      }
      else {
        writer.WriteNumber(si[i]-si[i-1]);
        writer.WriteNumber(po[i]-po[i-1]);
      }
    }

    levels--;
  }

  writer.SetPos(lastLevelPageStart);

  writer.Write((unsigned long)pageStarts[0]);

  scanner.Close();

  return !writer.HasError() && writer.Close();
}

