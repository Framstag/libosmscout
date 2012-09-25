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

#include <osmscout/import/GenAreaAreaIndex.h>

#include <cassert>
#include <vector>

#include <osmscout/Relation.h>
#include <osmscout/Way.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/String.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  std::string AreaAreaIndexGenerator::GetDescription() const
  {
    return "Generate 'areaarea.idx'";
  }

  void AreaAreaIndexGenerator::SetOffsetOfChildren(const std::map<Coord,AreaLeaf>& leafs,
                                                   std::map<Coord,AreaLeaf>& newAreaLeafs)
  {
    // For every cell that had entries in one of its children we create
    // an index entry.
    for (std::map<Coord,AreaLeaf>::const_iterator leaf=leafs.begin();
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
  }

  bool AreaAreaIndexGenerator::WriteIndexLevel(const ImportParameter& parameter,
                                               FileWriter& writer,
                                               int level,
                                               std::map<Coord,AreaLeaf>& leafs)
  {
    for (std::map<Coord,AreaLeaf>::iterator leaf=leafs.begin();
         leaf!=leafs.end();
         ++leaf) {
      writer.GetPos(leaf->second.offset);

      assert(!leaf->second.areas.empty() ||
             !leaf->second.relAreas.empty() ||
             leaf->second.children[0]!=0 ||
             leaf->second.children[1]!=0 ||
             leaf->second.children[2]!=0 ||
             leaf->second.children[3]!=0);

      if (level<(int)parameter.GetAreaAreaIndexMaxMag()) {
        for (size_t c=0; c<4; c++) {
          writer.WriteNumber(leaf->second.children[c]);
        }
      }

      FileOffset lastOffset;

      // Areas
      writer.WriteNumber((uint32_t)leaf->second.areas.size());

      lastOffset=0;
      for (std::list<Entry>::const_iterator entry=leaf->second.areas.begin();
           entry!=leaf->second.areas.end();
           entry++) {
        writer.WriteNumber(entry->type);
        writer.WriteNumber(entry->offset-lastOffset);

        lastOffset=entry->offset;
      }

      // Relation areas
      writer.WriteNumber((uint32_t)leaf->second.relAreas.size());

      lastOffset=0;
      for (std::list<Entry>::const_iterator entry=leaf->second.relAreas.begin();
           entry!=leaf->second.relAreas.end();
           entry++) {
        writer.WriteNumber(entry->type);
        writer.WriteNumber(entry->offset-lastOffset);

        lastOffset=entry->offset;
      }
    }

    return !writer.HasError();
  }

  bool AreaAreaIndexGenerator::Import(const ImportParameter& parameter,
                                      Progress& progress,
                                      const TypeConfig& typeConfig)
  {
    FileScanner               wayScanner;
    FileScanner               relScanner;
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
      cellWidth[i]=360.0/pow(2.0,(int)i);
    }

    for (size_t i=0; i<cellHeight.size(); i++) {
      cellHeight[i]=180.0/pow(2.0,(int)i);
    }

    //
    // Writing index file
    //

    progress.SetAction("Generating 'areaarea.idx'");

    FileWriter writer;
    FileOffset topLevelOffset=0;
    FileOffset topLevelOffsetOffset; // Offset of the toplevel entry

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "areaarea.idx"))) {
      progress.Error("Cannot create 'areaarea.idx'");
      return false;
    }

    if (!wayScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "ways.dat"),
                         FileScanner::Sequential,
                         parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.dat'");
      return false;
    }

    if (!relScanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "relations.dat"),
                         FileScanner::Sequential,
                         true)) {
      progress.Error("Cannot open 'relations.dat'");
      return false;
    }

    writer.WriteNumber((uint32_t)parameter.GetAreaAreaIndexMaxMag()); // MaxMag

    if (!writer.GetPos(topLevelOffsetOffset)) {
      progress.Error("Cannot read current file position");
      return false;
    }

    if (!writer.WriteFileOffset(topLevelOffset)) {
      progress.Error("Cannot write top level entry offset");
      return false;
    }

    int l=parameter.GetAreaAreaIndexMaxMag();

    while (l>=0) {
      size_t areaLevelEntries=0;
      size_t relAreaLevelEntries=0;

      progress.Info(std::string("Storing level ")+NumberToString(l)+"...");

      newAreaLeafs.clear();

      SetOffsetOfChildren(leafs,newAreaLeafs);

      leafs=newAreaLeafs;

      // Ways

      if (ways==0 ||
          (ways>0 && ways>waysConsumed)) {
        uint32_t wayCount=0;

        progress.Info(std::string("Scanning ways.dat for ways of index level ")+NumberToString(l)+"...");

        if (!wayScanner.GotoBegin()) {
          progress.Error("Cannot go to begin of way file");
        }

        if (!wayScanner.Read(wayCount)) {
          progress.Error("Error while reading number of data entries in file");
          return false;
        }

        ways=0;
        for (uint32_t w=1; w<=wayCount; w++) {
          progress.SetProgress(w,wayCount);

          FileOffset offset;
          Way        way;

          wayScanner.GetPos(offset);

          if (!way.Read(wayScanner)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(w)+" of "+
                           NumberToString(wayCount)+
                           " in file '"+
                           wayScanner.GetFilename()+"'");
            return false;
          }

          if (!way.IsArea()) {
            continue;
          }

          ways++;

          double minLon;
          double maxLon;
          double minLat;
          double maxLat;

          way.GetBoundingBox(minLon,maxLon,minLat,maxLat);

          //
          // Calculate highest level where the bounding box completely
          // fits in the cell size and assign area to the tiles that
          // hold the geometric center of the tile.
          //

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
            // Renormated coordinate space (everything is >=0)
            //

            minLon+=180;
            maxLon+=180;
            minLat+=90;
            maxLat+=90;

            //
            // Calculate minimum and maximum tile ids that are covered
            // by the area
            //
            size_t minyc=(size_t)floor(minLat/cellHeight[level]);
            size_t maxyc=(size_t)floor(maxLat/cellHeight[level]);
            size_t minxc=(size_t)floor(minLon/cellWidth[level]);
            size_t maxxc=(size_t)floor(maxLon/cellWidth[level]);

            Entry entry;

            entry.type=way.GetType();
            entry.offset=offset;

            // Add this area to the tile where the center of the area lies in.
            leafs[Coord((minxc+maxxc)/2,(minyc+maxyc)/2)].areas.push_back(entry);
            areaLevelEntries++;

            waysConsumed++;
          }
        }
      }

      // Relations

      if (rels==0 ||
          (rels>0 && rels>relsConsumed)) {
        uint32_t relCount=0;

        progress.Info(std::string("Scanning relations.dat for relations of index level ")+NumberToString(l)+"...");

        if (!relScanner.GotoBegin()) {
          progress.Error("Cannot go to begin of relation file");
        }

        if (!relScanner.Read(relCount)) {
          progress.Error("Error while reading number of data entries in file");
          return false;
        }

        rels=0;
        for (uint32_t r=1; r<=relCount; r++) {
          progress.SetProgress(r,relCount);

          FileOffset offset;
          Relation   relation;

          relScanner.GetPos(offset);

          if (!relation.Read(relScanner)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(r)+" of "+
                           NumberToString(relCount)+
                           " in file '"+
                           relScanner.GetFilename()+"'");
            return false;
          }

          if (!relation.IsArea()) {
            continue;
          }

          rels++;

          //
          // Bounding box calculation
          //

          double minLon;
          double maxLon;
          double minLat;
          double maxLat;

          relation.GetBoundingBox(minLon,maxLon,minLat,maxLat);

          //
          // Calculate highest level where the bounding box completely
          // fits in the cell size and assign area to the tiles that
          // hold the geometric center of the tile.
          //

          int level=parameter.GetAreaAreaIndexMaxMag();

          while (level>=l) {
            if (maxLon-minLon<=cellWidth[level] &&
                maxLat-minLat<=cellHeight[level]) {
              break;
            }

            level--;
          }

          if (level==l) {
            //
            // Renormated coordinate space (everything is >=0)
            //

            minLon+=180;
            maxLon+=180;
            minLat+=90;
            maxLat+=90;

            //
            // Calculate minimum and maximum tile ids that are covered
            // by the area
            //
            size_t minyc=(size_t)floor(minLat/cellHeight[level]);
            size_t maxyc=(size_t)floor(maxLat/cellHeight[level]);
            size_t minxc=(size_t)floor(minLon/cellWidth[level]);
            size_t maxxc=(size_t)floor(maxLon/cellWidth[level]);

            Entry entry;

            entry.type=relation.GetType();
            entry.offset=offset;

            // Add this area to the tile where the center of the area lies in.
            leafs[Coord((minxc+maxxc)/2,(minyc+maxyc)/2)].relAreas.push_back(entry);
            relAreaLevelEntries++;

            relsConsumed++;
          }
        }
      }

      progress.Debug(std::string("Writing ")+NumberToString(leafs.size())+" leafs ("+
                     NumberToString(areaLevelEntries)+"/"+
                     NumberToString(relAreaLevelEntries)+") "+
                     "to index of level "+NumberToString(l)+"...");

      // Remember the offset of one cell in level '0'
      if (l==0) {
        if (!writer.GetPos(topLevelOffset)) {
          progress.Error("Cannot read top level entry offset");
          return false;
        }
      }

      if (!WriteIndexLevel(parameter,
                           writer,
                           l,
                           leafs)) {
        return false;
      }

      l--;
    }

    writer.SetPos(topLevelOffsetOffset);
    writer.WriteFileOffset(topLevelOffset);

    return !writer.HasError() && writer.Close();
  }
}

