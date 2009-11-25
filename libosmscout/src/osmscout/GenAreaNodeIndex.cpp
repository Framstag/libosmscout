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

#include <map>
#include <set>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Node.h>
#include <osmscout/Tiles.h>
#include <osmscout/Util.h>

bool GenerateAreaNodeIndex(const ImportParameter& parameter,
                           Progress& progress)
{
  //
  // Analysing nodes regarding draw type and matching tiles.
  //

  progress.SetAction("Analysing distribution");

  size_t                                         nodeIndexIntervalSize=parameter.GetNodeIndexIntervalSize();
  FileScanner                                    scanner;
  std::vector<size_t>                            drawTypeDist;
  std::vector<std::map<TileId,NodeCount> >       drawTypeTileNodeCount;
  std::vector<std::map<TileId,std::set<Page> > > drawTypeTilePages;
  size_t                                         nodeCount=0;

  scanner.Open("nodes.dat");

  if (scanner.HasError()) {
    progress.Error("Cannot open 'nodes.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    Node node;

    node.Read(scanner);

    if (!scanner.HasError()) {
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

  scanner.Close();

  progress.Info(std::string("Nodes scanned: ")+NumberToString(nodeCount));

  /*
  std::cout << "Distribution by type" << std::endl;
  for (size_t i=0; i<styleTypes.size(); i++) {
    if ((size_t)styleTypes[i].GetType()<drawTypeDist.size() &&
        drawTypeDist[styleTypes[i].GetType()]>0) {
      std::cout << styleTypes[i].GetType() << ": " << drawTypeDist[styleTypes[i].GetType()] << std::endl;
    }
  }*/

  progress.SetAction("Calculating statistics");

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

      for (std::map<TileId,std::set<Page> >::const_iterator tile=drawTypeTilePages[i].begin();
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

  progress.Info(std::string("Total number of draw types with tiles: ")+NumberToString(drawTypeSum));
  progress.Info(std::string("Total number of tiles: ")+NumberToString(tileSum));
  progress.Info(std::string("Total number of pages in tiles: ")+NumberToString(pageSum));
  progress.Info(std::string("Total number of nodes in tiles: ")+NumberToString(nodeSum));

  //
  // Writing index file
  //

  progress.SetAction("Generating 'areanode.idx'");

  FileWriter writer;

  if (!writer.Open("areanode.idx")) {
    progress.Error("Cannot create 'nodes.dat'");
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
