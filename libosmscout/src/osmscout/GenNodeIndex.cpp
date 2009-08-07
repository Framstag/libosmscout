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

#include <fstream>
#include <iostream>
#include <map>

#include <osmscout/FileWriter.h>
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

  FileWriter writer;
  size_t     intervalCount=offsetMap.size();

  if (!writer.Open("node.idx")) {
    return false;
  }

  writer.WriteNumber(intervalSize);   // Size of intervals
  writer.WriteNumber(intervalCount);  // Numbe rof intervals

  for (std::map<size_t,size_t>::const_iterator offset=offsetMap.begin();
       offset!=offsetMap.end();
       ++offset) {
    writer.WriteNumber(offset->first);  // The interval
    writer.WriteNumber(offset->second); // The offset into the file
    writer.WriteNumber(intervalDist[offset->first]); // The numbe rof nodes in the interval
  }

  return !writer.HasError() && writer.Close();
}

