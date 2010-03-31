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

#include <osmscout/AreaWayIndex.h>

#include <cassert>
#include <cmath>

#include <osmscout/FileScanner.h>
#include <osmscout/Util.h>
#include <iostream>

namespace osmscout {

  bool AreaWayIndex::Load(const std::string& path)
  {
    FileScanner scanner;
    std::string file=path+"/"+"areaway.idx";

    if (!scanner.Open(file)) {
      std::cerr << "Cannot open file areaway.idx" << std::endl;
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
      size_t   children=0;
      uint32_t entries;

      if (!scanner.ReadNumber(entries)) {
        std::cerr << "Cannot read data (2)" << std::endl;
        return false;
      }

      std::cout << "Reading " << entries << " entries for level " << l << std::endl;

      for (size_t i=0; i<entries; i++) {
        FileOffset offset;
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

        uint32_t typeCount;

        if (!scanner.ReadNumber(typeCount)) {
          std::cerr << "Cannot read data (5)" << std::endl;
          return false;
        }

        for (size_t t=0; t<typeCount; t++) {
          TypeId   type;
          uint32_t offsetCount;

          if (!scanner.ReadNumber(type)) {
            std::cerr << "Cannot read data (6)" << std::endl;
            return false;
          }

          if (!scanner.ReadNumber(offsetCount)) {
            std::cerr << "Cannot read data (7)" << std::endl;
            return false;
          }

          //std::cout << l << " " << i << " " << type << " " << offsetCount << std::endl;

          entry.dataOffsets[type].reserve(offsetCount);
          for (size_t c=0; c<offsetCount; c++) {
            FileOffset o;

            if (!scanner.ReadNumber(o)) {
              std::cerr << "Cannot read data (8), level " << l << " entry " << i << std::endl;
              return false;
            }

            entry.dataOffsets[type].push_back(o);
          }
        }
        index[l][offset]=entry;
      }

      l--;
    }

    assert(index[0].size()==1);

    return !scanner.HasError() && scanner.Close();
  }

  void AreaWayIndex::GetOffsets(const StyleConfig& styleConfig,
                                 double minlon, double minlat,
                                 double maxlon, double maxlat,
                                 const std::vector<TypeId>& types,
                                 size_t maxCount,
                                 std::set<FileOffset>& offsets) const
  {
    //std::cout << "ArayWayIndex2: " << types.size() << " " << maxCount << std::endl;
    offsets.clear();

    std::vector<FileOffset> newOffsets;

    minlon+=180;
    maxlon+=180;
    minlat+=90;
    maxlat+=90;

    for (size_t t=0; t<types.size(); t++) {
      std::vector<size_t>     ctx; // tile x coordinates in this level
      std::vector<size_t>     cty; // tile y coordinates in this level
      std::vector<FileOffset> co;  // offsets in this level

      std::vector<size_t>     ntx; // tile x coordinates in next level
      std::vector<size_t>     nty; // tile y coordinates in next level
      std::vector<FileOffset> no;  // offsets in next level

      ctx.push_back(0);
      cty.push_back(0);
      co.push_back(index[0].begin()->first);

      newOffsets.clear();

      // For all levels:
      // * Take the tiles and offsets of the last level
      // * Calculate the new tiles and offsets that still interfer with given area
      // * Add the new way offsets to the list of way offset and finish if we have
      //   reached maxCount.
      // * copy no, ntx, nty to ctx, cty, co and go to next iteration
      for (size_t level=0; level<=maxLevel && level<=this->maxLevel && co.size()>0; level++) {
        //std::cout << "Level: " << level << std::endl;
        ntx.clear();
        nty.clear();
        no.clear();

        for (size_t i=0; i<co.size(); i++) {
          IndexLevel::const_iterator entry=index[level].find(co[i]);

          if (entry==index[level].end()) {
            std::cerr << "Cannot find offset " << co[i] << " in level " << level << ", => aborting!" << std::endl;
            return;
          }

          std::map<TypeId,std::vector<FileOffset> >::const_iterator typeOffsets=entry->second.dataOffsets.find(types[t]);

          if (typeOffsets!=entry->second.dataOffsets.end()) {
            if (offsets.size()+newOffsets.size()+typeOffsets->second.size()>maxCount) {
              std::cout << "Maximum limit hit: " << offsets.size() << "+" << newOffsets.size() << "+" << typeOffsets->second.size() << ">" << maxCount << std::endl;
              std::cout << "Found " << offsets.size() << " way offsets in area way index with maximum level " << maxLevel << std::endl;
              return;
            }

            for (std::vector<FileOffset>::const_iterator o=typeOffsets->second.begin();
                 o!=typeOffsets->second.end();
                 ++o) {
              newOffsets.push_back(*o);
            }
          }

          size_t cx=ctx[i]*2;
          size_t cy=cty[i]*2;

          if (entry->second.children[0]!=0) {
            // top left

            double x=cx*cellWidth[level+1];
            double y=(cy+1)*cellHeight[level+1];

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
            double x=(cx+1)*cellWidth[level+1];
            double y=(cy+1)*cellHeight[level+1];

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
            double x=cx*cellWidth[level+1];
            double y=cy*cellHeight[level+1];

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
            double x=(cx+1)*cellWidth[level+1];
            double y=cy*cellHeight[level+1];

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

      for (std::vector<FileOffset>::const_iterator o=newOffsets.begin();
           o!=newOffsets.end();
           ++o) {
        offsets.insert(*o);
      }
    }

    std::cout << "Found " << offsets.size() << " ways offsets in area way index with maximum level " << maxLevel << std::endl;
  }

  void AreaWayIndex::DumpStatistics()
  {
  }
}

