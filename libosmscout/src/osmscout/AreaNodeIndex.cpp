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

#include <osmscout/AreaNodeIndex.h>

#include <cerrno>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>

namespace osmscout {

  AreaNodeIndex::Leaf::Leaf()
  {
    children[0]=0;
    children[1]=0;
    children[2]=0;
    children[3]=0;
  }

  AreaNodeIndex::AreaNodeIndex(size_t cacheSize)
  : filepart("areanode.idx"),
    leafCache(cacheSize)
  {
    // no code
  }

  bool AreaNodeIndex::LoadAreaNodeIndex(const std::string& path)
  {
    datafilename=path+"/"+filepart;

    if (!scanner.Open(datafilename)) {
      return false;
    }

    uint32_t types;
    uint32_t maxType;
    uint32_t maxLevel;

    // The number of draw types we have an index for
    scanner.ReadNumber(types);    // Number of entries

    scanner.ReadNumber(maxType);
    scanner.ReadNumber(maxLevel);

    std::cout << types << " area node index entries..." << std::endl;

    // Calculate the size of a cell in each index level
    cellWidth.resize(maxLevel+1);
    cellHeight.resize(maxLevel+1);

    for (size_t i=0; i<cellWidth.size(); i++) {
      cellWidth[i]=360.0/pow(2.0,(int)i);
    }

    for (size_t i=0; i<cellHeight.size(); i++) {
      cellHeight[i]=180.0/pow(2.0,(int)i);
    }

    topLevelOffsets.resize(maxType+1);

    for (size_t i=0; i<topLevelOffsets.size(); i++) {
      topLevelOffsets[i]=0;
    }

    for (size_t i=0; i<types; i++) {
      TypeId     type;
      FileOffset offset;

      scanner.Read(type);
      scanner.Read(offset);

      topLevelOffsets[type]=offset;
    }

    return !scanner.HasError();
  }

  bool AreaNodeIndex::GetIndexEntry(FileOffset offset,
                                    LeafCache::CacheRef& cacheRef) const
  {
    if (!leafCache.GetEntry(offset,cacheRef)) {
      LeafCache::CacheEntry cacheEntry(offset);

      cacheRef=leafCache.SetEntry(cacheEntry);

      if (!scanner.IsOpen()) {
        if (!scanner.Open(datafilename)) {
          std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
          return false;
        }
      }

      scanner.SetPos(offset);

      uint32_t     offsetCount;

      scanner.ReadNumber(cacheRef->value.children[0]);
      scanner.ReadNumber(cacheRef->value.children[1]);
      scanner.ReadNumber(cacheRef->value.children[2]);
      scanner.ReadNumber(cacheRef->value.children[3]);

      scanner.ReadNumber(offsetCount);

      cacheRef->value.offsets.resize(offsetCount);

      for (size_t i=0; i<offsetCount; i++) {
        scanner.ReadNumber(cacheRef->value.offsets[i]);
      }
    }

    return !scanner.HasError();
  }


