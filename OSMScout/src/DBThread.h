#ifndef DBTHREAD_H
#define DBTHREAD_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <QtGui>
#include <QThread>
#include <QMetaType>
#include <QMutex>

#include "config.h"

#if defined(HAVE_LIB_QTOPENGL)
 #include <QGLPixelBuffer>
#endif

#include <osmscout/Database.h>
#include <osmscout/Router.h>
#include <osmscout/RoutePostprocessor.h>

#include <osmscout/MapPainterQt.h>

#include <osmscout/util/Breaker.h>

struct RenderMapRequest
{
  double                  lon;
  double                  lat;
  osmscout::Magnification magnification;
  size_t                  width;
  size_t                  height;
};

Q_DECLARE_METATYPE(RenderMapRequest)

struct DatabaseLoadedResponse
{
  double minLat;
  double minLon;
  double maxLat;
  double maxLon;
};

Q_DECLARE_METATYPE(DatabaseLoadedResponse)

class QBreaker : public osmscout::Breaker
{
private:
  mutable QMutex mutex;
  bool           aborted;

public:
  QBreaker();

  bool Break();
  bool IsAborted() const;
  void Reset();
};

class DBThread : public QObject
{
  Q_OBJECT

signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void HandleMapRenderingResult();
  void Redraw();

public slots:
  void TriggerMapRendering();
  void Initialize();
  void Finalize();

private:
  mutable QMutex               mutex;
  osmscout::DatabaseParameter  databaseParameter;
  osmscout::Database           database;
  osmscout::StyleConfig        *styleConfig;
  osmscout::MapData            data;
  osmscout::MapPainterQt       painter;
  osmscout::RouterParameter    routerParameter;
  osmscout::Router             router;
  osmscout::FastestPathRoutingProfile routingProfile;
  osmscout::RoutePostprocessor routePostprocessor;
  QString                      iconDirectory;

  QImage                       *currentImage;
#if defined(HAVE_LIB_QTOPENGL)
  QGLPixelBuffer               *currentGLPixmap;
  QSize                        currentGLPixmapSize;
#endif
  double                       currentLat;
  double                       currentLon;
  osmscout::Magnification      currentMagnification;

  QImage                       *finishedImage;
#if defined(HAVE_LIB_QTOPENGL)
  QGLPixelBuffer               *finishedGLPixmap;
  QSize                        finishedGLPixmapSize;
#endif
  double                       finishedLat;
  double                       finishedLon;
  osmscout::Magnification      finishedMagnification;

  RenderMapRequest             currentRenderRequest;
  bool                         doRender;
  QBreaker*                    renderBreaker;
  osmscout::BreakerRef         renderBreakerRef;

private:
  void FreeMaps();

public:
  DBThread();

  void UpdateRenderRequest(const RenderMapRequest& request);

  bool RenderMap(QPainter& painter,
                 const RenderMapRequest& request);


  bool GetNodeByOffset(osmscout::FileOffset offset, osmscout::NodeRef& node) const;
  bool GetWayByOffset(osmscout::FileOffset offset, osmscout::WayRef& way) const;
  bool GetRelationByOffset(osmscout::FileOffset offset, osmscout::RelationRef& relation) const;

  bool GetMatchingAdminRegions(const QString& name,
                               std::list<osmscout::AdminRegion>& regions,
                               size_t limit,
                               bool& limitReached) const;

  bool GetMatchingLocations(const osmscout::AdminRegion& region,
                            const QString& name,
                            std::list<osmscout::Location>& locations,
                            size_t limit,
                            bool& limitReached) const;

  bool CalculateRoute(osmscout::Id startWayId,
                      size_t startNodeIndex,
                      osmscout::Id targetWayId,
                      size_t targetNodeIndex,
                      osmscout::RouteData& route);

  bool TransformRouteDataToRouteDescription(const osmscout::RouteData& data,
                                            osmscout::RouteDescription& description,
                                            const std::string& start,
                                            const std::string& target);
  bool TransformRouteDataToWay(const osmscout::RouteData& data,
                               osmscout::Way& way);

  void ClearRoute();
  void AddRoute(const osmscout::Way& way);
};

extern DBThread dbThread;

#endif
