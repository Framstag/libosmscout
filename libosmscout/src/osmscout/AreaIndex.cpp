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

#include <osmscout/AreaIndex.h>

namespace osmscout {

  AreaIndex::AreaIndex(size_t cacheSize)
  : filepart("area.idx"),
    indexCache(cacheSize)
  {
    // no code
  }

  bool AreaIndex::GetIndexEntry(uint32_t level,
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

      uint32_t typeCount;
      uint32_t offsetCount;

      if (!scanner.ReadNumber(typeCount)) {
        std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
        return false;
      }

      for (uint32_t t=0; t<typeCount; t++) {
        TypeId type;

        if (!scanner.ReadNumber(type)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }

        if (!scanner.ReadNumber(offsetCount)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }

        if (offsetCount>0) {
          std::vector<FileOffset> &vector=cacheRef->value.ways[type];

          vector.resize(offsetCount);

          for (size_t c=0; c<offsetCount; c++) {
            if (!scanner.Read(vector[c])) {
              std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
              return false;
            }
          }
        }
      }

      // Now read the way relation offsets by type in this index entry

      if (!scanner.ReadNumber(typeCount)) {
        std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
        return false;
      }

      for (size_t t=0; t<typeCount; t++) {
        TypeId type;

        if (!scanner.ReadNumber(type)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }

        if (!scanner.ReadNumber(offsetCount)) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }

        if (offsetCount>0) {
          std::vector<FileOffset> &vector=cacheRef->value.relWays[type];

          vector.resize(offsetCount);

          for (size_t c=0; c<offsetCount; c++) {
            if (!scanner.Read(vector[c])) {
              std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
              return false;
            }
          }
        }
      }

      // Areas

      if (!scanner.ReadNumber(offsetCount)) {
        std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
        return false;
      }

      cacheRef->value.areas.resize(offsetCount);

