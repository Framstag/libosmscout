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

#include <osmscoutclientqt/MapRenderer.h>

#include <osmscoutmapqt/MapPainterQt.h>

#include <QDebug>

namespace osmscout {

MapRenderer::MapRenderer(QThread *thread,
                         SettingsRef settings,
                         DBThreadRef dbThread,
                         const QString &iconDirectory,
                         const PixelRatioSetup &pixelRatio):
  thread(thread),
  settings(settings),
  dbThread(dbThread),
  pixelRatio(pixelRatio),
  iconDirectory(iconDirectory)
{
  mapDpi = settings->GetMapDPI();
  osmscout::log.Debug() << "Map DPI override: " << mapDpi;

  renderSea=settings->GetRenderSea();
  fontName=QString::fromStdString(settings->GetFontName());
  fontSize=settings->GetFontSize();
  showAltLanguage=settings->GetShowAltLanguage();
  units=QString::fromStdString(settings->GetUnits());

  settings->mapDPIChange.Connect(mapDpiChangeSlot);
  settings->renderSeaChanged.Connect(renderSeaSlot);
  settings->fontNameChanged.Connect(fontNameSlot);
  settings->fontSizeChanged.Connect(fontSizeSlot);
  settings->showAltLanguageChanged.Connect(showAltLanguageSlot);
  settings->unitsChanged.Connect(unitsSlot);
  dbThread->stylesheetFilenameChanged.Connect(stylesheetFilenameChangedSlot);
  dbThread->databaseLoadFinished.Connect(databaseLoadFinishedSlot);

  connect(this, &MapRenderer::mapDpiChangeSignal,
          this, &MapRenderer::onMapDPIChange,
          Qt::QueuedConnection);
  connect(this, &MapRenderer::renderSeaSignal,
          this, &MapRenderer::onRenderSeaChanged,
          Qt::QueuedConnection);
  connect(this, &MapRenderer::fontNameSignal,
          this, &MapRenderer::onFontNameChanged,
          Qt::QueuedConnection);
  connect(this, &MapRenderer::fontSizeSignal,
          this, &MapRenderer::onFontSizeChanged,
          Qt::QueuedConnection);
  connect(this, &MapRenderer::showAltLanguageSignal,
          this, &MapRenderer::onShowAltLanguageChanged,
          Qt::QueuedConnection);
  connect(this, &MapRenderer::unitsSignal,
          this, &MapRenderer::onUnitsChanged,
          Qt::QueuedConnection);

  connect(thread, &QThread::started,
          this, &MapRenderer::Initialize);
  connect(this, &MapRenderer::stylesheetFilenameChanged,
          this, &MapRenderer::onStylesheetFilenameChanged,
          Qt::QueuedConnection);
  connect(this, &MapRenderer::databaseLoadFinished,
          this, &MapRenderer::onDatabaseLoaded,
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

void MapRenderer::SetScreen(const QScreen *screen)
{
  bool changed=false;
  {
    QMutexLocker locker(&lock);
    if (std::holds_alternative<ScreenPixelRatio>(this->pixelRatio)
        && std::get<ScreenPixelRatio>(this->pixelRatio).ratio != screen->devicePixelRatio()) {
      this->pixelRatio = ScreenPixelRatio{screen->devicePixelRatio()};
      log.Debug() << "Screen pixel ratio: " << screen->devicePixelRatio();
      changed = true;
    }
  }
  if (changed) {
    InvalidateVisualCache();
    emit Redraw();
  }
}

void MapRenderer::addOverlayObject(int id, const OverlayObjectRef& obj)
{
  {
    QMutexLocker locker(&overlayLock);
    overlayObjectMap[id]=obj;
  }
  InvalidateVisualCache();
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
                         MapPainterQt *mapPainter,
                         std::vector<OverlayObjectRef> overlayObjects,
                         StyleConfigRef emptyStyleConfig,
                         bool drawCanvasBackground,
                         bool renderBasemap,
                         bool renderDatabases):
  renderProjection(renderProjection),
  tiles(tiles),
  drawParameter(drawParameter),
  p(p),
  mapPainter(mapPainter),
  success(false),
  drawCanvasBackground(drawCanvasBackground),
  renderBasemap(renderBasemap),
  renderDatabases(renderDatabases),
  overlayObjects(overlayObjects),
  emptyStyleConfig(emptyStyleConfig)
{
}

void DBRenderJob::Run(const DBInstanceRef& basemapDatabase,
                      const std::list<DBInstanceRef> &allDatabases,
                      ReadLock &&locker)
{
  std::list<DBInstanceRef> databases; // enabled databases for rendering
  if (renderDatabases) {
    databases = allDatabases;
  }
  DBJob::Run(basemapDatabase,databases,std::move(locker));

  if (renderBasemap && basemapDatabase) {
    databases.push_back(basemapDatabase); // basemap database is a ordinary database after all
  }

  success=true;

  // draw background
  if (drawCanvasBackground){
    bool backgroundRendered=false;
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
    }
    if (!backgroundRendered && emptyStyleConfig) {
      osmscout::FillStyleRef unknownFillStyle=emptyStyleConfig->GetUnknownFillStyle(renderProjection);
      if (unknownFillStyle){
        osmscout::Color backgroundColor=unknownFillStyle->GetFillColor();
        p->fillRect(QRectF(0,0,renderProjection.GetWidth(),renderProjection.GetHeight()),
                    QColor::fromRgbF(backgroundColor.GetR(),
                                     backgroundColor.GetG(),
                                     backgroundColor.GetB(),
                                     1));
        backgroundRendered=true;
      }
    }
    if (!backgroundRendered){
      // as backup, when "unknown" style is not defined, use black color
      p->fillRect(QRectF(0,0,renderProjection.GetWidth(),renderProjection.GetHeight()),
                  QBrush(QColor::fromRgbF(0,0,0,1)));
    }
  }

  // prepare data for batch
  std::vector<MapData> batch;
  batch.reserve(databases.size());
  size_t i=0;
  for (const auto &db: databases) {
    bool last = (i == databases.size() - 1);
    bool skip = true;
    ++i;

    std::list<osmscout::TileRef> tileList;
    if (tiles.contains(QString::fromStdString(db->path))) {
      auto list = tiles[QString::fromStdString(db->path)].values();
      tileList = std::list<osmscout::TileRef>(list.begin(), list.end());
      skip = false;
    }

    MapData data;
    data.styleConfig=db->GetStyleConfig();
    db->GetMapService()->AddTileDataToMapData(tileList, data);

    if (last) {
      osmscout::TypeConfigRef typeConfig = db->GetDatabase()->GetTypeConfig();
      skip &= !addOverlayObjectData(data, typeConfig);
    }

    if (skip) {
      osmscout::log.Debug() << "Skip db " << db->path;
      continue;
    }

    if (drawParameter->GetRenderSeaLand()) {
      if (db->GetDatabase()->IsBasemap()) {
        db->GetMapService()->GetGroundTiles(renderProjection,
                                            data.baseMapTiles);
      } else {
        db->GetMapService()->GetGroundTiles(renderProjection,
                                            data.groundTiles);
      }
    }

    batch.emplace_back(std::move(data));
  }

  // draw databases
  success &= mapPainter->DrawMap(renderProjection,
                                 *drawParameter,
                                 batch,
                                 p);
  Close();
}

bool DBRenderJob::addOverlayObjectData(MapData &data, TypeConfigRef typeConfig) const
{
  for (auto const &o: overlayObjects) {

    if (o->getObjectType() == osmscout::RefType::refWay) {
      OverlayWay *ow = dynamic_cast<OverlayWay *>(o.get());
      if (ow != nullptr) {
        osmscout::WayRef w = std::make_shared<osmscout::Way>();
        if (ow->toWay(w, *typeConfig)) {
          data.poiWays.push_back(w);
        }
      }
    } else if (o->getObjectType() == osmscout::RefType::refArea) {
      OverlayArea *oa = dynamic_cast<OverlayArea *>(o.get());
      if (oa != nullptr) {
        osmscout::AreaRef a = std::make_shared<osmscout::Area>();
        if (oa->toArea(a, *typeConfig)) {
          data.poiAreas.push_back(a);
        }
      }
    } else if (o->getObjectType() == osmscout::RefType::refNode) {
      OverlayNode *oo = dynamic_cast<OverlayNode *>(o.get());
      if (oo != nullptr) {
        osmscout::NodeRef n = std::make_shared<osmscout::Node>();
        if (oo->toNode(n, *typeConfig)) {
          data.poiNodes.push_back(n);
        }
      }
    }
  }

  return !overlayObjects.empty();
}
}
