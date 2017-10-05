/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2017  Lukáš Karas

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

#include <osmscout/MapRenderer.h>

MapRenderer::MapRenderer(QThread *thread,
                         SettingsRef settings,
                         DBThreadRef dbThread,
                         QString iconDirectory):
  thread(thread),
  settings(settings),
  dbThread(dbThread),
  lock(QMutex::Recursive),
  iconDirectory(iconDirectory)
{
  mapDpi = settings->GetMapDPI();
  osmscout::log.Debug() << "Map DPI override: " << mapDpi;

  renderSea=settings->GetRenderSea();
  fontName=settings->GetFontName();
  fontSize=settings->GetFontSize();

  connect(settings.get(), SIGNAL(MapDPIChange(double)),
          this, SLOT(onMapDPIChange(double)),
          Qt::QueuedConnection);
  connect(settings.get(), SIGNAL(RenderSeaChanged(bool)),
          this, SLOT(onRenderSeaChanged(bool)),
          Qt::QueuedConnection);
  connect(settings.get(), SIGNAL(FontNameChanged(const QString)),
          this, SLOT(onFontNameChanged(const QString)),
          Qt::QueuedConnection);
  connect(settings.get(), SIGNAL(FontSizeChanged(double)),
          this, SLOT(onFontSizeChanged(double)),
          Qt::QueuedConnection);
  connect(thread, SIGNAL(started()),
          this, SLOT(Initialize()));
  connect(dbThread.get(), SIGNAL(stylesheetFilenameChanged()),
          this, SLOT(onStylesheetFilenameChanged()),
          Qt::QueuedConnection);
}

MapRenderer::~MapRenderer()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  qDebug() << "~MapRenderer";
  if (thread!=NULL){
    thread->quit();
  }
}

void MapRenderer::onStylesheetFilenameChanged(){
  InvalidateVisualCache();
}

