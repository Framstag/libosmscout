/*
  This source is part of the libosmscout library
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

#include <osmscout/GenAreaNodeIndex.h>

#include <map>
#include <set>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Node.h>
#include <osmscout/Tiles.h>
#include <osmscout/Util.h>

namespace osmscout {

  bool GenerateAreaNodeIndex(const ImportParameter& parameter,
                             Progress& progress)
  {
    //
    // Analysing nodes regarding draw type and matching tiles.
    //

    progress.SetAction("Analysing distribution");

    FileScanner                                    scanner;
    std::vector<size_t>                            drawTypeDist;
    std::vector<std::map<TileId,std::list<Id> > >  drawTypeTileIds;
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

        if ((size_t)node.type>=drawTypeDist.size()) {
          drawTypeDist.resize(node.type+1,0);
          drawTypeTileIds.resize(node.type+1);
        }

        // Node count by draw type
        drawTypeDist[node.type]++;

        // Node ids by Type and tile
        drawTypeTileIds[node.type][tileId].push_back(node.id);
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

    //std::cout << "Number of tiles per type" << std::endl;

    // For every draw type...
    for (size_t i=0; i<drawTypeTileIds.size(); i++) {
      // if the draw types is used...
      if (i!=typeIgnore && drawTypeTileIds[i].size()>0) {

        drawTypeSum++;

        //std::cout << styleTypes[i].GetType() << ": " << drawTypeTileDist[styleTypes[i].GetType()].size();
        tileSum+=drawTypeTileIds[i].size();

        for (std::map<TileId,std::list<Id> >::const_iterator tile=drawTypeTileIds[i].begin();
             tile!=drawTypeTileIds[i].end();
             ++tile) {
          nodeSum+=tile->second.size();
        }
        //std::cout << " " << nodes << std::endl;
      }
    }

    progress.Info(std::string("Total number of draw types with tiles: ")+NumberToString(drawTypeSum));
    progress.Info(std::string("Total number of tiles: ")+NumberToString(tileSum));
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

    for (TypeId i=0; i<drawTypeTileIds.size(); i++) {
      if (i!=typeIgnore && drawTypeTileIds[i].size()>0) {
        writer.WriteNumber(i);                           // The draw type id
        writer.WriteNumber(drawTypeTileIds[i].size()); // The number of tiles

        for (std::map<TileId,std::list<Id> >::const_iterator tile=drawTypeTileIds[i].begin();
             tile!=drawTypeTileIds[i].end();
             ++tile) {
          writer.WriteNumber(tile->first);                           // The tile id
          writer.WriteNumber(tile->second.size());                   // The number of nodes

          // List of node ids in tile with given draw type...
          for (std::list<Id>::const_iterator id=tile->second.begin();
               id!=tile->second.end();
               ++id) {
            writer.WriteNumber(*id); // The id of the node
          }
        }
      }
    }

    return !writer.HasError() && writer.Close();
  }
}

