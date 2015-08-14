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

#include <vector>

#include <osmscout/Area.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/String.h>

namespace osmscout {

  std::string AreaAreaIndexGenerator::GetDescription() const
  {
    return "Generate 'areaarea.idx'";
  }

  void AreaAreaIndexGenerator::SetOffsetOfChildren(const std::map<Pixel,AreaLeaf>& leafs,
                                                   std::map<Pixel,AreaLeaf>& newAreaLeafs)
  {
    // For every cell that had entries in one of its children we create
    // an index entry.
    for (const auto& leaf: leafs) {
      // Coordinates of the children in "children dimension" calculated from the tile id
      uint32_t xc=leaf.first.x;
      uint32_t yc=leaf.first.y;

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

      assert(leaf.second.offset!=0);

      newAreaLeafs[Pixel(xc/2,yc/2)].children[index]=leaf.second.offset;
    }
  }

  bool AreaAreaIndexGenerator::WriteIndexLevel(const TypeConfigRef& typeConfig,
                                               const ImportParameter& parameter,
                                               FileWriter& writer,
                                               size_t level,
                                               std::map<Pixel,AreaLeaf>& leafs)
  {
    for (auto& leaf : leafs) {
      if (!writer.GetPos(leaf.second.offset)) {
        return false;
      }

      // There must be a reason to store this index cell: either we have children or data
      assert(!leaf.second.areas.empty() ||
             leaf.second.children[0]!=0 ||
             leaf.second.children[1]!=0 ||
             leaf.second.children[2]!=0 ||
             leaf.second.children[3]!=0);

      // We only store the ids of the children, if we are not the final
      // layer. The file offset of our children will always we smaller
      // our own file offset, and both index values will be different
      if (level<parameter.GetAreaAreaIndexMaxMag()) {
        for (size_t c=0; c<4; c++) {
          FileOffset childOffset;

          if (leaf.second.children[c]==0) {
            childOffset=0;
          }
          else {
            assert(leaf.second.offset>leaf.second.children[c]);
            childOffset=leaf.second.offset-leaf.second.children[c];
          }

          if (!writer.WriteNumber(childOffset)) {
            return false;
          }
        }
      }

      // Number of areas
      writer.WriteNumber((uint32_t)leaf.second.areas.size());

      FileOffset lastOffset=0;
      for (const auto& entry : leaf.second.areas) {
        writer.WriteTypeId(entry.type,
                           typeConfig->GetAreaTypeIdBytes());
        // Since objects are inserted in file position order, we do not need
        // to sort objects by file offset at this place
        writer.WriteNumber(entry.offset-lastOffset);

        lastOffset=entry.offset;
      }
    }

    return !writer.HasError();
  }

