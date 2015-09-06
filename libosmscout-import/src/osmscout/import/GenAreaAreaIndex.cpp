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
#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

namespace osmscout {

  std::string AreaAreaIndexGenerator::GetDescription() const
  {
    return "Generate 'areaarea.idx'";
  }

  void AreaAreaIndexGenerator::EnrichLevels(std::vector<Level>& levels)
  {
    for (size_t l=0; l<levels.size()-1; l++) {
      size_t level=levels.size()-l-1;

      for (const auto& cellEntry : levels[level]) {
        Pixel parentCell(cellEntry.first.x/2,cellEntry.first.y/2);

        if (levels[level-1].find(parentCell)==levels[level-1].end()) {
          levels[level-1].insert(std::make_pair(parentCell,AreaLeaf()));
        }
      }
    }
  }

  bool AreaAreaIndexGenerator::WriteCell(const TypeConfigRef& typeConfig,
                                         const ImportParameter& parameter,
                                         FileWriter& writer,
                                         const std::vector<Level>& levels,
                                         size_t level,
                                         const Pixel& pixel,
                                         const AreaLeaf& leaf,
                                         FileOffset& offset)
  {
    if (level<parameter.GetAreaAreaIndexMaxMag()) {
      Pixel      topLeftPixel(pixel.x*2,pixel.y*2+1);
      FileOffset topLeftOffset=0;

      auto topLeftCell=levels[level+1].find(topLeftPixel);

      if (topLeftCell!=levels[level+1].end()) {
        if (!WriteCell(typeConfig,
                       parameter,
                       writer,
                       levels,
                       level+1,
                       topLeftPixel,
                       topLeftCell->second,
                       topLeftOffset)) {
          return false;
        }
      }

      Pixel      topRightPixel(pixel.x*2+1,pixel.y*2+1);
      FileOffset topRightOffset=0;

      auto topRightCell=levels[level+1].find(topRightPixel);

      if (topRightCell!=levels[level+1].end()) {
        if (!WriteCell(typeConfig,
                       parameter,
                       writer,
                       levels,
                       level+1,
                       topRightPixel,
                       topRightCell->second,
                       topRightOffset)) {
          return false;
        }
      }

      Pixel      bottomLeftPixel(pixel.x*2,pixel.y*2);
      FileOffset bottomLeftOffset=0;

      auto bottomLeftCell=levels[level+1].find(bottomLeftPixel);

      if (bottomLeftCell!=levels[level+1].end()) {
        if (!WriteCell(typeConfig,
                       parameter,
                       writer,
                       levels,
                       level+1,
                       bottomLeftPixel,
                       bottomLeftCell->second,
                       bottomLeftOffset)) {
          return false;
        }
      }

      Pixel      bottomRightPixel(pixel.x*2+1,pixel.y*2);
      FileOffset bottomRightOffset=0;

      auto bottomRightCell=levels[level+1].find(bottomRightPixel);

      if (bottomRightCell!=levels[level+1].end()) {
        if (!WriteCell(typeConfig,
                       parameter,
                       writer,
                       levels,
                       level+1,
                       bottomRightPixel,
                       bottomRightCell->second,
                       bottomRightOffset)) {
          return false;
        }
      }

      if (!writer.GetPos(offset)) {
        return false;
      }

      if (topLeftOffset!=0) {
        topLeftOffset=offset-topLeftOffset;
      }

      if (!writer.WriteNumber(topLeftOffset)) {
        return false;
      }

      if (topRightOffset!=0) {
        topRightOffset=offset-topRightOffset;
      }

      if (!writer.WriteNumber(topRightOffset)) {
        return false;
      }

      if (bottomLeftOffset!=0) {
        bottomLeftOffset=offset-bottomLeftOffset;
      }

      if (!writer.WriteNumber(bottomLeftOffset)) {
        return false;
      }

      if (bottomRightOffset!=0) {
        bottomRightOffset=offset-bottomRightOffset;
      }

      if (!writer.WriteNumber(bottomRightOffset)) {
        return false;
      }
    }
    else {
      if (!writer.GetPos(offset)) {
        return false;
      }
    }

    // Number of areas
    writer.WriteNumber((uint32_t)leaf.areas.size());

    // Since objects are inserted in file position order, we do not need
    // to sort objects by file offset at this place

    FileOffset prevOffset=0;
    for (const auto& entry : leaf.areas) {
      uint64_t value=entry.offset-prevOffset;

      value=value << typeConfig->GetAreaTypeIdBits();
      value=value | entry.type;

      if (level==parameter.GetAreaAreaIndexMaxMag()) {
        value=value << 3;
        value=value | (uint8_t)(entry.level-parameter.GetAreaAreaIndexMaxMag());
      }

      writer.WriteNumber(value);

      prevOffset=entry.offset;
    }

    return !writer.HasError();
  }

