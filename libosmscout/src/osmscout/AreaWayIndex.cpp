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

#include <osmscout/AreaWayIndex.h>

#include <fstream>
#include <iostream>
#include <map>

bool AreaWayIndex::LoadAreaWayIndex(const std::string& path)
{
  std::ifstream indexFile;
  std::string   file=path+"/"+"areaway.idx";

  indexFile.open(file.c_str(),std::ios::in|std::ios::binary);

  if (!indexFile) {
    return false;
  }

  size_t drawTypes;

  // The number of draw types we have an index for
  indexFile.read((char*)&drawTypes,sizeof(drawTypes)); // Number of entries

  std::cout << drawTypes << " entries..." << std::endl;

  for (size_t i=0; i<drawTypes; i++) {
    TypeId type;
    size_t tiles;

    indexFile.read((char*)&type,sizeof(type)); // The draw type id
    indexFile.read((char*)&tiles,sizeof(tiles)); // The number of tiles

    for (size_t t=0; t<tiles; t++) {
      IndexEntry entry;
      TileId     tileId;
      size_t     pageCount;

      indexFile.read((char*)&tileId,sizeof(tileId)); // The tile id
      indexFile.read((char*)&entry.nodeCount,sizeof(entry.nodeCount)); // The number of nodes
      indexFile.read((char*)&pageCount,sizeof(pageCount)); // The number of pages

      entry.pages.reserve(pageCount);

      for (size_t p=0; p<pageCount; p++) {
        Page page;

        indexFile.read((char*)&page,sizeof(page)); // The id of the page
        entry.pages.push_back(page);
      }

      areaWayIndex[type][tileId]=entry;
    }
  }

  if (!indexFile) {
    indexFile.close();
    return false;
  }

  indexFile.close();
  return true;
}

size_t AreaWayIndex::GetNodes(TypeId drawType,
                              size_t tileMinX, size_t tileMinY,
                              size_t tileMaxX, size_t tileMaxY) const
{
  size_t nodes=0;

  std::map<TypeId,std::map<TileId,IndexEntry > >::const_iterator drawTypeEntry;

  drawTypeEntry=areaWayIndex.find(drawType);

  if (drawTypeEntry!=areaWayIndex.end()) {
    for (size_t y=tileMinY; y<=tileMaxY; y++) {
      for (size_t x=tileMinX; x<=tileMaxX; x++) {
        std::map<TileId,IndexEntry>::const_iterator tile;
        TileId                                      tileId=GetTileId(x,y);

        tile=drawTypeEntry->second.find(tileId);

        if (tile!=drawTypeEntry->second.end()) {
          nodes+=tile->second.nodeCount;
        }
      }
    }
  }

  return nodes;
}

void AreaWayIndex::GetPages(const StyleConfig& styleConfig,
                            double minlon, double minlat,
                            double maxlon, double maxlat,
                            double magnification,
                            size_t maxPriority,
                            std::set<Page>& pages) const
{
  std::set<TypeId> types;

  styleConfig.GetWayTypesWithMaxPrio(maxPriority,types);
  styleConfig.GetAreaTypesWithMaxPrio(maxPriority,types);

  size_t minTileX=GetTileX(minlon);
  size_t maxTileX=GetTileX(maxlon);
  size_t minTileY=GetTileY(minlat);
  size_t maxTileY=GetTileY(maxlat);

  for (std::set<TypeId>::const_iterator type=types.begin();
       type!=types.end();
       ++type) {

    std::map<TypeId,std::map<TileId,IndexEntry> >::const_iterator typeEntry;

    typeEntry=areaWayIndex.find(*type);

    if (typeEntry!=areaWayIndex.end()) {
      for (size_t y=minTileY; y<=maxTileY; y++) {
        for (size_t x=minTileX; x<=maxTileX; x++) {
          std::map<TileId,IndexEntry>::const_iterator tile;
          TileId                                      tileId=GetTileId(x,y);

          tile=typeEntry->second.find(tileId);

          if (tile!=typeEntry->second.end()) {
            for (size_t j=0; j<tile->second.pages.size(); j++) {
              pages.insert(tile->second.pages[j]);
            }
          }
        }
      }
    }
  }

  std::cout << "Found " << pages.size() << " ways pages in area way index with maximum priority " << maxPriority << std::endl;
}

void AreaWayIndex::DumpStatistics()
{
  size_t memory=0;
  size_t entries=0;

  for (std::map<TypeId,std::map<TileId,IndexEntry > >::const_iterator i=areaWayIndex.begin();
       i!=areaWayIndex.end();
       i++) {
    memory+=sizeof(i->first)+sizeof(i->second);
    for (std::map<TileId,IndexEntry >::const_iterator j=i->second.begin();
         j!=i->second.end();
         j++) {
      entries++;
      memory+=sizeof(j->first)+sizeof(j->second)+j->second.pages.size()*sizeof(Page);
    }
  }

  std::cout << "Area way index size " << entries << ", memory " << memory << std::endl;
}
