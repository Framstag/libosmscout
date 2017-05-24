
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

#include <iostream>
#include <algorithm>

#include <osmscout/TileCache.h>
#include <osmscout/OSMTile.h>

using namespace std;

uint qHash(const TileCacheKey &key){
  return (key.zoomLevel << 24) ^ (key.xtile << 12) ^ key.ytile;
}

bool operator==(const TileCacheKey a, const TileCacheKey b)
{
    return a.zoomLevel == b.zoomLevel && a.xtile == b.xtile && a.ytile == b.ytile;
}

TileCache::TileCache(size_t cacheSize):  
  tiles(), 
  requests(),
  cacheSize(cacheSize), 
  maximumLivetimeMs(5 * 60 * 1000)
{
}

TileCache::~TileCache() 
{
}

void TileCache::clearPendingRequests()
{        
    QMutableHashIterator<TileCacheKey, RequestState> it(requests);
    while (it.hasNext()){
      it.next();
      if (it.value().pending){
          // qDebug() << "remove pending " << QString("z: %1, %2x%3").arg(it.key().zoomLevel).arg(it.key().xtile).arg(it.key().ytile);                
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
        // qDebug() << "start process " << QString("z: %1, %2x%3").arg(key.zoomLevel).arg(key.xtile).arg(key.ytile);
        RequestState state = requests.value(key); 
        if (state.pending){
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

    // qDebug() << "request " << QString("z: %1, %2x%3").arg(key.zoomLevel).arg(key.xtile).arg(key.ytile);
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
        qWarning() << "No tile in cache for key {" << zoomLevel << ", " << x << ", " << y << "}";
        return {QTime(), QPixmap()}; // throw std::underflow_error ?
    }
    TileCacheVal val = tiles.value(key);
    val.lastAccess.start();
    tiles.insert(key, val);
    return val;
}

void TileCache::removeRequest(uint32_t zoomLevel, uint32_t x, uint32_t y)
{
    TileCacheKey key = {zoomLevel, x, y};
    requests.remove(key);    
    // qDebug() << "remove " << QString("z: %1, %2x%3").arg(key.zoomLevel).arg(key.xtile).arg(key.ytile);
}

void TileCache::put(uint32_t zoomLevel, uint32_t x, uint32_t y, QImage image)
{
    QPixmap pixmap = QPixmap::fromImage(image);
    removeRequest(zoomLevel, x, y);
    TileCacheKey key = {zoomLevel, x, y};
    QTime now;
    now.start();
    TileCacheVal val = {now, pixmap};
    tiles.insert(key, val);

    cleanupCache();
}

void TileCache::cleanupCache()
{
    
    if (tiles.size() > (int)cacheSize){
        /** 
         * maximum size reached
         * 
         * first, we will iterate over all entries and remove up to 10% tiles 
         * older than `maximumLivetimeMs`, if no such entry found, remove oldest
         */
        qDebug() << "Cleaning tile cache (" << cacheSize << ")";
        uint32_t removed = 0;
        int oldest = 0;
        TileCacheKey key;
        TileCacheKey oldestKey;

        QMutableHashIterator<TileCacheKey, TileCacheVal> it(tiles);
        while (it.hasNext() && removed < (cacheSize / 10)){
            it.next();

            //QHash<TileCacheKey, TileCacheVal>::const_iterator it = tiles.constBegin();
            //while (it != tiles.constEnd() && removed < (cacheSize / 10)){

            key = it.key();
            TileCacheVal val = it.value();

            int elapsed = val.lastAccess.elapsed();
            if (elapsed > oldest){
              oldest = elapsed;
              oldestKey = key;
            }

            if (elapsed > (int)maximumLivetimeMs){
              qDebug() << "  removing " << key.zoomLevel << " / " << key.xtile << " x " << key.ytile;

              //tiles.remove(key);
              it.remove();

              removed ++;
            }
            //++it;
        }
        if (removed == 0 && oldest > 0){
          key = oldestKey;
          qDebug() << "  removing " << key.zoomLevel << " / " << key.xtile << " x " << key.ytile;
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

