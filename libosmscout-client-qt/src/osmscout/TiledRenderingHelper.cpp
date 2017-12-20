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

#include <osmscout/TiledRenderingHelper.h>
#include <osmscout/DBThread.h>
#include <osmscout/OSMTile.h>

#include <osmscout/system/Math.h>

// uncomment or define by compiler parameter to render various debug marks
// #define DRAW_DEBUG

bool TiledRenderingHelper::RenderTiles(QPainter &painter,
                                       const MapViewStruct &request,
                                       QList<TileCache*> &layerCaches,
                                       double mapDpi,
                                       const QColor &unknownColor,
                                       double overlap)
{
  osmscout::MercatorProjection projection;

  // compute canvas transformation from angle
  double width, height;
  QPointF translateVector;
  if (request.angle==0) {
    width = request.width;
    height = request.height;
  } else {
    double cosAlpha=cos(request.angle);
    double cosBeta=cos(M_PI_2 - request.angle);
    double rw=request.width;
    double rh=request.height;
    height=std::abs(rw*cosBeta)+std::abs(rh*cosAlpha);
    width=std::abs(rw*cosAlpha)+std::abs(rh*cosBeta);
    if (request.angle>0 && request.angle<=M_PI_2) {
      translateVector.setY(cosBeta*rw*-1);
    } else if (request.angle<=M_PI) {
      translateVector.setY(height*-1);
      translateVector.setX(cosAlpha * rw);
    } else if (request.angle<=M_PI+M_PI_2) {
      translateVector.setX(width*-1);
      translateVector.setY(height*-1 - cosBeta*rw);
    } else {
      translateVector.setX(width*-1 + cosAlpha*rw);
    }
  }

  projection.Set(request.coord,
                 0,
                 request.magnification,
                 mapDpi,
                 width,
                 height);

  osmscout::GeoBox boundingBox;

  projection.GetDimensions(boundingBox);

  QColor grey2 = QColor::fromRgbF(0.8,0.8,0.8);

  // OpenStreetMap render its tiles up to latitude +-85.0511
  double osmMinLat = OSMTile::minLat();
  double osmMaxLat = OSMTile::maxLat();
  double osmMinLon = OSMTile::minLon();
  double osmMaxLon = OSMTile::maxLon();

  // check if request center is defined in Mercator projection
  if (!osmscout::GeoBox(osmscout::GeoCoord(osmMinLat, osmMinLon),
                        osmscout::GeoCoord(osmMaxLat, osmMaxLon))
      .Includes(request.coord)){
    qWarning() << "Outside projection";
    return false;
  }

  uint32_t osmTileRes = OSMTile::worldRes(projection.GetMagnification().GetLevel());
  double x1;
  double y1;
  projection.GeoToPixel(osmscout::GeoCoord(osmMaxLat, osmMinLon), x1, y1);
  double x2;
  double y2;
  projection.GeoToPixel(osmscout::GeoCoord(osmMinLat, osmMaxLon), x2, y2);

  double renderTileWidth = (x2 - x1) / osmTileRes; // pixels
  double renderTileHeight = (y2 - y1) / osmTileRes; // pixels

  uint32_t osmTileFromX = std::max(0.0, (double)osmTileRes * ((boundingBox.GetMinLon() + (double)180.0) / (double)360.0));
  double maxLatRad = boundingBox.GetMaxLat() * GRAD_TO_RAD;
  uint32_t osmTileFromY = std::max(0.0, (double)osmTileRes * ((double)1.0 - (log(tan(maxLatRad) + (double)1.0 / cos(maxLatRad)) / M_PI)) / (double)2.0);

  uint32_t zoomLevel = projection.GetMagnification().GetLevel();

  if (overlap<0) {
    // trick for avoiding white lines between tiles caused by antialiasing
    // http://stackoverflow.com/questions/7332118/antialiasing-leaves-thin-line-between-adjacent-widgets
    overlap = painter.testRenderHint(QPainter::Antialiasing) ? 0.5 : 0.0;
  }

  // render available tiles
  double x;
  double y;

  painter.save();
  if (request.angle!=0) {
    painter.rotate(qRadiansToDegrees(request.angle));
    painter.translate(translateVector);
  }

  painter.setPen(grey2);

  painter.fillRect(0,0,
                   projection.GetWidth(),projection.GetHeight(),
                   unknownColor);


  for ( uint32_t ty = 0;
        (ty <= (projection.GetHeight() / (uint32_t)renderTileHeight)+1) && ((osmTileFromY + ty) < osmTileRes);
        ty++ ){

    uint32_t ytile = (osmTileFromY + ty);
    double ytileLatRad = atan(sinh(M_PI * (1 - 2 * (double)ytile / (double)osmTileRes)));
    double ytileLatDeg = ytileLatRad * 180.0 / M_PI;

    for ( uint32_t tx = 0;
          (tx <= (projection.GetWidth() / (uint32_t)renderTileWidth)+1) && ((osmTileFromX + tx) < osmTileRes);
          tx++ ){

      uint32_t xtile = (osmTileFromX + tx);
      double xtileDeg = (double)xtile / (double)osmTileRes * 360.0 - 180.0;

      projection.GeoToPixel(osmscout::GeoCoord(ytileLatDeg, xtileDeg), x, y);

      bool lookupTileFound = false;
      for (TileCache *cache:layerCaches){
        lookupTileFound |= lookupAndDrawTile(*cache, painter,
                                             x, y, renderTileWidth, renderTileHeight,
                                             zoomLevel, xtile, ytile, /* up limit */ 6, /* down limit */ 3,
                                             overlap
        );
      }

      if (!lookupTileFound){
        // no tile found, draw its outline
        painter.drawLine(x,y, x + renderTileWidth, y);
        painter.drawLine(x,y, x, y + renderTileHeight);
      }
    }
  }
#ifdef DRAW_DEBUG
  painter.setPen(QColor::fromRgbF(1,0,0));
  painter.drawLine(0,0,width,height);
  painter.drawLine(width,0,0,height);

  painter.drawLine(0,0,width,0);
  painter.drawLine(0,height,width,height);
  painter.drawLine(0,0,0,height);
  painter.drawLine(width,0,width,height);

  painter.restore();

  painter.setPen(grey2);
  painter.drawText(20, 30, QString("%1").arg(projection.GetMagnification().GetLevel()));

  double centerLat;
  double centerLon;
  projection.PixelToGeo(projection.GetWidth() / 2.0, projection.GetHeight() / 2.0, centerLon, centerLat);
  painter.drawText(20, 60, QString::fromStdString(osmscout::GeoCoord(centerLat, centerLon).GetDisplayText()));

  painter.setPen(QColor::fromRgbF(0,0,1));
  painter.drawLine(0,0,request.width,request.height);
  painter.drawLine(request.width,0,0,request.height);
#else
  painter.restore();
#endif
  return true;
}

