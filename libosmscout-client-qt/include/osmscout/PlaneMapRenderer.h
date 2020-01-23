#ifndef OSMSCOUT_CLIENT_QT_PLANEMAPRENDERER_H
#define OSMSCOUT_CLIENT_QT_PLANEMAPRENDERER_H

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

#include <QObject>
#include <QSettings>
#include <QElapsedTimer>

#include <osmscout/DataTileCache.h>
#include <osmscout/DBThread.h>
#include <osmscout/MapRenderer.h>

#include <osmscout/ClientQtImportExport.h>

namespace osmscout {

class OSMSCOUT_CLIENT_QT_API PlaneMapRenderer : public MapRenderer {
  Q_OBJECT

private:
  double                        canvasOverrun; // scale of rendered canvas, relative to screen dimensions
  osmscout::MercatorProjection  projection;

  mutable QMutex                lastRequestMutex;
  MapViewStruct                 lastRequest;

  DBLoadJob                     *loadJob;

  QElapsedTimer                 lastRendering;
  QTimer                        pendingRenderingTimer;

  QImage                        *currentImage;
  size_t                        currentWidth;
  size_t                        currentHeight;
  osmscout::GeoCoord            currentCoord;
  double                        currentAngle;   // radians
  osmscout::Magnification       currentMagnification;
  size_t                        currentEpoch{0}; /// guarded by global mutex

  mutable QMutex                finishedMutex;  // mutex protecting access to finished* variables
                                                // to avoid deadlock, we should not acquire global mutex when holding finishedMutex
                                                // reverse order is possible
  QImage                        *finishedImage;
  size_t                        finishedEpoch{0};
  osmscout::GeoCoord            finishedCoord;
  double                        finishedAngle;
  osmscout::Magnification       finishedMagnification;
  osmscout::FillStyleRef        finishedUnknownFillStyle;

  /**
   * epoch is incremented when data or rendering parameters are changed
   * @see InvalidateVisualCache
   * guarded by finishedMutex
   */
  size_t                        epoch{0};

signals:
  //void TileStatusChanged(const osmscout::TileRef& tile);
  void TriggerMapRenderingSignal(const MapViewStruct& request, size_t requestEpoch);
  void TriggerInitialRendering();

public slots:
  virtual void Initialize();
  virtual void InvalidateVisualCache();
  void DrawMap();
  void HandleTileStatusChanged(QString dbPath,const osmscout::TileRef tile);
  void onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>>);
  void TriggerMapRendering(const MapViewStruct& request, size_t requestEpoch);
  void HandleInitialRenderingRequest();
  virtual void onStylesheetFilenameChanged();

public:
  PlaneMapRenderer(QThread *thread,
                   SettingsRef settings,
                   DBThreadRef dbThread,
                   QString iconDirectory);

  virtual ~PlaneMapRenderer();

  /**
   * Render map defined by request to painter
   * @param painter
   * @param request
   * @return true if rendered map is complete
   */
  virtual bool RenderMap(QPainter& painter,
                         const MapViewStruct& request);

private:
  double computeScale(const osmscout::MercatorProjection &previousProjection,
                      const osmscout::MercatorProjection &currentProjection);
};

}

#endif /* OSMSCOUT_CLIENT_QT_PLANEMAPRENDERER_H */
