#ifndef OSMSCOUT_CLIENT_QT_DBTHREAD_H
#define OSMSCOUT_CLIENT_QT_DBTHREAD_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/private/ClientQtImportExport.h>

#include <osmscout/Settings.h>

struct OSMSCOUT_CLIENT_QT_API RenderMapRequest
{
  osmscout::GeoCoord      coord;
  double                  angle;
  osmscout::Magnification magnification;
  size_t                  width;
  size_t                  height;
};

Q_DECLARE_METATYPE(RenderMapRequest)

struct OSMSCOUT_CLIENT_QT_API DatabaseLoadedResponse
{
    osmscout::GeoBox boundingBox;
};

Q_DECLARE_METATYPE(DatabaseLoadedResponse)

class OSMSCOUT_CLIENT_QT_API QBreaker : public osmscout::Breaker
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

class OSMSCOUT_CLIENT_QT_API StyleError
{
    enum StyleErrorType {
        Symbol, Error, Warning, Exception
    };

public:
    StyleError(StyleErrorType type, int line, int column, const QString &text) :
        type(type), line(line), column(column), text(text){}
    StyleError(QString msg);

    StyleErrorType GetType(){ return type; }
    QString GetTypeName() const;
    int GetLine(){ return line; }
    int GetColumn(){ return column; }
    const QString &GetText(){ return text; }

private:
    StyleErrorType  type;
    int             line;
    int             column;
    QString         text;
};

class OSMSCOUT_CLIENT_QT_API DBThread : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString stylesheetFilename READ GetStylesheetFilename)

signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void HandleMapRenderingResult();
  void TriggerInitialRendering();
  void TriggerDrawMap();
  void Redraw();
  void TileStatusChanged(const osmscout::TileRef& tile);
  void stylesheetFilenameChanged();

public slots:
  void ToggleDaylight();
  bool ReloadStyle(const QString &suffix="");
  void HandleInitialRenderingRequest();
  void HandleTileStatusChanged(const osmscout::TileRef& changedTile);
  void DrawMap();
  void TriggerMapRendering(const RenderMapRequest& request);
  void Initialize();
  void Finalize();

private:
  double                        dpi;

  mutable QMutex                mutex;

  osmscout::DatabaseParameter   databaseParameter;
  osmscout::DatabaseRef         database;
  osmscout::LocationServiceRef  locationService;
  osmscout::MapServiceRef       mapService;
  osmscout::MapService::CallbackId callbackId;
  osmscout::MercatorProjection  projection;
  osmscout::RouterParameter     routerParameter;
  osmscout::RoutingServiceRef   router;
  osmscout::RoutePostprocessor  routePostprocessor;

  QString                       stylesheetFilename;
  bool                          daylight;
  osmscout::StyleConfigRef      styleConfig;
  osmscout::MapData             data;
  osmscout::MapPainterQt        *painter;
  QString                       iconDirectory;

  QTime                         lastRendering;
  QTimer                        pendingRenderingTimer;

  QImage                        *currentImage;
  size_t                        currentWidth;
  size_t                        currentHeight;
  osmscout::GeoCoord            currentCoord;
  double                        currentAngle;
  osmscout::Magnification       currentMagnification;

  QImage                        *finishedImage;
  osmscout::GeoCoord            finishedCoord;
  double                        finishedAngle;
  osmscout::Magnification       finishedMagnification;

  osmscout::BreakerRef          dataLoadingBreaker;

  bool                          renderError;
  QList<StyleError>             styleErrors;

private:
  DBThread();
  virtual ~DBThread();

  void FreeMaps();
  bool AssureRouter(osmscout::Vehicle vehicle);

  void TileStateCallback(const osmscout::TileRef& changedTile);

public:
  QString GetStylesheetFilename() const;

  const QList<StyleError> &GetStyleErrors() const
  {
      return styleErrors;
  }

  void GetProjection(osmscout::MercatorProjection& projection);

  void CancelCurrentDataLoading();

  bool RenderMap(QPainter& painter,
                 const RenderMapRequest& request);
  void RenderMessage(QPainter& painter, qreal width, qreal height, const char* message);

  osmscout::TypeConfigRef GetTypeConfig() const;

  bool GetNodeByOffset(osmscout::FileOffset offset,
                       osmscout::NodeRef& node) const;
  bool GetAreaByOffset(osmscout::FileOffset offset,
                       osmscout::AreaRef& relation) const;
  bool GetWayByOffset(osmscout::FileOffset offset,
                      osmscout::WayRef& way) const;

  bool ResolveAdminRegionHierachie(const osmscout::AdminRegionRef& adminRegion,
                                   std::map<osmscout::FileOffset,osmscout::AdminRegionRef >& refs) const;

  bool SearchForLocations(const std::string& searchPattern,
                          size_t limit,
                          osmscout::LocationSearchResult& result) const;

  bool CalculateRoute(const osmscout::RoutingProfile& routingProfile,
                      const osmscout::RoutePosition& start,
                      const osmscout::RoutePosition target,
                      osmscout::RouteData& route);

  bool TransformRouteDataToRouteDescription(const osmscout::RoutingProfile& routingProfile,
                                            const osmscout::RouteData& data,
                                            osmscout::RouteDescription& description,
                                            const std::string& start,
                                            const std::string& target);
  bool TransformRouteDataToWay(osmscout::Vehicle vehicle,
                               const osmscout::RouteData& data,
                               osmscout::Way& way);

  osmscout::RoutePosition GetClosestRoutableNode(const osmscout::ObjectFileRef& refObject,
                                                 const osmscout::RoutingProfile& routingProfile,
                                                 double radius);

  void ClearRoute();
  void AddRoute(const osmscout::Way& way);

  static bool InitializeInstance();
  static DBThread* GetInstance();
  static void FreeInstance();
};

#endif
