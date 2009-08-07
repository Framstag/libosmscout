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

#include <osmscout/AreaNodeIndex.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <map>

#include <osmscout/FileReader.h>

bool AreaNodeIndex::LoadAreaNodeIndex(const std::string& path)
{
  FileReader  reader;
  std::string file=path+"/"+"areanode.idx";

  if (!reader.Open(file) || !reader.ReadFileToBuffer()) {
    return false;
  }

  size_t drawTypes;

  // The number of draw types we have an index for
  reader.ReadNumber(drawTypes); // Number of entries

  std::cout << drawTypes << " area node index entries..." << std::endl;

  for (size_t i=0; i<drawTypes; i++) {
    TypeId type;
    size_t tiles;

    reader.ReadNumber(type);  // The draw type id
    reader.ReadNumber(tiles); // The number of tiles

    for (size_t t=0; t<tiles; t++) {
      IndexEntry entry;
      TileId     tileId;
      size_t     pageCount;

      reader.ReadNumber(tileId);          // The tile id
      reader.ReadNumber(entry.nodeCount); // The number of nodes
      reader.ReadNumber(pageCount);       // The number of pages

      entry.pages.reserve(pageCount);

      for (size_t p=0; p<pageCount; p++) {
        Page page;

        reader.ReadNumber(page); // The id of the page

        entry.pages.push_back(page);

      }

      areaNodeIndex[type][tileId]=entry;
    }
  }

  return !reader.HasError() && reader.Close();
}

size_t AreaNodeIndex::GetNodes(TypeId drawType,
                               size_t tileMinX, size_t tileMinY,
                               size_t tileMaxX, size_t tileMaxY) const
{
  size_t nodes=0;

  std::map<TypeId,std::map<TileId,IndexEntry > >::const_iterator drawTypeEntry;

  drawTypeEntry=areaNodeIndex.find(drawType);

  if (drawTypeEntry!=areaNodeIndex.end()) {
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

void AreaNodeIndex::GetPages(const StyleConfig& styleConfig,
                             double minlon, double minlat,
                             double maxlon, double maxlat,
                             double magnification,
                             size_t maxPriority,
                             std::set<Page>& pages) const
{
  std::set<TypeId> types;

  styleConfig.GetNodeTypesWithMag(magnification,types);

  for (std::set<TypeId>::const_iterator type=types.begin();
       type!=types.end();
       ++type) {

    //std::cout << "Displaying draw type: " << *type << std::endl;

    std::map<TypeId,std::map<TileId,IndexEntry> >::const_iterator typeEntry;

    typeEntry=areaNodeIndex.find(*type);

    if (typeEntry!=areaNodeIndex.end()) {
      for (size_t y=GetTileY(minlat); y<=GetTileY(maxlat); y++) {
        for (size_t x=GetTileX(minlon); x<=GetTileX(maxlon); x++) {
          TileId                                      tileId=GetTileId(x,y);
          std::map<TileId,IndexEntry>::const_iterator tile;

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

  std::cout << "Found " << pages.size() << " node pages in area node index with maximum priority " << maxPriority << std::endl;
}

void AreaNodeIndex::DumpStatistics()
{
  size_t memory=0;
  size_t entries=0;

  for (std::map<TypeId,std::map<TileId,IndexEntry> >::const_iterator i=areaNodeIndex.begin();
       i!=areaNodeIndex.end();
       i++) {
    memory+=sizeof(i->first)+sizeof(i->second);
    for (std::map<TileId,IndexEntry>::const_iterator j=i->second.begin();
         j!=i->second.end();
         j++) {
      entries++;
      memory+=sizeof(j->first)+sizeof(j->second)+j->second.pages.size()*sizeof(Page);
    }
  }

  std::cout << "Area node index size " << entries << ", memory " << memory << std::endl;
}