void MapRenderer::onMapDPIChange(double dpi)
{
  {
    QMutexLocker locker(&lock);
    mapDpi = dpi;
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::onRenderSeaChanged(bool b)
{
  {
    QMutexLocker locker(&lock);
    renderSea = b;
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::onFontNameChanged(const QString fontName)
{
  {
    QMutexLocker locker(&lock);
    this->fontName=fontName;
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::onFontSizeChanged(double fontSize)
{
  {
    QMutexLocker locker(&lock);
    this->fontSize=fontSize;
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::addOverlayWay(int id,OverlayWayRef way)
{
  {
    QMutexLocker locker(&overlayLock);
    overlayWayMap[id]=way;
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::removeOverlayWay(int id)
{
  {
    QMutexLocker locker(&overlayLock);
    overlayWayMap.erase(id);
  }
  InvalidateVisualCache();
  emit Redraw();
}

std::map<int,OverlayWayRef> MapRenderer::getOverlayWays() const
{
  {
    QMutexLocker locker(&overlayLock);
    return overlayWayMap;
  }
}

osmscout::GeoBox MapRenderer::overlayObjectsBox() const
{
  {
    QMutexLocker locker(&overlayLock);
    osmscout::GeoBox box;
    for (auto &p:overlayWayMap){
      osmscout::GeoBox wayBox=p.second->boundingBox();
      if (wayBox.IsValid()){
        if (box.IsValid()){
          box.Include(wayBox);
        } else {
          box=wayBox;
        }
      }
    }
    return box;
  }
}

void MapRenderer::getOverlayWays(std::vector<OverlayWayRef> &ways,
                                 osmscout::GeoBox requestBox) const
{
  QMutexLocker locker(&overlayLock);
  ways.clear();
  ways.reserve(overlayWayMap.size());
  for (auto &p:overlayWayMap){
    if (requestBox.Intersects(p.second->boundingBox())){
      ways.push_back(p.second);
    }
  }
}

DBRenderJob::DBRenderJob(osmscout::MercatorProjection renderProjection,
                         QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> tiles,
                         osmscout::MapParameter *drawParameter,
                         QPainter *p,
                         std::vector<OverlayWayRef> overlayWays,
                         bool drawCanvasBackground,
                         bool renderBasemap):
  renderProjection(renderProjection),
  tiles(tiles),
  drawParameter(drawParameter),
  p(p),
  success(false),
  drawCanvasBackground(drawCanvasBackground),
  renderBasemap(renderBasemap),
  overlayWays(overlayWays)
{
}

DBRenderJob::~DBRenderJob()
{
}

void DBRenderJob::Run(const osmscout::BasemapDatabaseRef& basemapDatabase,
                      const std::list<DBInstanceRef> &databases,
                      QReadLocker *locker)
{
  DBJob::Run(basemapDatabase,databases,locker);

  bool backgroundRendered=false;
  success=true;

  // draw background
  if (drawCanvasBackground){
    for (auto &db:databases){
      // fill background with "unknown" color
      if (!backgroundRendered && db->styleConfig){
          osmscout::FillStyleRef unknownFillStyle;
          db->styleConfig->GetUnknownFillStyle(renderProjection, unknownFillStyle);
          if (unknownFillStyle){
            osmscout::Color backgroundColor=unknownFillStyle->GetFillColor();
            p->fillRect(QRectF(0,0,renderProjection.GetWidth(),renderProjection.GetHeight()),
                        QColor::fromRgbF(backgroundColor.GetR(),
                                         backgroundColor.GetG(),
                                         backgroundColor.GetB(),
                                         1));
            backgroundRendered=true;
            break;
          }
      }
      if (!backgroundRendered){
        // as backup, when "unknown" style is not defined, use black color
        p->fillRect(QRectF(0,0,renderProjection.GetWidth(),renderProjection.GetHeight()),
                    QBrush(QColor::fromRgbF(0,0,0,1)));
        backgroundRendered=true;
        break;
      }
    }
    if (!backgroundRendered){
      p->fillRect(QRectF(0,0,renderProjection.GetWidth(),renderProjection.GetHeight()),
                  QBrush(QColor::fromRgbF(0,0,0,1)));
    }
  }

  // draw base map
  if (renderBasemap && basemapDatabase && !databases.empty()) {
    osmscout::MapPainterQt* mapPainter=databases.front()->GetPainter();
    osmscout::WaterIndexRef waterIndex=basemapDatabase->GetWaterIndex();

    if (mapPainter && waterIndex) {
      osmscout::GeoBox                boundingBox;
      std::list<osmscout::GroundTile> tiles;

      renderProjection.GetDimensions(boundingBox);
      if (waterIndex->GetRegions(boundingBox,
                                 renderProjection.GetMagnification(),
                                 tiles)) {

        mapPainter->DrawGroundTiles(renderProjection,
                                    *drawParameter,
                                    tiles,
                                    p);

        backgroundRendered=true;
      }
    }
  }

  // draw databases
  size_t i=0;
  bool last;

  for (auto &db:databases){
    last=(i==databases.size()-1);
    ++i;

    std::list<osmscout::TileRef> tileList;
    if (tiles.contains(db->path)){
      tileList=tiles[db->path].values().toStdList();
    }else{
      if (!last){
        osmscout::log.Debug() << "Skip database " << db->path.toStdString();
        continue;
      }
    }
    
    osmscout::MapData data;

    db->mapService->AddTileDataToMapData(tileList,data);
    if (last){
      osmscout::TypeConfigRef typeConfig=db->database->GetTypeConfig();
      for (auto const &ow:overlayWays){
        osmscout::WayRef w=std::make_shared<osmscout::Way>();
        if (ow->toWay(w,*typeConfig)){
          data.poiWays.push_back(w);
        }
      }
    }

    if (drawParameter->GetRenderSeaLand()) {
      db->mapService->GetGroundTiles(renderProjection,
                                     data.groundTiles);
    }

    auto painter=db->GetPainter();
    if (painter) {
      success &= painter->DrawMap(renderProjection,
                                  *drawParameter,
                                  data,
                                  p);
    }else{
      osmscout::log.Warn() << "Painter is not available for database: " << db->path.toStdString();
      success=false;
    }
  }
  Close();
}