  bool AreaNodeIndex::GetOffsets(const StyleConfig& styleConfig,
                                 double minlon,
                                 double minlat,
                                 double maxlon,
                                 double maxlat,
                                 const std::vector<TypeId>& types,
                                 size_t maxNodeCount,
                                 std::vector<FileOffset>& nodeOffsets) const
  {
    std::vector<FileOffset> newNodeOffsets;
    bool                    stop;
    std::vector<size_t>     ctx;  // tile x coordinates in this level
    std::vector<size_t>     cty;  // tile y coordinates in this level
    std::vector<FileOffset> co;   // offsets in this level

    std::vector<size_t>     ntx;  // tile x coordinates in next level
    std::vector<size_t>     nty;  // tile y coordinates in next level
    std::vector<FileOffset> no;   // offsets in next level

    nodeOffsets.clear();
    nodeOffsets.reserve(std::min(100000u,(uint32_t)maxNodeCount));
    newNodeOffsets.reserve(std::min(100000u,(uint32_t)maxNodeCount));

    minlon+=180;
    maxlon+=180;
    minlat+=90;
    maxlat+=90;

    stop=false;
    for (std::vector<TypeId>::const_iterator type=types.begin();
         !stop && type!=types.end();
         ++type) {
      if (*type>=topLevelOffsets.size() ||
          topLevelOffsets[*type]==0) {
        continue;
      }

      newNodeOffsets.clear();

      // Start at the top of the index
      ctx.clear();
      cty.clear();
      co.clear();
      ctx.push_back(0);
      cty.push_back(0);
      co.push_back(topLevelOffsets[*type]);

      for (size_t level=0;
           !stop &&
           co.size()>0;
           level++) {
        // Clear the list of new index tiles
        ntx.clear();
        nty.clear();
        no.clear();

        // For all tiles...
        for (size_t i=0; !stop && i<co.size(); i++) {
          size_t cx;
          size_t cy;
          double x;
          double y;

          LeafCache::CacheRef entry;

          if (!GetIndexEntry(co[i],entry)) {
            std::cerr << "Cannot find offset " << co[i] << " in level " << level << ", => aborting!" << std::endl;
            return false;
          }

          // Evaluate data

          if (nodeOffsets.size()+
              newNodeOffsets.size()+entry->value.offsets.size()>maxNodeCount) {
            std::cout << "Maximum node limit hit: " << nodeOffsets.size();
            std::cout << "+" << newNodeOffsets.size();
            std::cout << "+" << entry->value.offsets.size();
            std::cout << ">" << maxNodeCount << " for type " << types[*type] << std::endl;
            stop=true;
            break;
          }

          for (std::vector<FileOffset>::const_iterator offset=entry->value.offsets.begin();
               offset!=entry->value.offsets.end();
               ++offset) {
            newNodeOffsets.push_back(*offset);
          }

          // Now calculate the new index tiles for the next level

          cx=ctx[i]*2;
          cy=cty[i]*2;

          if (entry->value.children[0]!=0) {
            // top left

            x=cx*cellWidth[level+1];
            y=(cy+1)*cellHeight[level+1];

            if (!(x>maxlon ||
                  y>maxlat ||
                  x+cellWidth[level+1]<minlon ||
                  y+cellHeight[level+1]<minlat)) {
              ntx.push_back(cx);
              nty.push_back(cy+1);
              no.push_back(entry->value.children[0]);
            }
          }

          if (entry->value.children[1]!=0) {
            // top right
            x=(cx+1)*cellWidth[level+1];
            y=(cy+1)*cellHeight[level+1];

            if (!(x>maxlon ||
                  y>maxlat ||
                  x+cellWidth[level+1]<minlon ||
                  y+cellHeight[level+1]<minlat)) {
              ntx.push_back(cx+1);
              nty.push_back(cy+1);
              no.push_back(entry->value.children[1]);
            }
          }

          if (entry->value.children[2]!=0) {
            // bottom left
            x=cx*cellWidth[level+1];
            y=cy*cellHeight[level+1];

            if (!(x>maxlon ||
                  y>maxlat ||
                  x+cellWidth[level+1]<minlon ||
                  y+cellHeight[level+1]<minlat)) {
              ntx.push_back(cx);
              nty.push_back(cy);
              no.push_back(entry->value.children[2]);
              }
          }

          if (entry->value.children[3]!=0) {
            // bottom right
            x=(cx+1)*cellWidth[level+1];
            y=cy*cellHeight[level+1];

            if (!(x>maxlon ||
                  y>maxlat ||
                  x+cellWidth[level+1]<minlon ||
                  y+cellHeight[level+1]<minlat)) {
              ntx.push_back(cx+1);
              nty.push_back(cy);
              no.push_back(entry->value.children[3]);
            }
          }
        }

        ctx=ntx;
        cty=nty;
        co=no;
      }

      if (!stop) {
        for (std::vector<FileOffset>::const_iterator offset=newNodeOffsets.begin();
             offset!=newNodeOffsets.end();
             ++offset) {
          nodeOffsets.push_back(*offset);
        }
      }
    }

    return true;
  }

  void AreaNodeIndex::DumpStatistics()
  {
    leafCache.DumpStatistics(filepart.c_str(),LeafCacheValueSizer());
  }
}

