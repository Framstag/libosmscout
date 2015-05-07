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

#include <osmscout/AreaAreaIndex.h>

#include <iostream>

#include <osmscout/util/Logger.h>

#include <osmscout/system/Math.h>

namespace osmscout {

  AreaAreaIndex::AreaAreaIndex(size_t cacheSize)
  : filepart("areaarea.idx"),
    maxLevel(0),
    topLevelOffset(0),
    indexCache(cacheSize)
  {
    // no code
  }

  void AreaAreaIndex::Close()
  {
    if (scanner.IsOpen()) {
      scanner.Close();
    }
  }

  bool AreaAreaIndex::GetIndexCell(const TypeConfig& typeConfig,
                                   uint32_t level,
                                   FileOffset offset,
                                   IndexCache::CacheRef& cacheRef) const
  {
    if (!indexCache.GetEntry(offset,cacheRef)) {
      IndexCache::CacheEntry cacheEntry(offset);

      cacheRef=indexCache.SetEntry(cacheEntry);

      if (!scanner.IsOpen()) {
        if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
          log.Error() << "Error while opening '" << scanner.GetFilename() << "' for reading!";
          return false;
        }
      }

      scanner.SetPos(offset);

      // Read offsets of children if not in the bottom level

      if (level<maxLevel) {
        for (size_t c=0; c<4; c++) {
          if (!scanner.ReadNumber(cacheRef->value.children[c])) {
            log.Error() << "Cannot read index data at offset " << offset << " in file '" << scanner.GetFilename() << "'";
            return false;
          }
        }
      }
      else {
        for (size_t c=0; c<4; c++) {
          cacheRef->value.children[c]=0;
        }
      }

      // Now read the way offsets by type in this index entry

      uint32_t offsetCount;

      // Areas

      if (!scanner.ReadNumber(offsetCount)) {
        log.Error() << "Cannot read index data for level " << level << " at offset " << offset << " in file '" << scanner.GetFilename() << "'";
        return false;
      }

      cacheRef->value.areas.resize(offsetCount);

      FileOffset prevOffset=0;

      for (size_t c=0; c<offsetCount; c++) {
        if (!scanner.ReadTypeId(cacheRef->value.areas[c].type,
                                typeConfig.GetAreaTypeIdBytes())) {
          log.Error() << "Cannot read index data for level " << level << " at offset " << offset << " in file '" << scanner.GetFilename() << "'";
          return false;
        }

        if (!scanner.ReadNumber(cacheRef->value.areas[c].offset)) {
          log.Error() << "Cannot read index data for level " << level << " at offset " << offset << " in file '" << scanner.GetFilename() << "'";
          return false;
        }

        cacheRef->value.areas[c].offset+=prevOffset;

        prevOffset=cacheRef->value.areas[c].offset;
      }
    }

    return true;
  }

  bool AreaAreaIndex::Load(const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename,FileScanner::LowMemRandom,true)) {
      log.Error() << "Cannot open file '" << scanner.GetFilename() << "'";
      return false;
    }

    if (!scanner.ReadNumber(maxLevel)) {
      log.Error() << "Cannot read data from file '" << scanner.GetFilename() << "'";
      return false;
    }

    if (!scanner.ReadFileOffset(topLevelOffset)) {
      log.Error() << "Cannot read data from file '" << scanner.GetFilename() << "'";
      return false;
    }

    // Calculate the size of a cell in each index level
    cellWidth.resize(maxLevel+1);
    cellHeight.resize(maxLevel+1);

    for (size_t i=0; i<cellWidth.size(); i++) {
      cellWidth[i]=360.0/pow(2.0,(int)i);
    }

    for (size_t i=0; i<cellHeight.size(); i++) {
      cellHeight[i]=180.0/pow(2.0,(int)i);
    }

    return !scanner.HasError() && scanner.Close();
  }

  bool AreaAreaIndex::GetOffsets(const TypeConfigRef& typeConfig,
                                 double minlon,
                                 double minlat,
                                 double maxlon,
                                 double maxlat,
                                 size_t maxLevel,
                                 const TypeSet& types,
                                 size_t maxCount,
                                 std::vector<FileOffset>& offsets) const
  {
    std::vector<CellRef>    cellRefs;     // cells to scan in this level
    std::vector<CellRef>    nextCellRefs; // cells to scan for the next level
    std::vector<FileOffset> newOffsets;   // offsets collected in the current level

    minlon+=180;
    maxlon+=180;
    minlat+=90;
    maxlat+=90;

    // Clear result datastructures
    offsets.clear();

    // Make the vector preallocate memory for the expected data size
    // This should void reallocation
    offsets.reserve(std::min(100000u,(uint32_t)maxCount));
    newOffsets.reserve(std::min(100000u,(uint32_t)maxCount));

    cellRefs.reserve(1000);

    nextCellRefs.reserve(1000);

    cellRefs.push_back(CellRef(topLevelOffset,0,0));

    // For all levels:
    // * Take the tiles and offsets of the last level
    // * Calculate the new tiles and offsets that still interfere with given area
    // * Add the new offsets to the list of offsets and finish if we have
    //   reached maxLevel or maxAreaCount.
    // * copy no, ntx, nty to ctx, cty, co and go to next iteration
    bool stopArea=false;
    for (uint32_t level=0;
         !stopArea &&
         level<=this->maxLevel &&
         level<=maxLevel &&
         !cellRefs.empty();
         level++) {
      nextCellRefs.clear();

      newOffsets.clear();

      for (size_t i=0; !stopArea && i<cellRefs.size(); i++) {
        size_t               cx;
        size_t               cy;
        IndexCache::CacheRef cell;

        if (!GetIndexCell(*typeConfig,
                          level,
                          cellRefs[i].offset,
                          cell)) {
          log.Error() << "Cannot find offset " << cellRefs[i].offset << " in level " << level << " in file '" << scanner.GetFilename() << "'";
          return false;
        }

        if (offsets.size()+
            newOffsets.size()+
            cell->value.areas.size()>=maxCount) {
          stopArea=true;
          continue;
        }

        for (const auto entry : cell->value.areas) {
          if (types.IsTypeSet(entry.type)) {
            newOffsets.push_back(entry.offset);
          }
        }

        cx=cellRefs[i].x*2;
        cy=cellRefs[i].y*2;

        if (cell->value.children[0]!=0) {
          // top left
          double x=cx*cellWidth[level+1];
          double y=(cy+1)*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            nextCellRefs.push_back(CellRef(cell->value.children[0],cx,cy+1));
          }
        }

        if (cell->value.children[1]!=0) {
          // top right
          double x=(cx+1)*cellWidth[level+1];
          double y=(cy+1)*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            nextCellRefs.push_back(CellRef(cell->value.children[1],cx+1,cy+1));
          }
        }

        if (cell->value.children[2]!=0) {
          // bottom left
          double x=cx*cellWidth[level+1];
          double y=cy*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            nextCellRefs.push_back(CellRef(cell->value.children[2],cx,cy));
          }
        }

        if (cell->value.children[3]!=0) {
          // bottom right
          double x=(cx+1)*cellWidth[level+1];
          double y=cy*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            nextCellRefs.push_back(CellRef(cell->value.children[3],cx+1,cy));
          }
        }
      }

      if (!stopArea) {
        offsets.insert(offsets.end(),newOffsets.begin(),newOffsets.end());
      }

      std::swap(cellRefs,nextCellRefs);
    }

    return true;
  }

  void AreaAreaIndex::DumpStatistics()
  {
    indexCache.DumpStatistics(filepart.c_str(),IndexCacheValueSizer());
  }
}

