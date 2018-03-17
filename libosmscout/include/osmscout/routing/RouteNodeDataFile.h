#ifndef OSMSCOUT_ROUTENODEDATAFILE_H
#define OSMSCOUT_ROUTENODEDATAFILE_H

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

#include <map>
#include <vector>

#include <osmscout/DataFile.h>
#include <osmscout/Pixel.h>

#include <osmscout/routing/RouteNode.h>

#include <osmscout/util/Tiling.h>

namespace osmscout {
  /**
   * \ingroup Routing
   */
  class OSMSCOUT_API RouteNodeDataFile CLASS_FINAL
  {
  private:
    struct IndexEntry
    {
      FileOffset fileOffset;
      uint32_t   count;
    };

    struct IndexPage
    {
      FileOffset                          fileOffset;
      uint32_t                            remaining;
      std::unordered_map<Id,RouteNodeRef> nodeMap;

      RouteNodeRef find(FileScanner& scanner,
                        Id id);
    };

  private:
    typedef Cache<Id,IndexPage> ValueCache;

  private:
    std::string                datafile;        //!< Basename part of the data file name
    std::string                datafilename;    //!< complete filename for data file

    TypeConfigRef              typeConfig;      //! typeConfig

    std::map<Pixel,IndexEntry> index;

    mutable FileScanner        scanner;         //!< File stream to the data file
    mutable ValueCache         cache;           //!< Cache of loaded route node pages
    mutable std::mutex         accessMutex;     //!< Mutex to secure multi-thread access
    mutable TileCalculator     tileCalculator;

  private:
    bool LoadIndexPage(const osmscout::Pixel& tile,
                       ValueCache::CacheRef& cacheRef) const;
    bool GetIndexPage(const osmscout::Pixel& tile,
                      ValueCache::CacheRef& cacheRef) const;

  public:
    explicit RouteNodeDataFile(const std::string& datafile,
                         size_t cacheSize);

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMapedData);
    bool IsOpen() const;
    bool Close();

    Pixel GetTile(const GeoCoord& coord) const;
    bool IsCovered(const Pixel& tile) const;

    bool IsCovered(const GeoCoord& coord) const;

    bool Get(Id id,
             RouteNodeRef& node) const;

    template<typename IteratorIn>
    bool Get(IteratorIn begin, IteratorIn end, size_t size,
             std::vector<RouteNodeRef>& data) const
    {
      data.reserve(size);

      for (IteratorIn idIter=begin; idIter!=end; ++idIter) {
        Id                   id=*idIter;
        ValueCache::CacheRef cacheRef;

        GeoCoord coord=Point::GetCoordFromId(id);
        osmscout::Pixel tile=tileCalculator.GetTileId(coord);

        //std::cout << "Tile " << tile.GetDisplayText() << " " << tile.GetId() << "..." << std::endl;

        if (!GetIndexPage(tile,
                          cacheRef)) {
          return false;
        }

        auto node=cacheRef->value.find(scanner,
                                       id);

        if (node==nullptr) {
          return false;
        }

        data.push_back(node);
      }

      return true;
    }

    template<typename IteratorIn>
    bool Get(IteratorIn begin, IteratorIn end, size_t /*size*/,
             std::unordered_map<Id,RouteNodeRef>& dataMap) const
    {
      for (IteratorIn idIter=begin; idIter!=end; ++idIter) {
        Id                   id=*idIter;
        ValueCache::CacheRef cacheRef;

        GeoCoord coord=Point::GetCoordFromId(id);
        osmscout::Pixel tile=tileCalculator.GetTileId(coord);

        //std::cout << "Tile " << tile.GetDisplayText() << " " << tile.GetId() << "..." << std::endl;

        if (!GetIndexPage(tile,
                          cacheRef)) {
          return false;
        }

        auto node=cacheRef->value.find(scanner,
                                       id);

        if (node==nullptr) {
          return false;
        }

        dataMap[id]=node;
      }

      return true;
    }
  };

}

#endif

