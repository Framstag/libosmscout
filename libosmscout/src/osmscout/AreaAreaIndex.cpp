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

#include <osmscout/AreaAreaIndex.h>

#include <cassert>
#include <cmath>

#include <osmscout/FileScanner.h>
#include <osmscout/Util.h>
#include <iostream>

bool AreaAreaIndex::Load(const std::string& path)
{
  FileScanner scanner;
  std::string file=path+"/"+"areaarea.idx";

  if (!scanner.Open(file)) {
    std::cerr << "Cannot open file areaarea.idx" << std::endl;
    return false;
  }

  if (!scanner.ReadNumber(maxLevel)) {
    std::cerr << "Cannot read data (1)" << std::endl;
    return false;
  }

  std::cout << "Max level is: " << maxLevel << std::endl;

  cellWidth.resize(maxLevel+1);
  cellHeight.resize(maxLevel+1);

  for (size_t i=0; i<cellWidth.size(); i++) {
    cellWidth[i]=360/pow(2,i);
  }

  for (size_t i=0; i<cellHeight.size(); i++) {
    cellHeight[i]=180/pow(2,i);
  }

  index.resize(maxLevel+1);

  int l=maxLevel;

  while (l>=0) {
    size_t children=0;
    size_t entries;

    if (!scanner.ReadNumber(entries)) {
      std::cerr << "Cannot read data (2)" << std::endl;
      return false;
    }

    std::cout << "Reading " << entries << " entries for level " << l << std::endl;

    for (size_t i=0; i<entries; i++) {
      long       offset;
      IndexEntry entry;

      if (!scanner.GetPos(offset)) {
        std::cerr << "Cannot read data (3)" << std::endl;
        return false;
      }

      if (l<maxLevel) {
        for (size_t c=0; c<4; c++) {
          if (!scanner.ReadNumber(entry.children[c])) {
            std::cerr << "Cannot read data (4)" << std::endl;
            return false;
          }

          if (entry.children[c]!=0) {
            children++;
          }
        }
      }
      else {
        for (size_t c=0; c<4; c++) {
          entry.children[c]=0;
        }
      }

      size_t offsetCount;

      if (!scanner.ReadNumber(offsetCount)) {
        std::cerr << "Cannot read data (5)" << std::endl;
        return false;
      }

      entry.dataOffsets.reserve(offsetCount);
      for (size_t c=0; c<offsetCount; c++) {
        long o;

        if (!scanner.ReadNumber(o)) {
          std::cerr << "Cannot read data (6), level " << l << " entry " << i << std::endl;
          return false;
        }

        entry.dataOffsets.push_back(o);
      }

      index[l][offset]=entry;
    }

    l--;
  }

  assert(index[0].size()==1);

  return !scanner.HasError() && scanner.Close();
}

void AreaAreaIndex::GetOffsets(const StyleConfig& styleConfig,
                               double minlon, double minlat,
                               double maxlon, double maxlat,
                               size_t maxLevel,
                               size_t maxCount,
                               std::set<long>& offsets) const
{
  std::vector<size_t> ctx; // tile x coordinates in this level
  std::vector<size_t> cty; // tile y coordinates in this level
  std::vector<long>   co;  // offsets in this level

  std::vector<size_t> ntx; // tile x coordinates in next level
  std::vector<size_t> nty; // tile y coordinates in next level
  std::vector<long>   no;  // offsets in next level

  minlon+=180;
  maxlon+=180;
  minlat+=90;
  maxlat+=90;

  offsets.clear();

  ctx.push_back(0);
  cty.push_back(0);
  co.push_back(index[0].begin()->first);

  // For all levels:
  // * Take the tiles and offsets of the last level
  // * Calculate the new tiles and offsets that still interfer with given area
  // * Add the new way offsets to the list of way offset and finish if we have
  //   reached maxCount.
  // * copy no, ntx, nty to ctx, cty, co and go to next iteration
  for (size_t level=0; level<=maxLevel && level<=this->maxLevel && co.size()>0; level++) {
    ntx.clear();
    nty.clear();
    no.clear();

    for (size_t i=0; i<co.size(); i++) {
      size_t cx;
      size_t cy;
      double x;
      double y;
      IndexLevel::const_iterator entry=index[level].find(co[i]);

      if (entry==index[level].end()) {
        std::cerr << "Cannot find offset " << co[i] << " in level " << level << ", => aborting!" << std::endl;
        return;
      }

      if (offsets.size()+entry->second.dataOffsets.size()>maxCount) {
        std::cout << "Found " << offsets.size() << " ways ids in area area index with maximum level " << maxLevel << std::endl;
        return;
      }

      for (std::vector<long>::const_iterator o=entry->second.dataOffsets.begin();
           o!=entry->second.dataOffsets.end();
           ++o) {
        offsets.insert(*o);
      }

      cx=ctx[i]*2;
      cy=cty[i]*2;

      if (entry->second.children[0]!=0) {
        // top left

        x=cx*cellWidth[level+1];
        y=(cy+1)*cellHeight[level+1];

        if (!(x>maxlon ||
              y>maxlat ||
              x+cellWidth[level+1]<minlon ||
              y+cellHeight[level+1]<minlat)) {
          ntx.push_back(cx);
          nty.push_back(cy+1);
          no.push_back(entry->second.children[0]);
        }
      }

      if (entry->second.children[1]!=0) {
        // top right
        x=(cx+1)*cellWidth[level+1];
        y=(cy+1)*cellHeight[level+1];

        if (!(x>maxlon ||
              y>maxlat ||
              x+cellWidth[level+1]<minlon ||
              y+cellHeight[level+1]<minlat)) {
          ntx.push_back(cx+1);
          nty.push_back(cy+1);
          no.push_back(entry->second.children[1]);
        }
      }

      if (entry->second.children[2]!=0) {
        // bottom left
        x=cx*cellWidth[level+1];
        y=cy*cellHeight[level+1];

        if (!(x>maxlon ||
              y>maxlat ||
              x+cellWidth[level+1]<minlon ||
              y+cellHeight[level+1]<minlat)) {
          ntx.push_back(cx);
          nty.push_back(cy);
          no.push_back(entry->second.children[2]);
        }
      }

      if (entry->second.children[3]!=0) {
        // bottom right
        x=(cx+1)*cellWidth[level+1];
        y=cy*cellHeight[level+1];

        if (!(x>maxlon ||
              y>maxlat ||
              x+cellWidth[level+1]<minlon ||
              y+cellHeight[level+1]<minlat)) {
          ntx.push_back(cx+1);
          nty.push_back(cy);
          no.push_back(entry->second.children[3]);
        }
      }
    }

    ctx=ntx;
    cty=nty;
    co=no;
  }

  std::cout << "Found " << offsets.size() << " ways ids in area area index with maximum level " << maxLevel << std::endl;
}

void AreaAreaIndex::DumpStatistics()
{
}
