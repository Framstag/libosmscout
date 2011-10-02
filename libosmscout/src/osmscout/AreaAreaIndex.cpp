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

#include <osmscout/private/Math.h>

namespace osmscout {

  AreaAreaIndex::AreaAreaIndex(size_t cacheSize)
  : filepart("areaarea.idx"),
    indexCache(cacheSize)
  {
    // no code
  }

  bool AreaAreaIndex::GetIndexCell(uint32_t level,
                                   FileOffset offset,
                                   IndexCache::CacheRef& cacheRef) const
  {
    if (!indexCache.GetEntry(offset,cacheRef)) {
      IndexCache::CacheEntry cacheEntry(offset);

      cacheRef=indexCache.SetEntry(cacheEntry);

      if (!scanner.IsOpen()) {
        if (!scanner.Open(datafilename)) {
          std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
          return false;
        }
      }

      scanner.SetPos(offset);

      // Read offsets of children if not in the bottom level

      if (level<maxLevel) {
        for (size_t c=0; c<4; c++) {
          if (!scanner.ReadNumber(cacheRef->value.children[c])) {
            std::cerr << "Cannot read index data at offset " << offset << std::endl;
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
        std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
        return false;
      }

      cacheRef->value.areas.resize(offsetCount);

      FileOffset lastOffset;

      lastOffset=0;
      for (size_t c=0; c<offsetCount; c++) {
        if (!scanner.ReadNumber(cacheRef->value.areas[c].type)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }
        if (!scanner.ReadNumber(cacheRef->value.areas[c].offset)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }

        cacheRef->value.areas[c].offset+=lastOffset;

        lastOffset=cacheRef->value.areas[c].offset;
      }

      // Relation areas

      if (!scanner.ReadNumber(offsetCount)) {
        std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
        return false;
      }

      cacheRef->value.relAreas.resize(offsetCount);

      lastOffset=0;
      for (size_t c=0; c<offsetCount; c++) {
        if (!scanner.ReadNumber(cacheRef->value.relAreas[c].type)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }
        if (!scanner.ReadNumber(cacheRef->value.relAreas[c].offset)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }

        cacheRef->value.relAreas[c].offset+=lastOffset;

        lastOffset=cacheRef->value.relAreas[c].offset;
      }
    }

    return true;
  }

  bool AreaAreaIndex::Load(const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename)) {
      std::cerr << "Cannot open file '" << datafilename << "'" << std::endl;
      return false;
    }

    if (!scanner.ReadNumber(maxLevel)) {
      std::cerr << "Cannot read data" << std::endl;
      return false;
    }

    if (!scanner.Read(topLevelOffset)) {
      std::cerr << "Cannot read data" << std::endl;
      return false;
    }

    //std::cout << "Max level is: " << maxLevel << std::endl;

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

  bool AreaAreaIndex::GetOffsets(double minlon,
                                 double minlat,
                                 double maxlon,
                                 double maxlat,
                                 size_t maxAreaLevel,
                                 const TypeSet& types,
                                 size_t maxAreaCount,
                                 std::vector<FileOffset>& wayAreaOffsets,
                                 std::vector<FileOffset>& relationAreaOffsets) const
  {
    std::vector<size_t>     ctx;  // tile x coordinates in this level
    std::vector<size_t>     cty;  // tile y coordinates in this level
    std::vector<FileOffset> co;   // offsets in this level

    std::vector<size_t>     ntx;  // tile x coordinates in next level
    std::vector<size_t>     nty;  // tile y coordinates in next level
    std::vector<FileOffset> no;   // offsets in next level

    bool                    stopArea; // to break out of iteration

    minlon+=180;
    maxlon+=180;
    minlat+=90;
    maxlat+=90;

    // Clear result datastructures
    wayAreaOffsets.clear();
    relationAreaOffsets.clear();

    // Make the vector preallocate memory for the expected data size
    // This should void reallocation
    wayAreaOffsets.reserve(std::min(100000u,(uint32_t)maxAreaCount));
    relationAreaOffsets.reserve(std::min(100000u,(uint32_t)maxAreaCount));

    //
    // Areas
    //

    ctx.clear();
    cty.clear();
    co.clear();

    ctx.push_back(0);
    cty.push_back(0);
    co.push_back(topLevelOffset);

    // For all levels:
    // * Take the tiles and offsets of the last level
    // * Calculate the new tiles and offsets that still interfer with given area
    // * Add the new offsets to the list of offsets and finish if we have
    //   reached maxLevel or maxAreaCount.
    // * copy no, ntx, nty to ctx, cty, co and go to next iteration
    stopArea=false;
    for (size_t level=0;
         !stopArea &&
         level<=this->maxLevel &&
         !co.empty() &&
         level<=maxAreaLevel;
         level++) {
      ntx.clear();
      nty.clear();
      no.clear();

      for (size_t i=0; !stopArea && i<co.size(); i++) {
        size_t cx;
        size_t cy;
        double x;
        double y;
        IndexCache::CacheRef cell;

        if (!GetIndexCell(level,co[i],cell)) {
          std::cerr << "Cannot find offset " << co[i] << " in level " << level << ", => aborting!" << std::endl;
          return false;
        }

        // TODO: First collect all areas for a level, then - after the level is scanned -
        // add it to the result

        if (wayAreaOffsets.size()+relationAreaOffsets.size()+
                 cell->value.areas.size()+
                 cell->value.relAreas.size()>=maxAreaCount) {
          stopArea=true;
          continue;
        }

        for (std::vector<IndexEntry>::const_iterator entry=cell->value.areas.begin();
             entry!=cell->value.areas.end();
             ++entry) {
          if (types.IsTypeSet(entry->type)) {
            wayAreaOffsets.push_back(entry->offset);
          }
        }

        for (std::vector<IndexEntry>::const_iterator entry=cell->value.relAreas.begin();
             entry!=cell->value.relAreas.end();
             ++entry) {
          if (types.IsTypeSet(entry->type)) {
            relationAreaOffsets.push_back(entry->offset);
          }
        }

        cx=ctx[i]*2;
        cy=cty[i]*2;

        if (cell->value.children[0]!=0) {
          // top left

          x=cx*cellWidth[level+1];
          y=(cy+1)*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            ntx.push_back(cx);
            nty.push_back(cy+1);
            no.push_back(cell->value.children[0]);
          }
        }

        if (cell->value.children[1]!=0) {
          // top right
          x=(cx+1)*cellWidth[level+1];
          y=(cy+1)*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            ntx.push_back(cx+1);
            nty.push_back(cy+1);
            no.push_back(cell->value.children[1]);
          }
        }

        if (cell->value.children[2]!=0) {
          // bottom left
          x=cx*cellWidth[level+1];
          y=cy*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            ntx.push_back(cx);
            nty.push_back(cy);
            no.push_back(cell->value.children[2]);
          }
        }

        if (cell->value.children[3]!=0) {
          // bottom right
          x=(cx+1)*cellWidth[level+1];
          y=cy*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            ntx.push_back(cx+1);
            nty.push_back(cy);
            no.push_back(cell->value.children[3]);
          }
        }
      }

      ctx=ntx;
      cty=nty;
      co=no;
    }

    //std::cout << "Found " << wayAreaOffsets.size() << "/" << relationAreaOffsets.size() << " offsets in '" << filepart << "'" << std::endl;

    return true;
  }

  void AreaAreaIndex::DumpStatistics()
  {
    indexCache.DumpStatistics(filepart.c_str(),IndexCacheValueSizer());
  }
}

