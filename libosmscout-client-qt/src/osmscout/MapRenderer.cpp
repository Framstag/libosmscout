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

DBRenderJob::DBRenderJob(osmscout::MercatorProjection renderProjection,
                         QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> tiles,
                         osmscout::MapParameter *drawParameter,
                         QPainter *p,
                         bool drawCanvasBackground,
                         bool renderBasemap):
  renderProjection(renderProjection),
  tiles(tiles),
  drawParameter(drawParameter),
  p(p),
  success(false),
  drawCanvasBackground(drawCanvasBackground),
  renderBasemap(renderBasemap)
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
  for (auto &db:databases){
    if (!tiles.contains(db->path)){
      osmscout::log.Debug() << "Skip database " << db->path.toStdString();
      continue;
    }
    std::list<osmscout::TileRef> tileList=tiles[db->path].values().toStdList();
    osmscout::MapData            data;

    db->mapService->AddTileDataToMapData(tileList,data);

    if (drawParameter->GetRenderSeaLand()) {
      db->mapService->GetGroundTiles(renderProjection,
                                     data.groundTiles);
    }

    success&=db->GetPainter()->DrawMap(renderProjection,
                                       *drawParameter,
                                       data,
                                       p);
  }
  Close();
}
