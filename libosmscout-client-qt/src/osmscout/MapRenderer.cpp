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

void MapRenderer::addOverlayObject(int id, OverlayObjectRef obj)
{
  {
    QMutexLocker locker(&overlayLock);
    overlayObjectMap[id]=obj;
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::removeOverlayObject(int id)
{
  {
    QMutexLocker locker(&overlayLock);
    overlayObjectMap.erase(id);
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::removeAllOverlayObjects()
{
  bool change;
  {
    QMutexLocker locker(&overlayLock);
    change=!overlayObjectMap.empty();
    overlayObjectMap.clear();
  }
  if (change) {
    InvalidateVisualCache();
    emit Redraw();
  }
}

std::map<int,OverlayObjectRef> MapRenderer::getOverlayObjects() const
{
  {
    QMutexLocker locker(&overlayLock);
    return overlayObjectMap;
  }
}

osmscout::GeoBox MapRenderer::overlayObjectsBox() const
{
  {
    QMutexLocker locker(&overlayLock);
    osmscout::GeoBox box;
    for (auto &p:overlayObjectMap){
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

void MapRenderer::getOverlayObjects(std::vector<OverlayObjectRef> &objs,
                                    osmscout::GeoBox requestBox) const
{
  QMutexLocker locker(&overlayLock);
  objs.clear();
  objs.reserve(overlayObjectMap.size());
  for (auto &p:overlayObjectMap){
    if (requestBox.Intersects(p.second->boundingBox())){
      objs.push_back(p.second);
    }
  }
}

DBRenderJob::DBRenderJob(osmscout::MercatorProjection renderProjection,
                         QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> tiles,
                         osmscout::MapParameter *drawParameter,
                         QPainter *p,
                         std::vector<OverlayObjectRef> overlayObjects,
                         bool drawCanvasBackground,
                         bool renderBasemap):
  renderProjection(renderProjection),
  tiles(tiles),
  drawParameter(drawParameter),
  p(p),
  success(false),
  drawCanvasBackground(drawCanvasBackground),
  renderBasemap(renderBasemap),
  overlayObjects(overlayObjects)
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
          osmscout::FillStyleRef unknownFillStyle=db->styleConfig->GetUnknownFillStyle(renderProjection);
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

  // prepare data for batch
  osmscout::MapPainterBatchQt batch(databases.size());
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

    osmscout::MapDataRef data=std::make_shared<osmscout::MapData>();
    db->mapService->AddTileDataToMapData(tileList,*data);
    if (last){
      osmscout::TypeConfigRef typeConfig=db->database->GetTypeConfig();
      for (auto const &o:overlayObjects){

        if (o->getObjectType()==osmscout::RefType::refWay){
          OverlayWay *ow=dynamic_cast<OverlayWay*>(o.get());
          if (ow != NULL) {
            osmscout::WayRef w = std::make_shared<osmscout::Way>();
            if (ow->toWay(w, *typeConfig)) {
              data->poiWays.push_back(w);
            }
          }
        } else if (o->getObjectType()==osmscout::RefType::refArea){
          OverlayArea *oa=dynamic_cast<OverlayArea*>(o.get());
          if (oa != NULL) {
            osmscout::AreaRef a = std::make_shared<osmscout::Area>();
            if (oa->toArea(a, *typeConfig)) {
              data->poiAreas.push_back(a);
            }
          }
        } else if (o->getObjectType()==osmscout::RefType::refNode){
          OverlayNode *oo=dynamic_cast<OverlayNode*>(o.get());
          if (oo != NULL) {
            osmscout::NodeRef n = std::make_shared<osmscout::Node>();
            if (oo->toNode(n, *typeConfig)) {
              data->poiNodes.push_back(n);
            }
          }
        }
      }
    }

    if (drawParameter->GetRenderSeaLand()) {
      db->mapService->GetGroundTiles(renderProjection,
                                     data->groundTiles);
    }

    auto painter=db->GetPainter();
    if (painter) {
      batch.addData(data, painter);
    }else{
      osmscout::log.Warn() << "Painter is not available for database: " << db->path.toStdString();
      success=false;
    }
  }

  // draw databases
  success &= batch.paint(renderProjection,
                         *drawParameter,
                         p);
  Close();
}
