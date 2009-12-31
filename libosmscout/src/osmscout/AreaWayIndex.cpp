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

#include <iostream>
#include <map>

#include <osmscout/FileReader.h>

bool AreaWayIndex::LoadAreaWayIndex(const std::string& path)
{
  FileReader  reader;
  std::string file=path+"/"+"areaway.idx";

  if (!reader.Open(file) || !reader.ReadFileToBuffer()) {
    return false;
  }

  size_t drawTypes;

  // The number of draw types we have an index for
  reader.ReadNumber(drawTypes); // Number of entries

  std::cout << drawTypes << " entries in area way index..." << std::endl;

  for (size_t i=0; i<drawTypes; i++) {
    TypeId type;
    size_t tiles;

    reader.ReadNumber(type);  // The draw type id
    reader.ReadNumber(tiles); // The number of tiles

    for (size_t t=0; t<tiles; t++) {
      IndexEntry entry;
      TileId     tileId;
      size_t     idCount;

      reader.ReadNumber(tileId);          // The tile id
      reader.ReadNumber(entry.nodeCount); // The number of nodes
      reader.ReadNumber(idCount);       // The number of pages

      entry.ids.reserve(idCount);

      for (size_t i=0; i<idCount; i++) {
        Id id;

        reader.ReadNumber(id); // The id of the page
        entry.ids.push_back(id);
      }

      areaWayIndex[type][tileId]=entry;
    }
  }

  return !reader.HasError() && reader.Close();
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
      TileId                                      startTileId=GetTileId(tileMinX,y);
      TileId                                      endTileId=GetTileId(tileMaxX,y);
      std::map<TileId,IndexEntry>::const_iterator tile=drawTypeEntry->second.lower_bound(startTileId);

      while (tile->first<=endTileId && tile!=drawTypeEntry->second.end()) {
        nodes+=tile->second.nodeCount;

        ++tile;
      }
    }
  }

  return nodes;
}

void AreaWayIndex::GetIds(const StyleConfig& styleConfig,
                          double minlon, double minlat,
                          double maxlon, double maxlat,
                          double magnification,
                          size_t maxPriority,
                          std::set<Id>& ids) const
{
  std::set<TypeId> types;

  styleConfig.GetWayTypesWithMaxPrio(maxPriority,types);

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
        TileId                                      startTileId=GetTileId(minTileX,y);
        TileId                                      endTileId=GetTileId(maxTileX,y);
        std::map<TileId,IndexEntry>::const_iterator tile=typeEntry->second.lower_bound(startTileId);

        while (tile->first<=endTileId && tile!=typeEntry->second.end()) {
          for (size_t j=0; j<tile->second.ids.size(); j++) {
            ids.insert(tile->second.ids[j]);
          }

          ++tile;
        }
      }
    }
  }

  std::cout << "Found " << ids.size() << " ways ids in area way index with maximum priority " << maxPriority << std::endl;
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
      memory+=sizeof(j->first)+sizeof(j->second)+j->second.ids.size()*sizeof(Id);
    }
  }

  std::cout << "Area way index size " << entries << ", memory " << memory << std::endl;
}
