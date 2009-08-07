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

#include <osmscout/GenAreaNodeIndex.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include <osmscout/FileWriter.h>
#include <osmscout/Node.h>
#include <osmscout/Tiles.h>

bool GenerateAreaNodeIndex(size_t nodeIndexIntervalSize)
{
  //
  // Analysing nodes regarding draw type and matching tiles.
  //

  std::cout << "Analysing distribution..." << std::endl;

  std::ifstream                                  in;
  std::vector<size_t>                            drawTypeDist;
  std::vector<std::map<TileId,NodeCount> >       drawTypeTileNodeCount;
  std::vector<std::map<TileId,std::set<Page> > > drawTypeTilePages;
  size_t                                         nodeCount=0;

  in.open("nodes.dat",std::ios::in|std::ios::out|std::ios::binary);
  std::ostream out(in.rdbuf());

  if (!in) {
    return false;
  }

  while (in) {
    Node node;

    node.Read(in);

    if (in) {
      TileId tileId=GetTileId(node.lon,node.lat);

      nodeCount++;

      // By Type

      if ((size_t)node.type>=drawTypeDist.size()) {
        drawTypeDist.resize(node.type+1,0);
        drawTypeTileNodeCount.resize(node.type+1);
        drawTypeTilePages.resize(node.type+1);
      }
      drawTypeDist[node.type]++;

      // By Type and tile

      if (drawTypeTileNodeCount[node.type].find(tileId)==drawTypeTileNodeCount[node.type].end()) {
        drawTypeTileNodeCount[node.type][tileId]=1;
      }
      else {
        drawTypeTileNodeCount[node.type][tileId]++;
      }

      drawTypeTilePages[node.type][tileId].insert(node.id/nodeIndexIntervalSize);
    }
  }

  in.close();

  std::cout << "Nodes scanned: " << nodeCount << std::endl;

  /*
  std::cout << "Distribution by type" << std::endl;
  for (size_t i=0; i<styleTypes.size(); i++) {
    if ((size_t)styleTypes[i].GetType()<drawTypeDist.size() &&
        drawTypeDist[styleTypes[i].GetType()]>0) {
      std::cout << styleTypes[i].GetType() << ": " << drawTypeDist[styleTypes[i].GetType()] << std::endl;
    }
  }*/

  size_t drawTypeSum=0;
  size_t tileSum=0;
  size_t nodeSum=0;
  size_t pageSum=0;

  //std::cout << "Number of tiles per type" << std::endl;
  for (size_t i=0; i<drawTypeTilePages.size(); i++) {
    if (i!=typeIgnore && drawTypeTilePages[i].size()>0) {

      drawTypeSum++;

      //std::cout << styleTypes[i].GetType() << ": " << drawTypeTileDist[styleTypes[i].GetType()].size();
      tileSum+=drawTypeTilePages[i].size();

      for (std::map<size_t,std::set<Page> >::const_iterator tile=drawTypeTilePages[i].begin();
           tile!=drawTypeTilePages[i].end();
           ++tile) {
        pageSum+=tile->second.size();
      }

      for (std::map<size_t,NodeCount>::const_iterator tile=drawTypeTileNodeCount[i].begin();
           tile!=drawTypeTileNodeCount[i].end();
           ++tile) {
        nodeSum+=tile->second;
      }

      //std::cout << " " << nodes << std::endl;
    }
  }

  std::cout << "Total number of draw types with tiles: " << drawTypeSum << std::endl;
  std::cout << "Total number of tiles: " << tileSum << std::endl;
  std::cout << "Total number of pages in tiles: " << pageSum << std::endl;
  std::cout << "Total number of nodes in tiles: " << nodeSum << std::endl;

  //
  // Writing index file
  //

  FileWriter writer;

  if (!writer.Open("areanode.idx")) {
    return false;
  }

  // The number of draw types we have an index for
  writer.WriteNumber(drawTypeSum); // Number of entries

  for (TypeId i=0; i<drawTypeTilePages.size(); i++) {
    if (i!=typeIgnore && drawTypeTilePages[i].size()>0) {
      writer.WriteNumber(i);                           // The draw type id
      writer.WriteNumber(drawTypeTilePages[i].size()); // The number of tiles

      for (std::map<TileId,std::set<Page> >::const_iterator tile=drawTypeTilePages[i].begin();
           tile!=drawTypeTilePages[i].end();
           ++tile) {
        writer.WriteNumber(tile->first);                           // The tile id
        writer.WriteNumber(drawTypeTileNodeCount[i][tile->first]); // The number of nodes
        writer.WriteNumber(tile->second.size());                   // The number of pages

        for (std::set<Page>::const_iterator page=tile->second.begin();
             page!=tile->second.end();
             ++page) {
          Page p=*page;

          writer.WriteNumber(p); // The id of the page
        }
      }
    }
  }

  return !writer.HasError() && writer.Close();
}
