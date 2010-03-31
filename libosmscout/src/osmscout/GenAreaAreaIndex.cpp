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

#include <osmscout/GenAreaAreaIndex.h>

#include <cassert>
#include <cmath>
#include <vector>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Util.h>
#include <osmscout/Way.h>

#include <iostream>

namespace osmscout {

  struct Coord
  {
    size_t x;
    size_t y;

    Coord(size_t x, size_t y)
     :x(x),y(y)
    {
      // no code
    }

    bool operator==(const Coord& other) const
    {
      return x==other.x && y==other.y;
    }

    bool operator<(const Coord& other) const
    {
      return y<other.y ||
      ( y==other.y && x<other.x);
    }
  };

  struct Leaf
  {
    FileOffset            offset;
    std::list<FileOffset> dataOffsets;
    FileOffset            children[4];

    Leaf()
    {
      offset=0;
      children[0]=0;
      children[1]=0;
      children[2]=0;
      children[3]=0;
    }
  };

  bool GenerateAreaAreaIndex(const ImportParameter& parameter,
                             Progress& progress)
  {
    //
    // Count the number of objects per index level
    //

    progress.SetAction("Analysing distribution");

    FileScanner           scanner;
    size_t                ways=0;     // Number of ways found
    size_t                consumed=0; // Number of ways consumed
    std::vector<double>   cellWidth;
    std::vector<double>   cellHeight;
    std::map<Coord,Leaf>  leafs;
    std::map<Coord,Leaf>  newLeafs;

    cellWidth.resize(parameter.GetAreaAreaIndexMaxMag()+1);
    cellHeight.resize(parameter.GetAreaAreaIndexMaxMag()+1);

    for (size_t i=0; i<cellWidth.size(); i++) {
      cellWidth[i]=360/pow(2,i);
    }

    for (size_t i=0; i<cellHeight.size(); i++) {
      cellHeight[i]=180/pow(2,i);
    }

    //
    // Writing index file
    //

    progress.SetAction("Generating 'areaarea.idx'");

    FileWriter writer;

    if (!writer.Open("areaarea.idx")) {
      progress.Error("Cannot create 'areaarea.idx'");
      return false;
    }

    writer.WriteNumber((unsigned long)parameter.GetAreaAreaIndexMaxMag()); // MaxMag

    int l=parameter.GetAreaAreaIndexMaxMag();

    while (l>=0) {
      size_t levelEntries=0;

      progress.Info(std::string("Storing level ")+NumberToString(l)+"...");

      newLeafs.clear();

      // For every cell that had entries in one of its childrenwe create
      // an index entry.
      for (std::map<Coord,Leaf>::iterator leaf=leafs.begin();
           leaf!=leafs.end();
           ++leaf) {
        // Coordinates of the children in "children dimension" calculated from the tile id
        size_t xc=leaf->first.x;
        size_t yc=leaf->first.y;

        size_t index;

        //
        // child index is build as folowing (y-axis is from bottom to top!):
        //   01
        //   23

        if (yc%2!=0) {
          if (xc%2==0) {
            index=0;
          }
          else {
            index=1;
          }
        }
        else {
          if (xc%2==0) {
            index=2;
          }
          else {
            index=3;
          }
        }

        assert(leaf->second.offset!=0);

        newLeafs[Coord(xc/2,yc/2)].children[index]=leaf->second.offset;
      }

      leafs=newLeafs;

      if (ways==0 ||
          (ways>0 && ways>consumed)) {

        progress.Info(std::string("Scanning ways.dat for ways of index level ")+NumberToString(l)+"...");

        if (!scanner.Open("ways.dat")) {
          progress.Error("Cannot open 'ways.dat'");
          return false;
        }

        ways=0;
        while (!scanner.HasError()) {
          FileOffset offset;
          Way        way;

          scanner.GetPos(offset);
          way.Read(scanner);

          if (scanner.HasError()) {
            continue;
          }

          if (!way.IsArea()) {
            continue;
          }

          ways++;

          //
          // Bounding box calculation
          //

          double minLon=way.nodes[0].lon;
          double maxLon=way.nodes[0].lon;
          double minLat=way.nodes[0].lat;
          double maxLat=way.nodes[0].lat;

          for (size_t i=1; i<way.nodes.size(); i++) {
            minLon=std::min(minLon,way.nodes[i].lon);
            maxLon=std::max(maxLon,way.nodes[i].lon);
            minLat=std::min(minLat,way.nodes[i].lat);
            maxLat=std::max(maxLat,way.nodes[i].lat);
          }

          //
          // Renormated coordinate space (everything is >=0)
          //

          minLon+=180;
          maxLon+=180;
          minLat+=90;
          maxLat+=90;

          //
          // Calculate highest level where the bounding box completely
          // fits in the cell size for this level.
          //

          // TODO: We can possibly do faster
          // ...in calculating level
          //...in detecting if this way is relevant for this level
          int level=parameter.GetAreaAreaIndexMaxMag();
          while (level>=0) {
            if (maxLon-minLon<=cellWidth[level] &&
                maxLat-minLat<=cellHeight[level]) {
              break;
            }

            level--;
          }

          if (level==l) {
            //
            // Calculate all tile ids that are covered
            // by the area
            //
            size_t minyc=floor(minLat/cellHeight[level]);
            size_t maxyc=floor(maxLat/cellHeight[level]);
            size_t minxc=floor(minLon/cellWidth[level]);
            size_t maxxc=floor(maxLon/cellWidth[level]);

            //
            // Add offset to all tiles in this level that coint (parts) of
            // the bounding box
            //
            for (size_t yc=minyc; yc<=maxyc; yc++) {
              for (size_t xc=minxc; xc<=maxxc; xc++) {
                leafs[Coord(xc,yc)].dataOffsets.push_back(offset);
                levelEntries++;
              }
            }
            consumed++;
          }
        }

        scanner.Close();
      }

      progress.Debug(std::string("Writing ")+NumberToString(leafs.size())+" entries with "+NumberToString(levelEntries)+" ways to index of level "+NumberToString(l)+"...");
      //
      // Store all index entries for this level and store their file offset
      //
      writer.WriteNumber((unsigned long)leafs.size()); // Number of leafs
      for (std::map<Coord,Leaf>::iterator leaf=leafs.begin();
           leaf!=leafs.end();
           ++leaf) {
        writer.GetPos(leaf->second.offset);

        assert(leaf->second.dataOffsets.size()>0 ||
               leaf->second.children[0]!=0 ||
               leaf->second.children[1]!=0 ||
               leaf->second.children[2]!=0 ||
               leaf->second.children[3]!=0);

        if (l<parameter.GetAreaAreaIndexMaxMag()) {
          // TODO: Is writer.Write better?
          for (size_t c=0; c<4; c++) {
            writer.WriteNumber((long)leaf->second.children[c]);
          }
        }

        writer.WriteNumber((unsigned long)leaf->second.dataOffsets.size());
        for (std::list<FileOffset>::const_iterator o=leaf->second.dataOffsets.begin();
             o!=leaf->second.dataOffsets.end();
             o++) {
          // TODO: Is writer.Write better?
          writer.WriteNumber((unsigned long)*o);
        }
      }

      l--;
    }

    return !writer.HasError() && writer.Close();
  }
}

