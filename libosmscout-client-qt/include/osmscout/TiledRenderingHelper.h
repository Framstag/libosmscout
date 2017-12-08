#ifndef LIBOSMSCOUT_TILEDRENDERINGHELPER_H
#define LIBOSMSCOUT_TILEDRENDERINGHELPER_H
/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings
 Copyright (C) 2017 Lukas Karas

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

#include <osmscout/TileCache.h>
#include <osmscout/DBThread.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QPainter>

class OSMSCOUT_CLIENT_QT_API TiledRenderingHelper
{
private:
  TiledRenderingHelper(){};

  /**
   * lookup tile in cache, if not found, try upper zoom level for substitute.
   * (It is better upscaled tile than empty space)
   * Is is repeated up to zoomLevel - upLimit
   */
  static bool lookupAndDrawTile(TileCache& tileCache, QPainter& painter,
                                double x, double y, double renderTileWidth, double renderTileHeight,
                                uint32_t zoomLevel, uint32_t xtile, uint32_t ytile,
                                uint32_t upLimit, uint32_t downLimit,
                                double overlap);

  static void lookupAndDrawBottomTileRecursive(TileCache& tileCache, QPainter& painter,
                                               double x, double y, double renderTileWidth, double renderTileHeight, double overlap,
                                               uint32_t zoomLevel, uint32_t xtile, uint32_t ytile,
                                               uint32_t downLimit);

public:
  static bool RenderTiles(QPainter& painter,
                          const MapViewStruct& request,
                          QList<TileCache*> &layerCaches,
                          double mapDpi,
                          const QColor &unknownColor,
                          double overlap=-1);
};

#endif //LIBOSMSCOUT_TILEDRENDERINGHELPER_H
