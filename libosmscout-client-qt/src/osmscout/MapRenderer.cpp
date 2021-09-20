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

namespace osmscout {

MapRenderer::MapRenderer(QThread *thread,
                         SettingsRef settings,
                         DBThreadRef dbThread,
                         QString iconDirectory):
  thread(thread),
  settings(settings),
  dbThread(dbThread),
  iconDirectory(iconDirectory)
{
  mapDpi = settings->GetMapDPI();
  osmscout::log.Debug() << "Map DPI override: " << mapDpi;

  renderSea=settings->GetRenderSea();
  fontName=settings->GetFontName();
  fontSize=settings->GetFontSize();
  showAltLanguage=settings->GetShowAltLanguage();
  units=settings->GetUnits();

  connect(settings.get(), &Settings::MapDPIChange,
          this, &MapRenderer::onMapDPIChange,
          Qt::QueuedConnection);
  connect(settings.get(), &Settings::RenderSeaChanged,
          this, &MapRenderer::onRenderSeaChanged,
          Qt::QueuedConnection);
  connect(settings.get(), &Settings::FontNameChanged,
          this, &MapRenderer::onFontNameChanged,
          Qt::QueuedConnection);
  connect(settings.get(), &Settings::FontSizeChanged,
          this, &MapRenderer::onFontSizeChanged,
          Qt::QueuedConnection);
  connect(settings.get(), &Settings::ShowAltLanguageChanged,
          this, &MapRenderer::onShowAltLanguageChanged,
          Qt::QueuedConnection);
  connect(settings.get(), &Settings::UnitsChanged,
          this, &MapRenderer::onUnitsChanged,
          Qt::QueuedConnection);
  connect(thread, &QThread::started,
          this, &MapRenderer::Initialize);
  connect(dbThread.get(), &DBThread::stylesheetFilenameChanged,
          this, &MapRenderer::onStylesheetFilenameChanged,
          Qt::QueuedConnection);
}

MapRenderer::~MapRenderer()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  qDebug() << "~MapRenderer";
  if (thread!=nullptr){
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

void MapRenderer::onFontNameChanged(const QString& fontName)
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

void MapRenderer::onShowAltLanguageChanged(bool showAltLanguage)
{
  {
    QMutexLocker locker(&lock);
    this->showAltLanguage=showAltLanguage;
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::onUnitsChanged(const QString& units)
{
  {
    QMutexLocker locker(&lock);
    this->units=units;
  }
  InvalidateVisualCache();
  emit Redraw();
}

void MapRenderer::addOverlayObject(int id, const OverlayObjectRef& obj)
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
    for (const auto &p:overlayObjectMap){
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
  for (const auto &p:overlayObjectMap){
    if (requestBox.Intersects(p.second->boundingBox())){
      objs.push_back(p.second);
    }
  }
}

DBRenderJob::DBRenderJob(osmscout::MercatorProjection renderProjection,
                         QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles,
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
    for (const auto &db:databases){
      // fill background with "unknown" color
      if (!backgroundRendered && db->GetStyleConfig()){
          osmscout::FillStyleRef unknownFillStyle=db->GetStyleConfig()->GetUnknownFillStyle(renderProjection);
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

  // prepare data for batch
  osmscout::MapPainterBatchQt batch(databases.size());
  size_t i=0;
  for (const auto &db:databases){
    bool first=(i==0);
    bool last=(i==databases.size()-1);
    bool skip=true;
    ++i;

    std::list<osmscout::TileRef> tileList;
    if (tiles.contains(db->path)){
      auto list = tiles[db->path].values();
      tileList=std::list<osmscout::TileRef>(list.begin(), list.end());
      skip=false;
    }

    osmscout::MapDataRef data=std::make_shared<osmscout::MapData>();
    db->GetMapService()->AddTileDataToMapData(tileList,*data);

    if (first){
      // draw base map
      if (renderBasemap && basemapDatabase) {
        osmscout::WaterIndexRef waterIndex=basemapDatabase->GetWaterIndex();
        if (waterIndex) {
          osmscout::GeoBox                boundingBox;
          renderProjection.GetDimensions(boundingBox);
          if (waterIndex->GetRegions(boundingBox,
                                     renderProjection.GetMagnification(),
                                     data->baseMapTiles)) {
          }
          skip=false;
        }
      }
    }

    if (last){
      osmscout::TypeConfigRef typeConfig=db->GetDatabase()->GetTypeConfig();
      for (auto const &o:overlayObjects){

        if (o->getObjectType()==osmscout::RefType::refWay){
          OverlayWay *ow=dynamic_cast<OverlayWay*>(o.get());
          if (ow != nullptr) {
            osmscout::WayRef w = std::make_shared<osmscout::Way>();
            if (ow->toWay(w, *typeConfig)) {
              data->poiWays.push_back(w);
            }
          }
        } else if (o->getObjectType()==osmscout::RefType::refArea){
          OverlayArea *oa=dynamic_cast<OverlayArea*>(o.get());
          if (oa != nullptr) {
            osmscout::AreaRef a = std::make_shared<osmscout::Area>();
            if (oa->toArea(a, *typeConfig)) {
              data->poiAreas.push_back(a);
            }
          }
        } else if (o->getObjectType()==osmscout::RefType::refNode){
          OverlayNode *oo=dynamic_cast<OverlayNode*>(o.get());
          if (oo != nullptr) {
            osmscout::NodeRef n = std::make_shared<osmscout::Node>();
            if (oo->toNode(n, *typeConfig)) {
              data->poiNodes.push_back(n);
            }
          }
        }
      }
      skip&=overlayObjects.empty();
    }

    if (skip){
      osmscout::log.Debug() << "Skip database " << db->path.toStdString();
      continue;
    }

    if (drawParameter->GetRenderSeaLand()) {
      db->GetMapService()->GetGroundTiles(renderProjection,
                                       data->groundTiles);
    }

    auto *painter=db->GetPainter();
    if (painter != nullptr) {
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
}
