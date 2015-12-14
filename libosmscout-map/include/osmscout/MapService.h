#ifndef OSMSCOUT_MAPSERVICE_H
#define OSMSCOUT_MAPSERVICE_H

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

#include <list>
#include <memory>
#include <vector>

#include <osmscout/private/MapImportExport.h>

#include <osmscout/Database.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/MapPainter.h>
#include <osmscout/StyleConfig.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/StopClock.h>

#include <osmscout/TiledDataCache.h>

namespace osmscout {

  /**
    Parameter to influence the search result for searching for (drawable)
    objects in a given area.
    */
  class OSMSCOUT_MAP_API AreaSearchParameter
  {
  private:
    unsigned long maxAreaLevel;
    bool          useLowZoomOptimization;
    BreakerRef    breaker;
    bool          useMultithreading;

  public:
    AreaSearchParameter();

    void SetMaximumAreaLevel(unsigned long maxAreaLevel);

    void SetUseLowZoomOptimization(bool useLowZoomOptimization);

    void SetUseMultithreading(bool useMultithreading);

    void SetBreaker(const BreakerRef& breaker);

    unsigned long GetMaximumAreaLevel() const;

    bool GetUseLowZoomOptimization() const;

    bool GetUseMultithreading() const;

    bool IsAborted() const;
  };

  /**
   * \ingroup Service
   * MapService offers services for retrieving data in a way that is
   * helpful for drawing maps.
   *
   * Currently the following functionalities are supported:
   * - Get objects of a certain type in a given area and impose certain
   * limits on the resulting data (size of area, number of objects,
   * low zoom optimizations,...).
   */
  class OSMSCOUT_MAP_API MapService
  {
  private:
    struct TypeDefinition
    {
      Magnification magnification;
      TypeInfoSet   nodeTypes;
      TypeInfoSet   wayTypes;
      TypeInfoSet   areaTypes;
      TypeInfoSet   optimizedAreaTypes;
      TypeInfoSet   optimizedWayTypes;
    };

    typedef std::shared_ptr<TypeDefinition> TypeDefinitionRef;

  private:
    DatabaseRef            database;       //!< The reference to the database
    mutable TiledDataCache cache;          //!< Data cache
    TypeDefinitionRef      typeDefinition; //<! Last used and cached TypeDefinition

  private:
    TypeDefinitionRef GetTypeDefinition(const AreaSearchParameter& parameter,
                                        const StyleConfig& styleConfig,
                                        const Magnification& magnification) const;

    bool GetNodes(const AreaSearchParameter& parameter,
                  const TypeInfoSet& nodeTypes,
                  const GeoBox& boundingBox,
                  TileRef tile) const;

    bool GetAreasLowZoom(const AreaSearchParameter& parameter,
                         const TypeInfoSet& areaTypes,
                         const Magnification& magnification,
                         const GeoBox& boundingBox,
                         TileRef tile) const;

    bool GetAreas(const AreaSearchParameter& parameter,
                  const TypeInfoSet& areaTypes,
                  const Magnification& magnification,
                  const GeoBox& boundingBox,
                  TileRef tile) const;

    bool GetWaysLowZoom(const AreaSearchParameter& parameter,
                        const TypeInfoSet& wayTypes,
                        const Magnification& magnification,
                        const GeoBox& boundingBox,
                        TileRef tile) const;

    bool GetWays(const AreaSearchParameter& parameter,
                 const TypeInfoSet& wayTypes,
                 const GeoBox& boundingBox,
                 TileRef tile) const;

  public:
    MapService(const DatabaseRef& database);
    virtual ~MapService();

    void SetCacheSize(size_t cacheSize);

    void LookupTiles(const Magnification& magnification,
                     const GeoBox& boundingBox,
                     std::list<TileRef>& tiles) const;

    void LookupTiles(const Projection& projection,
                     std::list<TileRef>& tiles) const;

    TileRef LookupTile(const TileId& id) const;

    bool LoadMissingTileData(const AreaSearchParameter& parameter,
                             const StyleConfig& styleConfig,
                             std::list<TileRef>& tiles) const;

    void ConvertTilesToMapData(std::list<TileRef>& tiles,
                               MapData& data) const;

    bool GetGroundTiles(const Projection& projection,
                        std::list<GroundTile>& tiles) const;

    bool GetGroundTiles(const GeoBox& boundingBox,
                        const Magnification& magnification,
                        std::list<GroundTile>& tiles) const;
  };

  //! \ingroup Service
  //! Reference counted reference to an Database instance
  typedef std::shared_ptr<MapService> MapServiceRef;
}

#endif