bool TiledRenderingHelper::lookupAndDrawTile(TileCache& tileCache, QPainter& painter,
                                             double x, double y, double renderTileWidth, double renderTileHeight,
                                             uint32_t zoomLevel, uint32_t xtile, uint32_t ytile,
                                             uint32_t upLimit, uint32_t downLimit, double overlap)
{
  bool triggerRequest = true;

  uint32_t lookupTileZoom = zoomLevel;
  uint32_t lookupXTile = xtile;
  uint32_t lookupYTile = ytile;
  QRectF lookupTileViewport(0, 0, 1, 1); // tile viewport (percent)
  bool lookupTileFound = false;

  // lookup upper zoom levels
  //qDebug() << "Need paint tile " << xtile << " " << ytile << " zoom " << zoomLevel;
  while ((!lookupTileFound) && (zoomLevel - lookupTileZoom <= upLimit)){
    //qDebug() << "  - lookup tile " << lookupXTile << " " << lookupYTile << " zoom " << lookupTileZoom << " " << " viewport " << lookupTileViewport;
    if (tileCache.contains(lookupTileZoom, lookupXTile, lookupYTile)){
      TileCacheVal val = tileCache.get(lookupTileZoom, lookupXTile, lookupYTile);
      if (!val.image.isNull()){
        double imageWidth = val.image.width();
        double imageHeight = val.image.height();
        QRectF imageViewport(imageWidth * lookupTileViewport.x(), imageHeight * lookupTileViewport.y(),
                             imageWidth * lookupTileViewport.width(), imageHeight * lookupTileViewport.height() );

        // TODO: support map rotation
        painter.drawPixmap(QRectF(x, y, renderTileWidth+overlap, renderTileHeight+overlap), val.image, imageViewport);
      }
      lookupTileFound = true;
      if (lookupTileZoom == zoomLevel)
        triggerRequest = false;
    }else{
      // no tile found on current zoom zoom level, lookup upper zoom level
      if (lookupTileZoom==0)
        break;
      lookupTileZoom --;
      uint32_t crop = 1 << (zoomLevel - lookupTileZoom);
      double viewportWidth = 1.0 / (double)crop;
      double viewportHeight = 1.0 / (double)crop;
      lookupTileViewport = QRectF(
          (double)(xtile % crop) * viewportWidth,
          (double)(ytile % crop) * viewportHeight,
          viewportWidth,
          viewportHeight);
      lookupXTile = lookupXTile / 2;
      lookupYTile = lookupYTile / 2;
    }
  }

  // lookup bottom zoom levels
  if (!lookupTileFound && downLimit > 0){
    lookupAndDrawBottomTileRecursive(tileCache, painter,
                                     x, y, renderTileWidth, renderTileHeight, overlap,
                                     zoomLevel, xtile, ytile,
                                     downLimit -1);
  }

  if (triggerRequest){
    if (tileCache.request(zoomLevel, xtile, ytile)){
      //std::cout << "  tile request: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
    }else{
      //std::cout << "  requested already: " << zoomLevel << " xtile: " << xtile << " ytile: " << ytile << std::endl;
    }
  }
  return lookupTileFound;
}

