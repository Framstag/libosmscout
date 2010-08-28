/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/GenAreaIndex.h>

#include <cassert>
#include <cmath>
#include <vector>

#include <osmscout/FileScanner.h>
#include <osmscout/FileWriter.h>

#include <osmscout/Relation.h>
#include <osmscout/Way.h>

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

  struct AreaLeaf
  {
    FileOffset                              offset;
    std::map<TypeId,std::list<FileOffset> > ways;
    std::map<TypeId,std::list<FileOffset> > relWays;
    std::list<FileOffset>                   areas;
    std::list<FileOffset>                   relAreas;
    FileOffset                              children[4];

    AreaLeaf()
    {
      offset=0;
      children[0]=0;
      children[1]=0;
      children[2]=0;
      children[3]=0;
    }
  };

  std::string AreaIndexGenerator::GetDescription() const
  {
    return "Generate 'area.idx'";
  }

  bool AreaIndexGenerator::Import(const ImportParameter& parameter,
                                  Progress& progress,
                                  const TypeConfig& typeConfig)
  {
    FileScanner               scanner;
    std::set<Id>              wayBlacklist;   // Ids of ways that should not be in index
    size_t                    ways=0;         // Number of ways found
    size_t                    waysConsumed=0; // Number of ways consumed
    size_t                    rels=0;         // Number of relations found
    size_t                    relsConsumed=0; // Number of relations consumed
    std::vector<double>       cellWidth;
    std::vector<double>       cellHeight;
    std::map<Coord,AreaLeaf>  leafs;
    std::map<Coord,AreaLeaf>  newAreaLeafs;

    cellWidth.resize(parameter.GetAreaAreaIndexMaxMag()+1);
    cellHeight.resize(parameter.GetAreaAreaIndexMaxMag()+1);

    for (size_t i=0; i<cellWidth.size(); i++) {
      cellWidth[i]=360/pow(2,i);
    }

    for (size_t i=0; i<cellHeight.size(); i++) {
      cellHeight[i]=180/pow(2,i);
    }

    //
    // Loading way blacklist
    //

    progress.SetAction("Loading way blacklist");

    if (!scanner.Open("wayblack.dat")) {
      progress.Error("Cannot open 'wayblack.dat'");
      return false;
    }

    while (!scanner.IsEOF()) {
      Id id;

      scanner.Read(id);

      if (!scanner.HasError()){
        wayBlacklist.insert(id);
      }
    }

    if (!scanner.Close()) {
      return false;
    }

    //
    // Writing index file
    //

    progress.SetAction("Generating 'area.idx'");

    FileWriter writer;

    if (!writer.Open("area.idx")) {
      progress.Error("Cannot create 'area.idx'");
      return false;
    }

    writer.WriteNumber((uint32_t)parameter.GetAreaAreaIndexMaxMag()); // MaxMag

    int l=parameter.GetAreaAreaIndexMaxMag();

    while (l>=0) {
      size_t wayLevelEntries=0;
      size_t areaLevelEntries=0;
      size_t relWayLevelEntries=0;
      size_t relAreaLevelEntries=0;

      progress.Info(std::string("Storing level ")+NumberToString(l)+"...");

      newAreaLeafs.clear();

      // For every cell that had entries in one of its children we create
      // an index entry.
      for (std::map<Coord,AreaLeaf>::iterator leaf=leafs.begin();
           leaf!=leafs.end();
           ++leaf) {
        // Coordinates of the children in "children dimension" calculated from the tile id
        size_t xc=leaf->first.x;
        size_t yc=leaf->first.y;

        size_t index;

        //
        // child index is build as following (y-axis is from bottom to top!):
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

        newAreaLeafs[Coord(xc/2,yc/2)].children[index]=leaf->second.offset;
      }

      leafs=newAreaLeafs;

      // Ways

      if (ways==0 ||
          (ways>0 && ways>waysConsumed)) {
        uint32_t wayCount=0;

        progress.Info(std::string("Scanning ways.dat for ways of index level ")+NumberToString(l)+"...");

        if (!scanner.Open("ways.dat")) {
          progress.Error("Cannot open 'ways.dat'");
          return false;
        }

        if (!scanner.Read(wayCount)) {
          progress.Error("Error while reading number of data entries in file");
          return false;
        }

        ways=0;
        for (uint32_t w=1; w<=wayCount; w++) {
          FileOffset offset;
          Way        way;

          scanner.GetPos(offset);
          if (!way.Read(scanner)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(w)+" of "+
                           NumberToString(wayCount)+
                           " in file '"+
                           scanner.GetFilename()+"'");
            return false;
          }

          // We do not index a way that is in the blacklist
          if (wayBlacklist.find(way.id)!=wayBlacklist.end()) {
            continue;
          }

          ways++;

          if (way.IsArea()) { // Areas
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
                  leafs[Coord(xc,yc)].areas.push_back(offset);
                  areaLevelEntries++;
                }
              }

              waysConsumed++;
            }
          }
          else { // Ways
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
            // fits in the cell size and in one cell for this level.
            //

            // TODO: We can possibly do faster
            // ...in calculating level
            //...in detecting if this way is relevant for this level
            int    level=parameter.GetAreaWayIndexMaxMag();

            while (level>=l) {
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

              // Now index the way for all cells in the bounding box
              // TODO: We could do better by only indexing the way for all cells that it
              // really intersects, thus reducing the number of (false) entries in the index
              for (size_t yc=minyc; yc<=maxyc; yc++) {
                for (size_t xc=minxc; xc<=maxxc; xc++) {
                  leafs[Coord(xc,yc)].ways[way.GetType()].push_back(offset);
                  wayLevelEntries++;
                }
              }

              waysConsumed++;
            }
          }
        }

        scanner.Close();
      }

      // Relations

      if (rels==0 ||
          (rels>0 && rels>relsConsumed)) {
        uint32_t relCount=0;

        progress.Info(std::string("Scanning relations.dat for relations of index level ")+NumberToString(l)+"...");

        if (!scanner.Open("relations.dat")) {
          progress.Error("Cannot open 'relations.dat'");
          return false;
        }

        if (!scanner.Read(relCount)) {
          progress.Error("Error while reading number of data entries in file");
          return false;
        }

        rels=0;
        for (uint32_t r=1; r<=relCount; r++) {
          FileOffset offset;
          Relation   relation;

          scanner.GetPos(offset);

          if (!relation.Read(scanner)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(r)+" of "+
                           NumberToString(relCount)+
                           " in file '"+
                           scanner.GetFilename()+"'");
            return false;
          }

          rels++;

          if (relation.IsArea()) {
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

            while (level>=l) {
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

              for (size_t yc=minyc; yc<=maxyc; yc++) {
                for (size_t xc=minxc; xc<=maxxc; xc++) {
                  leafs[Coord(xc,yc)].relAreas.push_back(offset);
                  relAreaLevelEntries++;
                }
              }

              relsConsumed++;
            }
          }
          else {
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

            while (level>=l) {
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

              for (size_t yc=minyc; yc<=maxyc; yc++) {
                for (size_t xc=minxc; xc<=maxxc; xc++) {
                  leafs[Coord(xc,yc)].relWays[relation.type].push_back(offset);
                  relWayLevelEntries++;
                }
              }

              relsConsumed++;
            }
          }
        }

        scanner.Close();
      }

      progress.Debug(std::string("Writing ")+NumberToString(leafs.size())+" entries ("+
                     NumberToString(wayLevelEntries)+"/"+
                     NumberToString(relWayLevelEntries)+"/"+
                     NumberToString(areaLevelEntries)+"/"+
                     NumberToString(relAreaLevelEntries)+") "+
                     "to index of level "+NumberToString(l)+"...");
      //
      // Store all index entries for this level and store their file offset
      //
      writer.WriteNumber((uint32_t)leafs.size()); // Number of leafs
      for (std::map<Coord,AreaLeaf>::iterator leaf=leafs.begin();
           leaf!=leafs.end();
           ++leaf) {
        writer.GetPos(leaf->second.offset);

        assert(leaf->second.ways.size()>0 ||
               leaf->second.relWays.size()>0 ||
               leaf->second.areas.size()>0 ||
               leaf->second.relAreas.size()>0 ||
               leaf->second.children[0]!=0 ||
               leaf->second.children[1]!=0 ||
               leaf->second.children[2]!=0 ||
               leaf->second.children[3]!=0);

        if (l<parameter.GetAreaAreaIndexMaxMag()) {
          // TODO: Is writer.Write better?
          for (size_t c=0; c<4; c++) {
            writer.WriteNumber(leaf->second.children[c]);
          }
        }

        // Ways
        writer.WriteNumber((uint32_t)leaf->second.ways.size());
        for (std::map<TypeId, std::list<FileOffset> >::const_iterator entry=leaf->second.ways.begin();
             entry!=leaf->second.ways.end();
             ++entry) {
          writer.WriteNumber(entry->first);
          writer.WriteNumber((uint32_t)entry->second.size());

          for (std::list<FileOffset>::const_iterator o=entry->second.begin();
               o!=entry->second.end();
               o++) {
            // TODO: Is writer.Write better?
            writer.Write(*o);
          }
        }

        // Relation ways
        writer.WriteNumber((uint32_t)leaf->second.relWays.size());
        for (std::map<TypeId, std::list<FileOffset> >::const_iterator entry=leaf->second.relWays.begin();
             entry!=leaf->second.relWays.end();
             ++entry) {
          writer.WriteNumber(entry->first);
          writer.WriteNumber((uint32_t)entry->second.size());

          for (std::list<FileOffset>::const_iterator o=entry->second.begin();
               o!=entry->second.end();
               o++) {
            // TODO: Is writer.Write better?
            writer.Write(*o);
          }
        }

        // Areas
        writer.WriteNumber((uint32_t)leaf->second.areas.size());
        for (std::list<FileOffset>::const_iterator o=leaf->second.areas.begin();
             o!=leaf->second.areas.end();
             o++) {
          // TODO: Is writer.Write better?
          writer.Write(*o);
        }

        // Relation areas
        writer.WriteNumber((uint32_t)leaf->second.relAreas.size());
        for (std::list<FileOffset>::const_iterator o=leaf->second.relAreas.begin();
             o!=leaf->second.relAreas.end();
             o++) {
          // TODO: Is writer.Write better?
          writer.Write(*o);
        }
      }

      l--;
    }

    return !writer.HasError() && writer.Close();
  }
}