      for (size_t c=0; c<offsetCount; c++) {
        if (!scanner.Read(cacheRef->value.areas[c])) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }
      }

      // Relation areas

      if (!scanner.ReadNumber(offsetCount)) {
        std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
        return false;
      }

      cacheRef->value.relAreas.resize(offsetCount);

      for (size_t c=0; c<offsetCount; c++) {
        if (!scanner.Read(cacheRef->value.relAreas[c])) {
          std::cerr << "Cannot read index data for level " << level << " at offset " << offset << std::endl;
          return false;
        }
      }
    }

    return true;
  }

  bool AreaIndex::Load(const std::string& path)
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

    std::cout << "Max level is: " << maxLevel << std::endl;

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

  bool AreaIndex::GetOffsets(const StyleConfig& styleConfig,
                             double minlon,
                             double minlat,
                             double maxlon,
                             double maxlat,
                             size_t maxWayLevel,
                             size_t maxAreaLevel,
                             size_t maxAreaCount,
                             const std::vector<TypeId>& wayTypes,
                             size_t maxWayCount,
                             std::vector<FileOffset>& wayWayOffsets,
                             std::vector<FileOffset>& relationWayOffsets,
                             std::vector<FileOffset>& wayAreaOffsets,
                             std::vector<FileOffset>& relationAreaOffsets) const
  {
    std::vector<size_t>     ctx;  // tile x coordinates in this level
    std::vector<size_t>     cty;  // tile y coordinates in this level
    std::vector<FileOffset> co;   // offsets in this level

    std::vector<size_t>     ntx;  // tile x coordinates in next level
    std::vector<size_t>     nty;  // tile y coordinates in next level
    std::vector<FileOffset> no;   // offsets in next level

    bool                    stopWay; // to break out of iteration
    bool                    stopArea; // to break out of iteration

    std::vector<FileOffset> newWayWayOffsets;
    std::vector<FileOffset> newRelationWayOffsets;

    minlon+=180;
    maxlon+=180;
    minlat+=90;
    maxlat+=90;

    // Clear result datastructures
    wayWayOffsets.clear();
    relationWayOffsets.clear();
    wayAreaOffsets.clear();
    relationAreaOffsets.clear();

    // Make the vector preallocate memory for the expected data size
    // This should void reallocation
    wayWayOffsets.reserve(std::min(100000u,maxWayCount));
    relationWayOffsets.reserve(std::min(100000u,maxWayCount));
    wayAreaOffsets.reserve(std::min(100000u,maxAreaCount));
    relationAreaOffsets.reserve(std::min(100000u,maxAreaCount));

    newWayWayOffsets.reserve(std::min(100000u,maxWayCount));
    newRelationWayOffsets.reserve(std::min(100000u,maxWayCount));

    //
    // Ways
    //

    // For every way type (ordered by priority)...
    stopWay=false;
    for (size_t t=0; !stopWay && t<wayTypes.size(); t++) {
      // Clear lists of offsets we found in this iteration
      // We must store all hits in a temporary storage because we either take
      // all of the hits or none of them for a given type (else we would show
      // more details in the top left cordener than in the bottom right corner of the map)
      newWayWayOffsets.clear();
      newRelationWayOffsets.clear();

      // Start at the top of the index
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
      //   reached maxLevel or maxWayCount.
      // * copy no, ntx, nty to ctx, cty, co and go to next iteration ('n' = 'new')
      for (size_t level=0;
           !stopWay &&
           level<=this->maxLevel && co.size()>0 && level<=maxWayLevel;
           level++) {
        // Clear the list of new index tiles
        ntx.clear();
        nty.clear();
        no.clear();

        // For all tiles...
        for (size_t i=0; !stopWay && i<co.size(); i++) {
          size_t cx;
          size_t cy;
          double x;
          double y;

          IndexCache::CacheRef entry;

          if (!GetIndexEntry(level,co[i],entry)) {
            std::cerr << "Cannot find offset " << co[i] << " in level " << level << ", => aborting!" << std::endl;
            return false;
          }

          // Find the list of way and way relation offsets for the given type
          std::map<TypeId,std::vector<FileOffset> >::const_iterator wayTypeOffsets=entry->value.ways.find(wayTypes[t]);
          std::map<TypeId,std::vector<FileOffset> >::const_iterator relationTypeOffsets=entry->value.relWays.find(wayTypes[t]);

          size_t nw=0;
          if (wayTypeOffsets!=entry->value.ways.end()) {
            nw=wayTypeOffsets->second.size();
          }

          size_t nr=0;
          if (relationTypeOffsets!=entry->value.relWays.end()) {
            nr=relationTypeOffsets->second.size();
          }

          if (wayWayOffsets.size()+relationWayOffsets.size()+
              newWayWayOffsets.size()+
              newRelationWayOffsets.size()+
              nw+nr>maxWayCount) {
            std::cout << "Maximum limit hit: " << wayWayOffsets.size();
            std::cout << "+" << relationWayOffsets.size();
            std::cout << "+" << newWayWayOffsets.size();
            std::cout << "+" << newRelationWayOffsets.size();
            std::cout << "+" << nw;
            std::cout << "+" << nr;
            std::cout << ">" << maxWayCount << " for type " << wayTypes[t] << std::endl;
            stopWay=true;
            break;
          }

          if (wayTypeOffsets!=entry->value.ways.end()) {
            for (std::vector<FileOffset>::const_iterator o=wayTypeOffsets->second.begin();
                 o!=wayTypeOffsets->second.end();
                 ++o) {
              newWayWayOffsets.push_back(*o);
            }
          }

          if (relationTypeOffsets!=entry->value.relWays.end()) {
            for (std::vector<FileOffset>::const_iterator o=relationTypeOffsets->second.begin();
                 o!=relationTypeOffsets->second.end();
                 ++o) {
              newRelationWayOffsets.push_back(*o);
            }
          }

          // No need to calculate the new index tiles, since we stop anyway
          if (level<this->maxLevel) {
            // Now calculate the new index tiles for the next level

            cx=ctx[i]*2;
            cy=cty[i]*2;

            if (entry->value.children[0]!=0) {
              // top left

              x=cx*cellWidth[level+1];
              y=(cy+1)*cellHeight[level+1];

              if (!(x>maxlon+cellWidth[level+1]/2 ||
                    y>maxlat+cellHeight[level+1]/2 ||
                    x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                    y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
                ntx.push_back(cx);
                nty.push_back(cy+1);
                no.push_back(entry->value.children[0]);
              }
            }

            if (entry->value.children[1]!=0) {
              // top right
              x=(cx+1)*cellWidth[level+1];
              y=(cy+1)*cellHeight[level+1];

              if (!(x>maxlon+cellWidth[level+1]/2 ||
                    y>maxlat+cellHeight[level+1]/2 ||
                    x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                    y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
                ntx.push_back(cx+1);
                nty.push_back(cy+1);
                no.push_back(entry->value.children[1]);
              }
            }

            if (entry->value.children[2]!=0) {
              // bottom left
              x=cx*cellWidth[level+1];
              y=cy*cellHeight[level+1];

              if (!(x>maxlon+cellWidth[level+1]/2 ||
                    y>maxlat+cellHeight[level+1]/2 ||
                    x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                    y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
                ntx.push_back(cx);
                nty.push_back(cy);
                no.push_back(entry->value.children[2]);
              }
            }

            if (entry->value.children[3]!=0) {
              // bottom right
              x=(cx+1)*cellWidth[level+1];
              y=cy*cellHeight[level+1];

              if (!(x>maxlon+cellWidth[level+1]/2 ||
                    y>maxlat+cellHeight[level+1]/2 ||
                    x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                    y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
                ntx.push_back(cx+1);
                nty.push_back(cy);
                no.push_back(entry->value.children[3]);
              }
            }
          }
        }

        ctx=ntx;
        cty=nty;
        co=no;
      }

      if (!stopWay) {
        if (newWayWayOffsets.size()>0 || newRelationWayOffsets.size()>0) {
          // Now copy the result for this type into the result
          for (std::vector<FileOffset>::const_iterator o=newWayWayOffsets.begin();
               o!=newWayWayOffsets.end();
               ++o) {
            wayWayOffsets.push_back(*o);
          }

          for (std::vector<FileOffset>::const_iterator o=newRelationWayOffsets.begin();
               o!=newRelationWayOffsets.end();
               ++o) {
            relationWayOffsets.push_back(*o);
          }
        }
      }
    }

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
         level<=this->maxLevel && co.size()>0 && level<=maxAreaLevel;
         level++) {
      ntx.clear();
      nty.clear();
      no.clear();

      for (size_t i=0; !stopArea && i<co.size(); i++) {
        size_t cx;
        size_t cy;
        double x;
        double y;
        IndexCache::CacheRef entry;

        if (!GetIndexEntry(level,co[i],entry)) {
          std::cerr << "Cannot find offset " << co[i] << " in level " << level << ", => aborting!" << std::endl;
          return false;
        }

        // TODO: First collect all areas for a level, then - afte rthe level is scanned -
        // add it to the result

        if (wayAreaOffsets.size()+relationAreaOffsets.size()+
                 entry->value.areas.size()+
                 entry->value.relAreas.size()>=maxAreaCount) {
          stopArea=true;
        }
        else {
          for (std::vector<FileOffset>::const_iterator o=entry->value.areas.begin();
               o!=entry->value.areas.end();
               ++o) {
            wayAreaOffsets.push_back(*o);
          }

          for (std::vector<FileOffset>::const_iterator o=entry->value.relAreas.begin();
               o!=entry->value.relAreas.end();
               ++o) {
            relationAreaOffsets.push_back(*o);
          }
        }

        cx=ctx[i]*2;
        cy=cty[i]*2;

        if (entry->value.children[0]!=0) {
          // top left

          x=cx*cellWidth[level+1];
          y=(cy+1)*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            ntx.push_back(cx);
            nty.push_back(cy+1);
            no.push_back(entry->value.children[0]);
          }
        }

        if (entry->value.children[1]!=0) {
          // top right
          x=(cx+1)*cellWidth[level+1];
          y=(cy+1)*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            ntx.push_back(cx+1);
            nty.push_back(cy+1);
            no.push_back(entry->value.children[1]);
          }
        }

        if (entry->value.children[2]!=0) {
          // bottom left
          x=cx*cellWidth[level+1];
          y=cy*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
            ntx.push_back(cx);
            nty.push_back(cy);
            no.push_back(entry->value.children[2]);
          }
        }

        if (entry->value.children[3]!=0) {
          // bottom right
          x=(cx+1)*cellWidth[level+1];
          y=cy*cellHeight[level+1];

          if (!(x>maxlon+cellWidth[level+1]/2 ||
                y>maxlat+cellHeight[level+1]/2 ||
                x+cellWidth[level+1]<minlon-cellWidth[level+1]/2 ||
                y+cellHeight[level+1]<minlat-cellHeight[level+1]/2)) {
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

    std::cout << "Found " << wayWayOffsets.size() << "/" << relationWayOffsets.size() << "/"
    << wayAreaOffsets.size() << "/" << relationAreaOffsets.size() << " offsets in '" << filepart << "'" << std::endl;

    return true;
  }

  void AreaIndex::DumpStatistics()
  {
    indexCache.DumpStatistics(filepart.c_str(),IndexCacheValueSizer());
  }
}