  bool AreaAreaIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    FileScanner         scanner;
    std::vector<Level>  levels;

    //
    // Writing index file
    //

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

    progress.SetAction("Building in memory area index from '"+scanner.GetFilename()+"'");

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

    levels.resize(parameter.GetAreaAreaIndexMaxMag()+1);

    /*
    SetOffsetOfChildren(leafs,
                        newAreaLeafs);

    leafs=newAreaLeafs;*/

    // Areas

    uint32_t areaCount=0;

    if (!scanner.GotoBegin()) {
      progress.Error("Cannot go to begin of way file");
    }

    if (!scanner.Read(areaCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

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

      GeoBox boundingBox;

      area.GetBoundingBox(boundingBox);

      GeoCoord center=boundingBox.GetCenter();

      //
      // Calculate highest level where the bounding box completely
      // fits in the cell size and assign area to the tiles that
      // hold the geometric center of the tile.
      //

      size_t sizeLevel=std::min(parameter.GetAreaAreaIndexMaxMag()+7,CELL_DIMENSION_MAX);//parameter.GetAreaAreaIndexMaxMag();
      while (true) {
        if (boundingBox.GetWidth()<=cellDimension[sizeLevel].width &&
            boundingBox.GetHeight()<=cellDimension[sizeLevel].height) {
          break;
        }

        if (sizeLevel==0) {
          break;
        }

        sizeLevel--;
      }

      size_t indexLevel=sizeLevel;

      if (indexLevel>parameter.GetAreaAreaIndexMaxMag()) {
        indexLevel=parameter.GetAreaAreaIndexMaxMag();
      }

      // Calculate index of tile that contains the geometric center of the area
      uint32_t x=(uint32_t)((center.GetLon()+180.0)/cellDimension[indexLevel].width);
      uint32_t y=(uint32_t)((center.GetLat()+90.0)/cellDimension[indexLevel].height);

      Entry entry;

      entry.offset=offset;
      entry.type=area.GetType()->GetAreaId();
      entry.level=sizeLevel;

      levels[indexLevel][Pixel(x,y)].areas.push_back(entry);
    }

    progress.SetAction("Enriching index tree");

    EnrichLevels(levels);

    assert(levels[0].size()==1);

    progress.SetAction("Writing index '" + writer.GetFilename()+"'");

    if (!WriteCell(typeConfig,
                   parameter,
                   writer,
                   levels,
                   0,
                   Pixel(0,0),
                   levels[0][Pixel(0,0)],
                   topLevelOffset)) {
      return false;
    }

    /*
    progress.Debug(std::string("Writing ")+NumberToString(leafs.size())+" leafs ("+
                   NumberToString(areaLevelEntries)+") "+
                   "to index of level "+NumberToString(l)+"...");

    // Remember the offset of one/the cell in level '0'
    if (l==0) {
      if (!writer.GetPos(topLevelOffset)) {
        progress.Error("Cannot read top level entry offset");
        return false;
      }
    }*/

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

    if (!writer.SetPos(topLevelOffsetOffset) ||
        !writer.WriteFileOffset(topLevelOffset)) {
      return false;
    }

    return !writer.HasError() && writer.Close();
  }
}
