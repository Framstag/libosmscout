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

#include <osmscout/GenAreaWayIndex.h>

#include <map>
#include <set>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Tiles.h>
#include <osmscout/Util.h>
#include <osmscout/Way.h>

bool GenerateAreaWayIndex(const ImportParameter& parameter,
                                 Progress& progress)
{
  /*
  std::cout << "Tile -180 -90   : " << GetTileId(-180.0,-90.0) << std::endl;
  std::cout << "Tile -180 -89.91: " << GetTileId(-180.0,-89.91) << std::endl;
  std::cout << "Tile -180 -89.9 : " << GetTileId(-180.0,-89.9) << std::endl;
  std::cout << "Tile -180 -89.89: " << GetTileId(-180.0,-89.89) << std::endl;
  std::cout << "Tile -180 -89.85: " << GetTileId(-180.0,-89.85) << std::endl;
  std::cout << "Tile -180 -89.8 : " << GetTileId(-180.0,-89.8) << std::endl;
  std::cout << "Tile -180  89.9 : " << GetTileId(-180.0,89.9) << std::endl;
  std::cout << "Tile -180  89.8 : " << GetTileId(-180.0,89.8) << std::endl;
  std::cout << "Tile -180  89.85: " << GetTileId(-180.0,89.85) << std::endl;
  std::cout << "Tile -180  89.89: " << GetTileId(-180.0,89.89) << std::endl;
  std::cout << "Tile -180  89.91: " << GetTileId(-180.0,89.91) << std::endl;
  std::cout << "Tile  180  90   : " << GetTileId( 180.0,90.0) << std::endl;*/

  //
  // Analysing ways regarding draw type and matching tiles.
  //

  progress.SetAction("Analysing distribution");

  FileScanner                                   scanner;
  std::vector<size_t>                           drawTypeDist;
  std::vector<std::map<TileId,NodeCount > >     drawTypeTileNodeCount;
  std::vector<std::map<TileId,std::list<Id> > > drawTypeTileIds;
  size_t                                        wayCount=0;

  //
  // * Go through the list of ways
  // * For every way detect the tiles it covers (rectangular min-max region over nodes)
  // * For every type/tile combnation get the node index pages (node.id/wayIndexIntervalSize)
  //   that hold the matching ways for this type/tile combination
  //

  if (!scanner.Open("ways.dat")) {
    progress.Error("Cannot open 'ways.dat'");
    return false;
  }

  while (!scanner.HasError()) {
    Way way;

    way.Read(scanner);

    if (!scanner.HasError()) {
      if (way.IsArea()) {
        continue;
      }

      wayCount++;

      double xmin,xmax,ymin,ymax;

      xmin=way.nodes[0].lon;
      xmax=way.nodes[0].lon;
      ymin=way.nodes[0].lat;
      ymax=way.nodes[0].lat;

      for (size_t i=1; i<way.nodes.size(); i++) {
        xmin=std::min(xmin,way.nodes[i].lon);
        xmax=std::max(xmax,way.nodes[i].lon);
        ymin=std::min(ymin,way.nodes[i].lat);
        ymax=std::max(ymax,way.nodes[i].lat);
      }

      // By Type

      if ((size_t)way.type>=drawTypeDist.size()) {
        drawTypeDist.resize(way.type+1,0);
        drawTypeTileNodeCount.resize(way.type+1);
        drawTypeTileIds.resize(way.type+1);
      }
      drawTypeDist[way.type]++;

      // By Type and tile

      for (size_t y=GetTileY(ymin); y<=GetTileY(ymax); y++) {
        for (size_t x=GetTileX(xmin); x<=GetTileX(xmax); x++) {
          std::map<size_t,NodeCount>::iterator tile=drawTypeTileNodeCount[way.type].find(GetTileId(x,y));

          if (tile!=drawTypeTileNodeCount[way.type].end()) {
            drawTypeTileNodeCount[way.type][GetTileId(x,y)]+=way.nodes.size();
          }
          else {
            drawTypeTileNodeCount[way.type][GetTileId(x,y)]=way.nodes.size();
          }

          drawTypeTileIds[way.type][GetTileId(x,y)].push_back(way.id);
        }
      }
    }
  }

  scanner.Close();

  progress.Info(std::string("Ways scanned: ")+NumberToString(wayCount));
  /*
  std::cout << "Distribution by type" << std::endl;
  for (size_t i=0; i<styleTypes.size(); i++) {
    if ((size_t)styleTypes[i].GetType()<drawTypeDist.size() && drawTypeDist[styleTypes[i].GetType()]>0) {
      std::cout << styleTypes[i].GetType() << ": " << drawTypeDist[styleTypes[i].GetType()] << std::endl;
    }
  }*/

  progress.SetAction("Calculating statistics");

  size_t drawTypeSum=0;
  size_t tileSum=0;
  size_t nodeSum=0;
  size_t pageSum=0;

  //std::cout << "Number of tiles per type" << std::endl;
  for (size_t i=0; i<drawTypeTileIds.size(); i++) {
    if (i!=typeIgnore && drawTypeTileIds[i].size()>0) {

      drawTypeSum++;

      //std::cout << styleTypes[i].GetType() << ": " << drawTypeTileDist[styleTypes[i].GetType()].size();
      tileSum+=drawTypeTileIds[i].size();

      for (std::map<size_t,std::list<Id> >::const_iterator tile=drawTypeTileIds[i].begin();
           tile!=drawTypeTileIds[i].end();
           ++tile) {
        pageSum+=tile->second.size();
      }

      for (std::map<size_t,NodeCount>::const_iterator tile=drawTypeTileNodeCount[i].begin();
           tile!=drawTypeTileNodeCount[i].end();
           ++tile) {
        nodeSum+=tile->second;
      }

      //std::cout << " " << ways << std::endl;
    }
  }

  progress.Info(std::string("Total number of draw types with tiles: ")+NumberToString(drawTypeSum));
  progress.Info(std::string("Total number of tiles: ")+NumberToString(tileSum));
  progress.Info(std::string("Total number of pages in tiles: ")+NumberToString(pageSum));
  progress.Info(std::string("Total number of nodes in tiles: ")+NumberToString(nodeSum));

  //
  // Writing index file
  //

  progress.SetAction("Generating 'areaway.idx'");

  FileWriter writer;

  if (!writer.Open("areaway.idx")) {
    progress.Error("Cannot create 'areaway.idx'");
    return false;
  }

  // The number of draw types we have an index for
  writer.WriteNumber(drawTypeSum); // Number of entries

  for (TypeId i=0; i<drawTypeTileIds.size(); i++) {
    size_t tiles=drawTypeTileIds[i].size();

    if (i!=typeIgnore && tiles>0) {
      writer.WriteNumber(i);     // The draw type id
      writer.WriteNumber(tiles); // The number of tiles

      for (std::map<TileId,std::list<Id> >::const_iterator tile=drawTypeTileIds[i].begin();
           tile!=drawTypeTileIds[i].end();
           ++tile) {
        writer.WriteNumber(tile->first);                           // The tile id
        writer.WriteNumber(drawTypeTileNodeCount[i][tile->first]); // The number of nodes
        writer.WriteNumber(tile->second.size());                   // The number of pages

        for (std::list<Id>::const_iterator id=tile->second.begin();
             id!=tile->second.end();
             ++id) {
          writer.WriteNumber(*id); // The id of the node
        }
      }
    }
  }

  return !writer.HasError() && writer.Close();

  return true;
}
