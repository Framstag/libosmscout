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

#include <cmath>
#include <fstream>
#include <iostream>
#include <map>

#include <osmscout/Node.h>
#include <osmscout/Tiles.h>

bool GenerateNodeIndex(size_t intervalSize)
{
  //
  // Analysing distribution of nodes in the given interval size
  //

  std::cout << "Analysing distribution..." << std::endl;

  std::ifstream              in;
  std::map<size_t,NodeCount> intervalDist;
  std::map<size_t,size_t>    offsetMap;

  in.open("nodes.dat",std::ios::in|std::ios::binary);

  if (!in) {
    return false;
  }

  while (in) {
    size_t pos=in.tellg();
    Node node;

    node.Read(in);

    if (in) {
      std::map<size_t,NodeCount>::iterator entry=intervalDist.find(node.id/intervalSize);

      if (entry!=intervalDist.end()) {
        intervalDist[node.id/intervalSize]++;
      }
      else {
        intervalDist[node.id/intervalSize]=1;
        offsetMap[node.id/intervalSize]=pos;
      }
    }
  }

  in.close();

  /*
  std::cout << "Distribution:" << std::endl;

  for (std::map<size_t,size_t>::const_iterator interval=intervalDist.begin();
       interval!=intervalDist.end();
       ++interval) {
    std::cout << interval->first << ": " << interval->second << std::endl;
  }

  std::cout << intervalDist.size() << " intervals filled" << std::endl;*/

  //
  // Writing index file
  //

  std::ofstream indexFile;
  size_t        intervalCount=offsetMap.size();

  indexFile.open("node.idx",std::ios::out|std::ios::trunc|std::ios::binary);

  if (!indexFile) {
    return false;
  }

  indexFile.write((const char*)&intervalSize,sizeof(intervalSize)); // The size of the interval
  indexFile.write((const char*)&intervalCount,sizeof(intervalCount)); // The number of intervals we have

  for (std::map<size_t,size_t>::const_iterator offset=offsetMap.begin();
       offset!=offsetMap.end();
       ++offset) {
    NodeCount count=intervalDist[offset->first];

    indexFile.write((const char*)&offset->first,sizeof(offset->first)); // The interval
    indexFile.write((const char*)&offset->second,sizeof(offset->second)); // The offset
    indexFile.write((const char*)&count,sizeof(count)); // The number of nodes
  }

  indexFile.close();

  return true;
}

