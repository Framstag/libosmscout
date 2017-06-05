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

#ifndef MAPRENDERER_H
#define MAPRENDERER_H

#include <QObject>
#include <QSettings>

#include <osmscout/DataTileCache.h>
#include <osmscout/DBThread.h>

#include <osmscout/private/ClientQtImportExport.h>

class OSMSCOUT_CLIENT_QT_API DBRenderJob : public DBJob{
  Q_OBJECT
private:
  osmscout::MercatorProjection renderProjection;
  QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> tiles;
  osmscout::MapParameter *drawParameter;
  QPainter *p;
  bool success;
  bool drawCanvasBackground;
  bool renderBasemap;

public:
  DBRenderJob(osmscout::MercatorProjection renderProjection,
              QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> tiles,
              osmscout::MapParameter *drawParameter,
              QPainter *p,
              bool drawCanvasBackground=true,
              bool renderBasemap=true);
  virtual ~DBRenderJob();

  virtual void Run(const osmscout::BasemapDatabaseRef& basemapDatabase,
                   const std::list<DBInstanceRef> &databases,
                   QReadLocker *locker);

  inline bool IsSuccess(){
    return success;
  };
};

class OSMSCOUT_CLIENT_QT_API MapRenderer : public QObject {
  Q_OBJECT

protected:
  QThread     *thread;
  SettingsRef settings;
  DBThreadRef dbThread;
  QMutex      lock;

  double      mapDpi;
  bool        renderSea;

  QString     fontName;
  double      fontSize;
  QString     iconDirectory;

signals:
  void Redraw();
  void TriggerDrawMap();

public slots:
  virtual void Initialize() = 0;

  virtual void InvalidateVisualCache() = 0;
  virtual void onStylesheetFilenameChanged();

  virtual void onMapDPIChange(double dpi);
  virtual void onRenderSeaChanged(bool);
  virtual void onFontNameChanged(const QString);
  virtual void onFontSizeChanged(double);

protected:
  MapRenderer(QThread *thread,
              SettingsRef settings,
              DBThreadRef dbThread,
              QString iconDirectory);

public:
  virtual ~MapRenderer();

  /**
   * Render map defined by request to painter
   * @param painter
   * @param request
   * @return true if rendered map is complete
   */
  virtual bool RenderMap(QPainter& painter,
                         const RenderMapRequest& request) = 0;
};

typedef std::shared_ptr<MapRenderer> MapRendererRef;

#endif /* MAPRENDERER_H */

