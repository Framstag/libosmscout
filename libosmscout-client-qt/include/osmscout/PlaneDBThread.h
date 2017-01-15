#ifndef OSMSCOUT_CLIENT_QT_PLANE_DBTHREAD_H
#define OSMSCOUT_CLIENT_QT_PLANE_DBTHREAD_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings
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

#include <QtGui>
#include <QThread>
#include <QMetaType>
#include <QMutex>
#include <QTime>
#include <QTimer>

#include <osmscout/Database.h>
#include <osmscout/LocationService.h>
#include <osmscout/MapService.h>
#include <osmscout/RoutingService.h>
#include <osmscout/RoutePostprocessor.h>

#include <osmscout/MapPainterQt.h>

#include <osmscout/util/Breaker.h>

#include <osmscout/Settings.h>
#include <osmscout/DBThread.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API PlaneDBThread : public DBThread
{
  Q_OBJECT
  
signals:
  void TriggerDrawMap();
  void TileStatusChanged(const osmscout::TileRef& tile);
  void TriggerMapRenderingSignal(const RenderMapRequest& request);
  void TriggerInitialRendering();
  
public slots:
  void DrawMap();
  void HandleTileStatusChanged(const osmscout::TileRef& changedTile);
  void TriggerMapRendering(const RenderMapRequest& request);
  void HandleInitialRenderingRequest();
  void onStylesheetFilenameChanged();

private:
  osmscout::MercatorProjection  projection;

  mutable QMutex                lastRequestMutex;
  RenderMapRequest              lastRequest;

  QTime                         lastRendering;
  QTimer                        pendingRenderingTimer;

  QImage                        *currentImage;
  size_t                        currentWidth;
  size_t                        currentHeight;
  osmscout::GeoCoord            currentCoord;
  double                        currentAngle;
  osmscout::Magnification       currentMagnification;
  
  mutable QMutex                finishedMutex;  // mutex protecting access to finished* variables
                                                // to avoid deadlock, we should not acquire global mutex when holding finishedMutex
  QImage                        *finishedImage;
  osmscout::GeoCoord            finishedCoord;
  double                        finishedAngle;
  osmscout::Magnification       finishedMagnification;
  osmscout::FillStyleRef        finishedUnknownFillStyle;

protected:  
  virtual void TileStateCallback(const osmscout::TileRef& changedTile);

public:
  PlaneDBThread(QStringList databaseLookupDirs, 
                QString stylesheetFilename, 
                QString iconDirectory);
    
  virtual ~PlaneDBThread();

  virtual void Initialize();

  virtual bool RenderMap(QPainter& painter,
                         const RenderMapRequest& request);
  
  virtual void InvalidateVisualCache();
};

#endif