void TiledRenderingHelper::lookupAndDrawBottomTileRecursive(TileCache& tileCache, QPainter& painter,
                                                            double x, double y, double renderTileWidth, double renderTileHeight, double overlap,
                                                            uint32_t zoomLevel, uint32_t xtile, uint32_t ytile,
                                                            uint32_t downLimit)
{
  if (zoomLevel > 20)
    return;

  //qDebug() << "Need paint tile " << xtile << " " << ytile << " zoom " << zoomLevel;
  uint32_t lookupTileZoom = zoomLevel + 1;
  uint32_t lookupXTile;
  uint32_t lookupYTile;
  uint32_t tileCnt = 2;

  for (uint32_t ty = 0; ty < tileCnt; ty++){
    lookupYTile = ytile *2 + ty;
    for (uint32_t tx = 0; tx < tileCnt; tx++){
      lookupXTile = xtile *2 + tx;
      //qDebug() << "  - lookup tile " << lookupXTile << " " << lookupYTile << " zoom " << lookupTileZoom;
      bool found = false;
      if (tileCache.contains(lookupTileZoom, lookupXTile, lookupYTile)){
        TileCacheVal val = tileCache.get(lookupTileZoom, lookupXTile, lookupYTile);
        if (!val.image.isNull()){
          double imageWidth = val.image.width();
          double imageHeight = val.image.height();
          painter.drawPixmap(
              QRectF(x + tx * (renderTileWidth/tileCnt), y + ty * (renderTileHeight/tileCnt), renderTileWidth/tileCnt + overlap, renderTileHeight/tileCnt + overlap),
              val.image,
              QRectF(0.0, 0.0, imageWidth, imageHeight));
          found = true;
        }
      }
      if (!found && downLimit > 0){
        // recursion
        lookupAndDrawBottomTileRecursive(tileCache, painter,
                                         x + tx * (renderTileWidth/tileCnt), y + ty * (renderTileHeight/tileCnt), renderTileWidth/tileCnt, renderTileHeight/tileCnt, overlap,
                                         zoomLevel +1, lookupXTile, lookupYTile,
                                         downLimit -1);
      }
    }
  }
}