  bool AreaAreaIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    FileScanner               scanner;
    size_t                    areas=0;         // Number of areas found
    size_t                    areasConsumed=0; // Number of areas consumed
    std::vector<double>       cellWidth;
    std::vector<double>       cellHeight;
    std::map<Pixel,AreaLeaf>  leafs;
    std::map<Pixel,AreaLeaf>  newAreaLeafs;

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
    FileOffset topLevelOffsetOffset; // Offset of the top level entry

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "areaarea.idx"))) {
      progress.Error("Cannot create 'areaarea.idx'");
      return false;
    }

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                         "areas.dat"),
                         FileScanner::Sequential,
                         parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'areas.dat'");
      return false;
    }

    writer.WriteNumber((uint32_t)parameter.GetAreaAreaIndexMaxMag()); // MaxMag

    if (!writer.GetPos(topLevelOffsetOffset)) {
      progress.Error("Cannot read current file position");
      return false;
    }

    // This is not the final value, that will be written later on
    if (!writer.WriteFileOffset(topLevelOffset)) {
      progress.Error("Cannot write top level entry offset");
      return false;
    }

    size_t l=parameter.GetAreaAreaIndexMaxMag();

    while (true) {
      size_t areaLevelEntries=0;

      progress.Info(std::string("Processing level ")+NumberToString(l)+"...");

      newAreaLeafs.clear();

      SetOffsetOfChildren(leafs,
                          newAreaLeafs);

      leafs=newAreaLeafs;

      // Areas

      if (areas==0 ||
          (areas>0 && areas>areasConsumed)) {
        uint32_t areaCount=0;

        if (!scanner.GotoBegin()) {
          progress.Error("Cannot go to begin of way file");
        }

        if (!scanner.Read(areaCount)) {
          progress.Error("Error while reading number of data entries in file");
          return false;
        }

        areas=0;
        for (uint32_t a=1; a<=areaCount; a++) {
          progress.SetProgress(a,areaCount);

          FileOffset offset;
          Area       area;

          scanner.GetPos(offset);

          if (!area.Read(*typeConfig,
                         scanner)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(a)+" of "+
                           NumberToString(areaCount)+
                           " in file '"+
                           scanner.GetFilename()+"'");
            return false;
          }

          areas++;

          GeoBox boundingBox;

          area.GetBoundingBox(boundingBox);

          GeoCoord center=boundingBox.GetCenter();

          //
          // Calculate highest level where the bounding box completely
          // fits in the cell size and assign area to the tiles that
          // hold the geometric center of the tile.
          //

          size_t level=parameter.GetAreaAreaIndexMaxMag();
          while (true) {
            if (boundingBox.GetWidth()<=cellWidth[level] &&
                boundingBox.GetHeight()<=cellHeight[level]) {
              break;
            }

            if (level==0) {
              break;
            }

            level--;
          }

          if (level==l) {
            // Calculate index of tile that contains the geometric center of the area
            uint32_t x=(uint32_t)((center.GetLon()+180.0)/cellWidth[level]);
            uint32_t y=(uint32_t)((center.GetLat()+90.0)/cellHeight[level]);

            Entry entry;

            entry.type=area.GetType()->GetAreaId();
            entry.offset=offset;

            leafs[Pixel(x,y)].areas.push_back(entry);
            areaLevelEntries++;

            areasConsumed++;
          }
        }
      }

      progress.Debug(std::string("Writing ")+NumberToString(leafs.size())+" leafs ("+
                     NumberToString(areaLevelEntries)+") "+
                     "to index of level "+NumberToString(l)+"...");

      // Remember the offset of one/the cell in level '0'
      if (l==0) {
        if (!writer.GetPos(topLevelOffset)) {
          progress.Error("Cannot read top level entry offset");
          return false;
        }
      }

      /*
      uint32_t minX=std::numeric_limits<uint32_t>::max();
      uint32_t minY=std::numeric_limits<uint32_t>::max();
      uint32_t maxX=std::numeric_limits<uint32_t>::min();
      uint32_t maxY=std::numeric_limits<uint32_t>::min();

      std::map<TypeId,size_t> useMap;

      for (std::map<Pixel,AreaLeaf>::const_iterator leaf=leafs.begin();
           leaf!=leafs.end();
           ++leaf) {
        minX=std::min(minX,leaf->first.x);
        maxX=std::max(maxX,leaf->first.x);
        minY=std::min(minY,leaf->first.y);
        maxY=std::max(maxY,leaf->first.y);

        for (std::list<Entry>::const_iterator entry=leaf->second.areas.begin();
             entry!=leaf->second.areas.end();
             entry++) {
          std::map<TypeId,size_t>::iterator u=useMap.find(entry->type);

          if (u==useMap.end()) {
            useMap[entry->type]=1;
          }
          else {
            u->second++;
          }
        }
      }*/

      /*
      std::cout << "[" << minX << "-" << maxX << "]x[" << minY << "-" << maxY << "] => " << leafs.size() << "/" << (maxX-minX+1)*(maxY-minY+1) << " " << (int)BytesNeededToAddressFileData(leafs.size()) << " " << ByteSizeToString(BytesNeededToAddressFileData(leafs.size())*(maxX-minX+1)*(maxY-minY+1)) << std::endl;

      for (std::map<TypeId,size_t>::const_iterator u=useMap.begin();
          u!=useMap.end();
          ++u) {
        std::cout << "* " << u->first << " " << typeConfig.GetTypeInfo(u->first).GetName() << " " << u->second << std::endl;
      }*/

      if (!WriteIndexLevel(typeConfig,
                           parameter,
                           writer,
                           l,
                           leafs)) {
        return false;
      }

      if (l==0) {
        break;
      }

      l--;
    }

    if (!writer.SetPos(topLevelOffsetOffset) ||
        !writer.WriteFileOffset(topLevelOffset)) {
      return false;
    }

    return !writer.HasError() && writer.Close();
  }
}
