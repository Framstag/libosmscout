#ifndef OSMSCOUT_CLIENT_QT_ROUTER_H
#define OSMSCOUT_CLIENT_QT_ROUTER_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010 Tim Teulings
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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscoutclientqt/RouteDescriptionBuilder.h>
#include <osmscoutmap/DataTileCache.h>
#include <osmscoutclientqt/DBThread.h>
#include <osmscoutclientqt/Settings.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/routing/MultiDBRoutingService.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/DBFileOffset.h>
#include <osmscoutclientqt/QmlRoutingProfile.h>

#include <memory>

#include <QObject>
#include <QSettings>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API Router : public QObject{
  Q_OBJECT

private:
  typedef std::function<void(size_t)> ProgressReporter;

  class QtRoutingProgress : public osmscout::RoutingProgress
  {
  private:
    std::chrono::system_clock::time_point lastDump;
    double                                maxPercent;
    ProgressReporter                      reporter;

  public:
    explicit QtRoutingProgress(ProgressReporter reporter)
    : lastDump(std::chrono::system_clock::now()),
      maxPercent(0.0),
      reporter(std::move(reporter))
    {
      // no code
    }

    void Reset()
    {
      lastDump=std::chrono::system_clock::now();
      maxPercent=0.0;
    }

    void Progress(const Distance &currentMaxDistance,
                  const Distance &overallDistance)
    {
      double currentPercent=(currentMaxDistance.AsMeter()*100.0)/overallDistance.AsMeter();

      std::chrono::system_clock::time_point now=std::chrono::system_clock::now();

      maxPercent=std::max(maxPercent,currentPercent);

      if (std::chrono::duration_cast<std::chrono::milliseconds>(now-lastDump).count()>100) {
        //std::cout << (size_t)maxPercent << "%" << std::endl;
        reporter((size_t)maxPercent);
        lastDump=now;
      }
    }
  };

private:
  QThread     *thread;
  SettingsRef settings;
  DBThreadRef dbThread;

  osmscout::RouterParameter routerParameter;

public slots:
  void Initialize();

  /**
   * Start Route computation. Router emits multiple routingProgress followed by
   * routeComputed or routeFailed with requestId identifier.
   *
   * User of this function should use Qt::QueuedConnection for invoking
   * this slot, search may generate IO load and may took long time.
   *
   * Route computation can be long depending on the complexity of the route
   * (the further away the endpoints, the more difficult the routing).
   *
   * @param start - starting position for route computation
   * @param target - end position for route computation
   * @param vehicle - used vehicle for route
   * @param requestId - id used later in routeComputed/routeFailed signals
   * @param breaker - breaker that may be used for cancel routing computation
   */
  void onRouteRequest(LocationEntryRef start,
                      LocationEntryRef target,
                      QmlRoutingProfileRef profile,
                      int requestId,
                      osmscout::BreakerRef breaker);
signals:
  void routeComputed(QtRouteData route,
                     int requestId);

  void routeFailed(QString reason,
                   int requestId);

  void routeCanceled(int requestId);

  void routingProgress(int percent,
                       int requestId);

private:

  /**
   * Lookup routable node by Qt LocationEntry
   *
   * @param routingService
   * @param location
   * @return possible routable node. When no routable node node is found, nullopt is returned.
   */
  std::optional<RoutePosition> LocationToRoutePosition(osmscout::MultiDBRoutingServiceRef &routingService,
                                                       const LocationEntryRef &location);

  /**
   * Synchronous method for routing. Emits multiple routingProgress followed by one of:
   * routeComputed, routeCanceled or routeFailed.
   *
   * @param routingService
   * @param start
   * @param target
   * @param requestId
   * @param breaker
   */
  void ProcessRouteRequest(osmscout::MultiDBRoutingServiceRef &routingService,
                           const LocationEntryRef &start,
                           const LocationEntryRef &target,
                           int requestId,
                           const osmscout::BreakerRef &breaker);

  bool CalculateRoute(osmscout::MultiDBRoutingServiceRef &routingService,
                      const osmscout::RoutePosition& start,
                      const osmscout::RoutePosition& target,
                      osmscout::RouteData& route,
                      int requestId,
                      const osmscout::BreakerRef &breaker);

  RouteDescriptionResult TransformRouteDataToRouteDescription(osmscout::MultiDBRoutingServiceRef &routingService,
                                                              const osmscout::RouteData& data,
                                                              const std::string& start,
                                                              const std::string& target);

  osmscout::MultiDBRoutingServiceRef MakeRoutingService(const std::list<DBInstanceRef>& databases,
                                                        const QmlRoutingProfileRef &profile);

public:
  Router(QThread *thread,
         SettingsRef settings,
         DBThreadRef dbThread);

  virtual ~Router();

};

}

Q_DECLARE_METATYPE(osmscout::Vehicle)

#endif /* OSMSCOUT_CLIENT_QT_ROUTER_H */
