
/*
  This source is part of the libosmscout-map library
  Copyright (C) 2016  Lukáš Karas

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

#include <QDebug>
#include <QElapsedTimer>

#include <iostream>
#include <algorithm>
#include <tuple>

#include <osmscoutclientqt/TileCache.h>
#include <osmscoutclientqt/OSMTile.h>

namespace osmscout {

using namespace std;

uint qHash(const TileCacheKey &key){
  return (key.zoomLevel << 24) ^ (key.xtile << 12) ^ key.ytile;
}

bool operator==(const TileCacheKey &a, const TileCacheKey &b)
{
  return a.zoomLevel == b.zoomLevel && a.xtile == b.xtile && a.ytile == b.ytile;
}

bool operator<(const TileCacheKey &a, const TileCacheKey &b)
{
  return std::tie(a.zoomLevel, a.xtile, a.ytile) < std::tie(b.zoomLevel, b.xtile, b.ytile);
}

QDebug& operator<<(QDebug &out, const TileCacheKey &key)
{
  out << QString("z: %1, %2x%3").arg(key.zoomLevel).arg(key.xtile).arg(key.ytile);
  return out;
}

TileCache::TileCache(size_t cacheSize):
  cacheSize(cacheSize)
{
}

void TileCache::clearPendingRequests()
{        
    QMutableHashIterator<TileCacheKey, RequestState> it(requests);
    while (it.hasNext()){
      it.next();
      if (it.value().pending){
#ifdef DEBUG_TILE_CACHE
          qDebug() << this << "remove pending" << it.key();
#endif
          it.remove();
      }
    }
}

bool TileCache::isRequestQueueEmpty() const
{
    return requests.isEmpty();
}

void TileCache::mergeAndStartRequests(uint32_t zoomLevel, uint32_t xtile, uint32_t ytile, 
                                      uint32_t &xFrom, uint32_t &xTo, uint32_t &yFrom, uint32_t &yTo,
                                      uint32_t maxWidth, uint32_t maxHeight)
{
    xFrom = xtile;
    xTo = xtile;
    yFrom = ytile;
    yTo = ytile;

    TileCacheKey key = {zoomLevel, xtile, ytile};
    QMutableHashIterator<TileCacheKey, RequestState> it(requests);
    while (it.hasNext()){
        it.next();
        TileCacheKey req = it.key();
        RequestState state = it.value();
        if (state.pending && req.zoomLevel == zoomLevel
                && (std::max(xTo, req.xtile) - std::min(xFrom, req.xtile) < maxWidth)
                && (std::max(yTo, req.ytile) - std::min(yFrom, req.ytile) < maxHeight)
                ){

            xFrom = std::min(xFrom, req.xtile);
            xTo = std::max(xTo, req.xtile);
            yFrom = std::min(yFrom, req.ytile);
            yTo = std::max(yTo, req.ytile);
            state.pending = false;
            requests.insert(key, state);
        }
    }
}

bool TileCache::startRequestProcess(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    if (requests.contains(key)){
        RequestState state = requests.value(key);
        if (state.pending){
#ifdef DEBUG_TILE_CACHE
            qDebug() << this << "start process" << key;
#endif
            state.pending = false;
            requests.insert(key, state);
            return true;
        }else{
            return false; // started already
        }
    }else{
        return false;
    }
}

bool TileCache::request(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    if (requests.contains(key))
        return false;

#ifdef DEBUG_TILE_CACHE
    qDebug() << this << "request" << key;
#endif

    RequestState state = {true};
    requests.insert(key, state);
    emit tileRequested(zoomLevel, x, y);
    return true;
}

bool TileCache::reemitRequests()
{
    for (const auto &request: requests.keys()){
      emit tileRequested(request.zoomLevel, request.xtile, request.ytile);
    }
    return !requests.isEmpty();
}

bool TileCache::containsRequest(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    return requests.contains(key);
}

bool TileCache::contains(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    return tiles.contains(key);
}

TileCacheVal TileCache::get(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    if (!tiles.contains(key)){
        qWarning() << this << "No tile in cache for key " << key;
        return {TileCacheVal::clock::now(), QImage(), epoch}; // throw std::underflow_error ?
    }
    TileCacheVal val = tiles.value(key);
    val.lastAccess = TileCacheVal::clock::now();
    tiles.insert(key, val);
    return val;
}

bool TileCache::removeRequest(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};

#ifdef DEBUG_TILE_CACHE
    qDebug() << this << "remove request" << key;
#endif

    return requests.remove(key) > 0;
}

void TileCache::put(uint32_t zoomLevel, uint32_t x, uint32_t y, const QImage &image, size_t epoch)
{
    removeRequest(zoomLevel, x, y);
    TileCacheKey key = {zoomLevel, x, y};
    TileCacheVal val = {TileCacheVal::clock::now(), image, epoch};

#ifdef DEBUG_TILE_CACHE
    qDebug() << this << "inserting tile" << key;
#endif

    tiles.insert(key, val);

    if (tiles.size() > (int)cacheSize) {
      cleanupCache(cacheSize / 10, std::chrono::minutes{5});
    }
}

void TileCache::cleanupCache(uint32_t maxRemove, const std::chrono::milliseconds &maximumLifetime)
{
    if (maxRemove==std::numeric_limits<uint32_t>::max() && maximumLifetime.count() == 0) {
      // flush cache completely
      tiles.clear();
      return;
    }

    /**
     * first, we will iterate over all entries and remove up to maxRemove tiles
     * older than `maximumLifetime`, if no such entry found and size is bigger than cacheSize,
     * remove oldest tile
     *
     * Goal is to remove more items at once and minimise frequency of this expensive cleaning
     */

#ifdef DEBUG_TILE_CACHE
    qDebug() << this << "Cleaning tile cache (" << cacheSize << ")";
#endif

    uint32_t removed = 0;
    auto oldest=TileCacheVal::clock::duration::zero();
    TileCacheKey key;
    TileCacheKey oldestKey;
    auto now = TileCacheVal::clock::now();

    QMutableHashIterator<TileCacheKey, TileCacheVal> it(tiles);
    while (it.hasNext() && removed < maxRemove){
        it.next();

        key = it.key();
        TileCacheVal val = it.value();

        auto elapsed = now - val.lastAccess;
        if (elapsed > oldest){
          oldest = elapsed;
          oldestKey = key;
        }

        if (elapsed > duration_cast<TileCacheVal::clock::duration>(maximumLifetime)){
#ifdef DEBUG_TILE_CACHE
          qDebug() << this << "removing" << key;
#endif

          it.remove();

          removed ++;
        }
    }
    if (tiles.size() > (int)cacheSize){
        if (removed == 0 && oldest.count() > 0){
          key = oldestKey;
#ifdef DEBUG_TILE_CACHE
          qDebug() << this << "removing" << key;
#endif
          tiles.remove(key);
        }
    }
}

bool TileCache::invalidate(osmscout::GeoBox box){
    QMutableHashIterator<TileCacheKey, TileCacheVal> it(tiles);
    bool removed = false;
    TileCacheKey key;
    osmscout::GeoBox bbox;
    while (it.hasNext() ){            
        it.next();
        key = it.key();

        // TODO: setup some invalid flag instead of removing tile
        if (box.IsValid()){
            bbox = OSMTile::tileBoundingBox(key.zoomLevel, key.xtile, key.ytile);
            if (box.Intersects(bbox)){
                it.remove();
                removed = true;
            }
        }else{
            it.remove();
            removed = true;
        }
    }
    return removed;
}
}
