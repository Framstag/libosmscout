
#ifndef TILECACHE_H
#define	TILECACHE_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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


#include <QObject>
#include <QString>
#include <QMetaType>
#include <QMutex>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QTime>
#include <QImage>
#include <QPixmap>

#include <osmscout/util/GeoBox.h>
#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 */
struct TileCacheKey
{
    uint32_t zoomLevel;
    uint32_t xtile; 
    uint32_t ytile;
  
};

bool operator==(const TileCacheKey a, const TileCacheKey b);

uint qHash(const TileCacheKey &key);

Q_DECLARE_METATYPE(TileCacheKey)

/**
 * \ingroup QtAPI
 */
struct TileCacheVal
{
  QTime lastAccess;
  QPixmap image;
};

Q_DECLARE_METATYPE(TileCacheVal)

/**
 * \ingroup QtAPI
 */
struct RequestState
{
    bool pending; //!< if pending is false, request is currently processing
};

Q_DECLARE_METATYPE(RequestState)

/**
 * \ingroup QtAPI
 * 
 * Cache have to be locked by its mutex() while access.
 * It owns all inserted tiles and it is responsible for its release
 */
class OSMSCOUT_CLIENT_QT_API TileCache : public QObject
{
  Q_OBJECT
  
signals: 
  void tileRequested(uint32_t zoomLevel, uint32_t x, uint32_t y);
  
public:
  TileCache(size_t cacheSize);
  virtual ~TileCache();
  
  /**
   * revove all pending requests
   * TODO: in case of multiple map widgets, add some id to avoid removing requests 
   * of another widget
   */
  void clearPendingRequests();
  bool startRequestProcess(uint32_t zoomLevel, uint32_t x, uint32_t y);
  void mergeAndStartRequests(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile, 
    uint32_t &xFrom, uint32_t &xTo, uint32_t &yFrom, uint32_t &yTo, uint32_t maxWidth, uint32_t maxHeight);
  bool isRequestQueueEmpty() const;

  /**
   * try to create new tile request. 
   * If this request don't exists already, it emit signal tileRequested and return 
   * true. Otherwise false.
   */
  bool request(uint32_t zoomLevel, uint32_t x, uint32_t y);

  /**
   * trigger request signal for all pending requests
   */
  bool reemitRequests();
  bool contains(uint32_t zoomLevel, uint32_t x, uint32_t y);
  bool containsRequest(uint32_t zoomLevel, uint32_t x, uint32_t y);  
  TileCacheVal get(uint32_t zoomLevel, uint32_t x, uint32_t y);
  
  /**
   * 
   * @param box
   * @return 
   */
  bool invalidate(osmscout::GeoBox box = osmscout::GeoBox());
  
  void removeRequest(uint32_t zoomLevel, uint32_t x, uint32_t y);
  void put(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image);
  
  void cleanupCache();
  
private:
  QHash<TileCacheKey, TileCacheVal> tiles;
  QHash<TileCacheKey, RequestState> requests;
  size_t                            cacheSize; // maximum count of elements in cache
  uint32_t                          maximumLivetimeMs;
};

#endif	/* TILECACHE_H */

