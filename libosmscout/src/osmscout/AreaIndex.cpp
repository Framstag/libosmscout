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

  AreaIndex::AreaIndex()
  : filepart("area.idx")
  {
    // no code
  }

  bool AreaIndex::Load(const std::string& path)
  {
    FileScanner scanner;
    std::string file=path+"/"+filepart;

    if (!scanner.Open(file)) {
      std::cerr << "Cannot open file '" << file << "'" << std::endl;
      return false;
    }

    if (!scanner.ReadNumber(maxLevel)) {
      std::cerr << "Cannot read data" << std::endl;
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
        std::cerr << "Cannot read data, level " << l << std::endl;
        return false;
      }

      std::cout << "Reading " << entries << " entries for level " << l << std::endl;

      for (size_t i=0; i<entries; i++) {
        FileOffset offset;
        IndexEntry entry;

        if (!scanner.GetPos(offset)) {
          std::cerr << "Cannot read index data, level " << l << " entry " << i << std::endl;
          return false;
        }

        if (l<maxLevel) {
          for (size_t c=0; c<4; c++) {
            if (!scanner.ReadNumber(entry.children[c])) {
              std::cerr << "Cannot read index data, level " << l << " entry " << i << std::endl;
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

        // Ways

        uint32_t typeCount;
        uint32_t offsetCount;

        if (!scanner.ReadNumber(typeCount)) {
          std::cerr << "Cannot read index way data, level " << l << " entry " << i << std::endl;
          return false;
        }

        for (size_t t=0; t<typeCount; t++) {
          TypeId   type;

          if (!scanner.ReadNumber(type)) {
            std::cerr << "Cannot read index way data, level " << l << " entry " << i << std::endl;
            return false;
          }

          if (!scanner.ReadNumber(offsetCount)) {
            std::cerr << "Cannot read index way data, level " << l << " entry " << i << std::endl;
            return false;
          }

          for (size_t c=0; c<offsetCount; c++) {
            FileOffset o;

            if (!scanner.Read(o)) {
              std::cerr << "Cannot read index way data, level " << l << " entry " << i << std::endl;
              return false;
            }

            entry.ways[type].push_back(o);
          }
        }

        // Relation ways

        if (!scanner.ReadNumber(typeCount)) {
          std::cerr << "Cannot read index way relation data, level " << l << " entry " << i << std::endl;
          return false;
        }

        for (size_t t=0; t<typeCount; t++) {
          TypeId type;

          if (!scanner.ReadNumber(type)) {
            std::cerr << "Cannot read index way relation data, level " << l << " entry " << i << std::endl;
            return false;
          }

          if (!scanner.ReadNumber(offsetCount)) {
            std::cerr << "Cannot read index way relation data, level " << l << " entry " << i << std::endl;
            return false;
          }

          for (size_t c=0; c<offsetCount; c++) {
            FileOffset o;

            if (!scanner.Read(o)) {
              std::cerr << "Cannot read index way relation data, level " << l << " entry " << i << std::endl;
              return false;
            }

            entry.relWays[type].push_back(o);
          }
        }

        // Areas

        if (!scanner.ReadNumber(offsetCount)) {
          std::cerr << "Cannot read index area data, level " << l << " entry " << i << std::endl;
          return false;
        }

        for (size_t c=0; c<offsetCount; c++) {
          FileOffset o;

          if (!scanner.Read(o)) {
            std::cerr << "Cannot read index area data, level " << l << " entry " << i << std::endl;
            return false;
          }

          entry.areas.push_back(o);
        }

        // Relation areas

        if (!scanner.ReadNumber(offsetCount)) {
          std::cerr << "Cannot read index area relation data, level " << l << " entry " << i << std::endl;
          return false;
        }

        for (size_t c=0; c<offsetCount; c++) {
          FileOffset o;

          if (!scanner.Read(o)) {
            std::cerr << "Cannot read index area relation data, level " << l << " entry " << i << std::endl;
            return false;
          }

          entry.relAreas.push_back(o);
        }

        index[l][offset]=entry;
      }

      l--;
    }

    assert(index[0].size()==1);

    return !scanner.HasError() && scanner.Close();
  }

  bool AreaIndex::GetOffsets(const StyleConfig& styleConfig,
                             double minlon,
                             double minlat,
                             double maxlon,
                             double maxlat,
                             size_t maxAreaLevel,
                             size_t maxAreaCount,
                             const std::vector<TypeId>& wayTypes,
                             size_t maxWayCount,
                             std::set<FileOffset>& wayWayOffsets,
                             std::set<FileOffset>& relationWayOffsets,
                             std::set<FileOffset>& wayAreaOffsets,
                             std::set<FileOffset>& relationAreaOffsets) const
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

    wayWayOffsets.clear();
    relationAreaOffsets.clear();
    wayWayOffsets.clear();
    relationAreaOffsets.clear();

    stopWay=false;
    for (size_t t=0; !stopWay && t<wayTypes.size(); t++) {
      ctx.clear();
      cty.clear();
      co.clear();

      newWayWayOffsets.clear();
      newRelationWayOffsets.clear();

      ctx.push_back(0);
      cty.push_back(0);
      co.push_back(index[0].begin()->first);

      // For all levels:
      // * Take the tiles and offsets of the last level
      // * Calculate the new tiles and offsets that still interfer with given area
      // * Add the new offsets to the list of offsets and finish if we have
      //   reached maxCount.
      // * copy no, ntx, nty to ctx, cty, co and go to next iteration
      stopWay=false;
      for (size_t level=0;
           !stopWay &&
           level<=this->maxLevel && co.size()>0;
           level++) {
        ntx.clear();
        nty.clear();
        no.clear();

        for (size_t i=0; !stopWay && i<co.size(); i++) {
          size_t cx;
          size_t cy;
          double x;
          double y;
          IndexLevel::const_iterator entry=index[level].find(co[i]);

          if (entry==index[level].end()) {
            std::cerr << "Cannot find offset " << co[i] << " in level " << level << ", => aborting!" << std::endl;
            return false;
          }

          std::map<TypeId,std::vector<FileOffset> >::const_iterator wayTypeOffsets=entry->second.ways.find(wayTypes[t]);
          std::map<TypeId,std::vector<FileOffset> >::const_iterator relationTypeOffsets=entry->second.relWays.find(wayTypes[t]);

          size_t newWaysCount=0;
          size_t newRelsCount=0;

          if (wayTypeOffsets!=entry->second.ways.end()) {
            newWaysCount=wayTypeOffsets->second.size();
          }

          if (relationTypeOffsets!=entry->second.relWays.end()) {
            newRelsCount=relationTypeOffsets->second.size();
          }

          if (newWaysCount>0 || newRelsCount>0) {
            if (wayWayOffsets.size()+relationWayOffsets.size()+
                newWayWayOffsets.size()+
                newRelationWayOffsets.size()+
                newWaysCount+
                newRelsCount>maxWayCount) {
              std::cout << "Maximum limit hit: " << wayWayOffsets.size() << "+" << relationWayOffsets.size() << "+" << newWayWayOffsets.size() << "+" << newRelationWayOffsets.size() << "+" << newWaysCount << "+" << newRelsCount << ">" << maxWayCount << std::endl;
              stopWay=true;
              break;
            }
            else {
              if (wayTypeOffsets!=entry->second.ways.end()) {
                for (std::vector<FileOffset>::const_iterator o=wayTypeOffsets->second.begin();
                     o!=wayTypeOffsets->second.end();
                     ++o) {
                  newWayWayOffsets.push_back(*o);
                }
              }

              if (relationTypeOffsets!=entry->second.relWays.end()) {
                for (std::vector<FileOffset>::const_iterator o=relationTypeOffsets->second.begin();
                     o!=relationTypeOffsets->second.end();
                     ++o) {
                  newRelationWayOffsets.push_back(*o);
                }
              }
            }
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

      for (std::vector<FileOffset>::const_iterator o=newWayWayOffsets.begin();
           o!=newWayWayOffsets.end();
           ++o) {
        wayWayOffsets.insert(*o);
      }

      for (std::vector<FileOffset>::const_iterator o=newRelationWayOffsets.begin();
           o!=newRelationWayOffsets.end();
           ++o) {
        relationWayOffsets.insert(*o);
      }
    }

    ctx.clear();
    cty.clear();
    co.clear();

    ctx.push_back(0);
    cty.push_back(0);
    co.push_back(index[0].begin()->first);

    // For all levels:
    // * Take the tiles and offsets of the last level
    // * Calculate the new tiles and offsets that still interfer with given area
    // * Add the new offsets to the list of offsets and finish if we have
    //   reached maxCount.
    // * copy no, ntx, nty to ctx, cty, co and go to next iteration
    stopArea=false;
    for (size_t level=0;
         !stopArea &&
         level<=this->maxLevel && co.size()>0;
         level++) {
      ntx.clear();
      nty.clear();
      no.clear();

      for (size_t i=0; !stopArea && i<co.size(); i++) {
        size_t cx;
        size_t cy;
        double x;
        double y;
        IndexLevel::const_iterator entry=index[level].find(co[i]);

        if (entry==index[level].end()) {
          std::cerr << "Cannot find offset " << co[i] << " in level " << level << ", => aborting!" << std::endl;
          return false;
        }

        if (level>maxAreaLevel) {
          stopArea=true;
        }
        else if (wayAreaOffsets.size()+relationAreaOffsets.size()+
                 entry->second.areas.size()+
                 entry->second.relAreas.size()>=maxAreaCount) {
          stopArea=true;
        }
        else {
          for (std::vector<FileOffset>::const_iterator o=entry->second.areas.begin();
               o!=entry->second.areas.end();
               ++o) {
            wayAreaOffsets.insert(*o);
          }

          for (std::vector<FileOffset>::const_iterator o=entry->second.relAreas.begin();
               o!=entry->second.relAreas.end();
               ++o) {
            relationAreaOffsets.insert(*o);
          }
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

    std::cout << "Found " << wayWayOffsets.size() << "/" << relationWayOffsets.size() << "/"
    << wayAreaOffsets.size() << "/" << relationAreaOffsets.size() << " offsets in '" << filepart << "' with maximum level " << maxLevel << std::endl;

    return true;
  }

  void AreaIndex::DumpStatistics()
  {
    // TODO
  }
}

