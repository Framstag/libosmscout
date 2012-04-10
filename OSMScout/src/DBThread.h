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

struct RenderMapRequest
{
  double lon;
  double lat;
  double magnification;
  size_t width;
  size_t height;
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

class DBThread : public QThread
{
  Q_OBJECT

signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void HandleMapRenderingResult();
  void Redraw();

public slots:
  void TriggerMapRendering(const RenderMapRequest& request);

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

  bool                         finish;

  QPixmap                      *currentPixmap;
#if defined(HAVE_LIB_QTOPENGL)
  QGLPixelBuffer               *currentGLPixmap;
#endif
  double                       currentLon,currentLat;
  double                       currentMagnification;

  QPixmap                      *finishedPixmap;
#if defined(HAVE_LIB_QTOPENGL)
  QGLPixelBuffer               *finishedGLPixmap;
#endif
  double                       finishedLon,finishedLat;
  double                       finishedMagnification;

private:
  void FreeMaps();

public:
  DBThread();

  void run();

  bool RenderMap(QPainter& painter,
                 const RenderMapRequest& request);


  bool GetNode(osmscout::Id id, osmscout::NodeRef& node) const;
  bool GetWay(osmscout::Id id, osmscout::WayRef& way) const;
  bool GetRelation(osmscout::Id id, osmscout::RelationRef& relation) const;

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
                      osmscout::Id startNodeId,
                      osmscout::Id targetWayId,
                      osmscout::Id targetNodeId,
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
