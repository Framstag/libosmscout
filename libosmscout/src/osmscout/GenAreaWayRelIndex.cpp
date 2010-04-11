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

#include <osmscout/GenAreaWayRelIndex.h>

#include <cassert>
#include <cmath>
#include <list>
#include <map>
#include <vector>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>
#include <osmscout/Relation.h>
#include <osmscout/Util.h>

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


  struct WayRelLeaf
  {
    FileOffset                              offset;
    FileOffset                              children[4];
    std::map<TypeId,std::list<FileOffset> > dataOffsets;

    WayRelLeaf()
    : offset(0)
    {
      children[0]=0;
      children[1]=0;
      children[2]=0;
      children[3]=0;
    }
  };

  std::string AreaWayRelIndexGenerator::GetDescription() const
  {
    return "Generate 'areawayrel.idx'";
  }

  bool AreaWayRelIndexGenerator::Import(const ImportParameter& parameter,
                                        Progress& progress,
                                        const TypeConfig& typeConfig)
  {
    //
    // Count the number of objects per index level
    //

    progress.SetAction("Analysing distribution");

    FileScanner                scanner;
    size_t                     relations=0;     // Number of relations found
    size_t                     consumed=0; // Number of relations consumed
    std::vector<double>        cellWidth;
    std::vector<double>        cellHeight;
    std::map<Coord,WayRelLeaf> leafs;
    std::map<Coord,WayRelLeaf> newLeafs;

    cellWidth.resize(parameter.GetAreaWayRelIndexMaxMag()+1);
    cellHeight.resize(parameter.GetAreaWayRelIndexMaxMag()+1);

    for (size_t i=0; i<cellWidth.size(); i++) {
      cellWidth[i]=360/pow(2,i);
    }

    for (size_t i=0; i<cellHeight.size(); i++) {
      cellHeight[i]=180/pow(2,i);
    }

    //
    // Writing index file
    //

    progress.SetAction("Generating 'areawayrel.idx'");

    FileWriter writer;

    if (!writer.Open("areawayrel.idx")) {
      progress.Error("Cannot create 'areawayrel.idx'");
      return false;
    }

    writer.WriteNumber((uint32_t)parameter.GetAreaWayRelIndexMaxMag()); // MaxMag

    int l=parameter.GetAreaWayRelIndexMaxMag();

    while (l>=0) {
      size_t levelEntries=0;

      progress.Info(std::string("Storing level ")+NumberToString(l)+"...");

      newLeafs.clear();

      // For every cell that had entries in one of its childrenwe create
      // an index entry.
      for (std::map<Coord,WayRelLeaf>::iterator leaf=leafs.begin();
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

      if (relations==0 ||
          (relations>0 && relations>consumed)) {

        progress.Info(std::string("Scanning relations.dat for relations of index level ")+NumberToString(l)+"...");

        if (!scanner.Open("relations.dat")) {
          progress.Error("Cannot open 'relations.dat'");
          return false;
        }

        relations=0;
        while (!scanner.HasError()) {
          FileOffset offset;
          Relation    relation;

          scanner.GetPos(offset);
          relation.Read(scanner);

          if (scanner.HasError()) {
            continue;
          }

          if (relation.IsArea()) {
            continue;
          }

          relations++;

          //
          // Bounding box calculation
          //

          double minLon=relation.roles[0].nodes[0].lon;
          double maxLon=relation.roles[0].nodes[0].lon;
          double minLat=relation.roles[0].nodes[0].lat;
          double maxLat=relation.roles[0].nodes[0].lat;

          for (std::vector<Relation::Role>::const_iterator role=relation.roles.begin();
               role!=relation.roles.end();
               ++role) {
            for (size_t i=0; i<role->nodes.size(); i++) {
              minLon=std::min(minLon,role->nodes[i].lon);
              maxLon=std::max(maxLon,role->nodes[i].lon);
              minLat=std::min(minLat,role->nodes[i].lat);
              maxLat=std::max(maxLat,role->nodes[i].lat);
            }
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
          // fits in the cell size and in one cell for this level.
          //

          // TODO: We can possibly do faster
          // ...in calculating level
          //...in detecting if this way is relevant for this level
          int    level=parameter.GetAreaWayIndexMaxMag();
          size_t minxc=0;
          size_t maxxc=0;
          size_t minyc=0;
          size_t maxyc=0;

          while (level>=l) {
            minxc=floor(minLon/cellWidth[level]);
            maxxc=floor(maxLon/cellWidth[level]);
            minyc=floor(minLat/cellHeight[level]);
            maxyc=floor(maxLat/cellHeight[level]);

            if (maxLon-minLon<=cellWidth[level] &&
                maxLat-minLat<=cellHeight[level]) {
              break;
            }

            level--;
          }

          if (level==l) {
            for (size_t yc=minyc; yc<=maxyc; yc++) {
              for (size_t xc=minxc; xc<=maxxc; xc++) {
                leafs[Coord(xc,yc)].dataOffsets[relation.type].push_back(offset);
                levelEntries++;
              }
            }

            levelEntries++;
            consumed++;
          }
        }

        scanner.Close();
      }

      progress.Debug(std::string("Writing ")+NumberToString(leafs.size())+" entries with "+NumberToString(levelEntries)+" relations to index of level "+NumberToString(l)+"...");
      //
      // Store all index entries for this level and store their file offset
      //
      writer.WriteNumber((uint32_t)leafs.size()); // Number of leafs
      for (std::map<Coord,WayRelLeaf>::iterator leaf=leafs.begin();
           leaf!=leafs.end();
           ++leaf) {
        writer.GetPos(leaf->second.offset);

        assert(leaf->second.dataOffsets.size()>0 ||
               leaf->second.children[0]!=0 ||
               leaf->second.children[1]!=0 ||
               leaf->second.children[2]!=0 ||
               leaf->second.children[3]!=0);

        if (l<parameter.GetAreaWayRelIndexMaxMag()) {
          // TODO: Is writer.Write better?
          for (size_t c=0; c<4; c++) {
            writer.WriteNumber(leaf->second.children[c]);
          }
        }

        writer.WriteNumber((uint32_t)leaf->second.dataOffsets.size());
        for (std::map<TypeId, std::list<FileOffset> >::const_iterator entry=leaf->second.dataOffsets.begin();
             entry!=leaf->second.dataOffsets.end();
             ++entry) {
          writer.WriteNumber(entry->first);
          writer.WriteNumber((uint32_t)entry->second.size());

          for (std::list<FileOffset>::const_iterator o=entry->second.begin();
               o!=entry->second.end();
               o++) {
            // TODO: Is writer.Write better?
            writer.WriteNumber(*o);
          }
        }
      }

      l--;
    }

    return !writer.HasError() && writer.Close();
  }
}

