/*
  This source is part of the libosmscout library
  Copyright (C) 2018  Tim Teulings

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

#include <osmscout/routing/RouteNodeDataFile.h>

namespace osmscout {

  RouteNodeDataFile::RouteNodeDataFile(const std::string& datafile,
                           size_t cacheSize)
  : datafile(datafile),
    cache(cacheSize),
    tileCalculator(0)
  {
  }

  bool RouteNodeDataFile::Open(const TypeConfigRef& typeConfig,
                         const std::string& path,
                         bool memoryMappedData)
  {
    this->typeConfig=typeConfig;

    datafilename=AppendFileToDir(path,datafile);

    try {
      FileOffset indexFileOffset;
      uint32_t   dataCount;
      uint32_t   indexEntryCount;
      uint32_t   tileMag;

      scanner.Open(datafilename,
                   FileScanner::LowMemRandom,
                   memoryMappedData);

      scanner.Read(indexFileOffset);
      scanner.Read(dataCount);
      scanner.Read(tileMag);

      tileCalculator=TileCalculator(std::pow(2,tileMag));

      scanner.SetPos(indexFileOffset);
      scanner.Read(indexEntryCount);

      for (size_t i=1; i<=indexEntryCount; i++) {
        Pixel      cell;
        IndexEntry entry;

        scanner.Read(cell.x);
        scanner.Read(cell.y);
        scanner.ReadFileOffset(entry.fileOffset);
        scanner.Read(entry.count);

        index[cell]=entry;
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }

  /**
   * Return true, if index is currently opened.
   *
   * Method is NOT thread-safe.
   */
  bool RouteNodeDataFile::IsOpen() const
  {
    return scanner.IsOpen();
  }

  /**
   * Close the index.
   *
   * Method is NOT thread-safe.
   */
  bool RouteNodeDataFile::Close()
  {
    typeConfig=nullptr;

    try  {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }

  bool RouteNodeDataFile::LoadIndexPage(const osmscout::Pixel& tile,
                                        ValueCache::CacheRef& cacheRef) const
  {
    assert(IsOpen());

    auto entry=index.find(tile);

    if (entry==index.end()) {
      return false;
    }

    ValueCache::CacheEntry cacheEntry(tile.GetId());

    scanner.SetPos(entry->second.fileOffset);

    for (size_t i=1; i<=entry->second.count; i++) {
      RouteNodeRef node=std::make_shared<RouteNode>();

      node->Read(scanner);
      cacheEntry.value.nodeMap.insert(std::make_pair(node->GetId(),node));
    }

    cacheRef=cache.SetEntry(cacheEntry);

    return true;
  }

  bool RouteNodeDataFile::GetIndexPage(const osmscout::Pixel& tile,
                                       ValueCache::CacheRef& cacheRef) const
  {
    if (!cache.GetEntry(tile.GetId(),
                        cacheRef)) {
      //std::cout << "RouteNodeDF::GetIndexPage() Not fond in cache, loading...!" << std::endl;
      if (!LoadIndexPage(tile,
                         cacheRef)) {
        return false;
      }
    }

    return true;
  }

  bool RouteNodeDataFile::Get(Id id,
                              RouteNodeRef& node) const
  {
    //std::cout << "Loading RouteNode " << id << "..." << std::endl;
    ValueCache::CacheRef cacheRef;

    GeoCoord coord=Point::GetCoordFromId(id);
    osmscout::Pixel tile=tileCalculator.GetTileId(coord);

    //std::cout << "Tile " << tile.GetDisplayText() << " " << tile.GetId() << "..." << std::endl;

    if (!GetIndexPage(tile,
                      cacheRef)) {
      return false;
    }

    auto nodeEntry=cacheRef->value.nodeMap.find(id);

    if (nodeEntry!=cacheRef->value.nodeMap.end()) {
      node=nodeEntry->second;
      return true;
    }

    return false;
  }
}

