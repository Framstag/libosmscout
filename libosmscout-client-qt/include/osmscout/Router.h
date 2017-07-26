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

#ifndef ROUTER_H
#define ROUTER_H

#include <memory>

#include <QObject>
#include <QSettings>

#include <osmscout/DataTileCache.h>
#include <osmscout/DBThread.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/routing/MultiDBRoutingService.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/DBFileOffset.h>

#include <osmscout/private/ClientQtImportExport.h>

Q_DECLARE_METATYPE(osmscout::Vehicle)

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API RouteStep : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString distance READ getDistance)
    Q_PROPERTY(QString distanceDelta READ getDistanceDelta)
    Q_PROPERTY(QString time READ getTime)
    Q_PROPERTY(QString timeDelta READ getTimeDelta)
    Q_PROPERTY(QString description READ getDescription)

public:
  QString distance;
  QString distanceDelta;
  QString time;
  QString timeDelta;
  QString description;

public:
  RouteStep();
  RouteStep(const RouteStep& other);

  RouteStep& operator=(const RouteStep& other);

  QString getDistance() const
  {
      return distance;
  }

  QString getDistanceDelta() const
  {
      return distanceDelta;
  }

  QString getTime() const
  {
      return time;
  }

  QString getTimeDelta() const
  {
      return timeDelta;
  }

  QString getDescription() const
  {
      return description;
  }
};

/**
 * \ingroup QtAPI
 */
struct RouteSelection
{
  osmscout::RouteData        routeData;
  osmscout::RouteDescription routeDescription;
  QList<RouteStep>           routeSteps;
  osmscout::Way              routeWay;
};

typedef std::shared_ptr<RouteSelection> RouteSelectionRef;

Q_DECLARE_METATYPE(RouteSelection)
Q_DECLARE_METATYPE(RouteSelectionRef)

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
    QtRoutingProgress(ProgressReporter reporter)
    : lastDump(std::chrono::system_clock::now()),
      maxPercent(0.0),
      reporter(reporter)
    {
      // no code
    }

    void Reset()
    {
      lastDump=std::chrono::system_clock::now();
      maxPercent=0.0;
    }

    void Progress(double currentMaxDistance,
                  double overallDistance)
    {
      double currentPercent=(currentMaxDistance*100.0)/overallDistance;

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
   * Start Route computation. Router emits routeComputed or routeFailed later.
   *
   * User of this function should use Qt::QueuedConnection for invoking
   * this slot, search may generate IO load and may tooks long time.
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
                      osmscout::Vehicle vehicle,
                      int requestId,
                      osmscout::BreakerRef breaker);
signals:
  void routeComputed(RouteSelectionRef route,
                     int requestId);

  void routeFailed(QString reason,
                   int requestId);

  void routeCanceled(int requestId);

  void routingProgress(int percent,
                       int requestId);

private:
  void GetCarSpeedTable(std::map<std::string,double>& map);

  void DumpStartDescription(RouteSelection &route,
                            const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                            const osmscout::RouteDescription::NameDescriptionRef& nameDescription);

  void DumpTargetDescription(RouteSelection &route, 
                             const osmscout::RouteDescription::TargetDescriptionRef& targetDescription);

  void DumpTurnDescription(RouteSelection &route,
                           const osmscout::RouteDescription::TurnDescriptionRef& turnDescription,
                           const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                           const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                           const osmscout::RouteDescription::NameDescriptionRef& nameDescription);

  void DumpRoundaboutEnterDescription(RouteSelection &route,
                                      const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                      const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

  void DumpRoundaboutLeaveDescription(RouteSelection &route,
                                      const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                      const osmscout::RouteDescription::NameDescriptionRef& nameDescription);

  void DumpMotorwayEnterDescription(RouteSelection &route,
                                    const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                    const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

  void DumpMotorwayChangeDescription(RouteSelection &route,
                                     const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription);
  
  void DumpMotorwayLeaveDescription(RouteSelection &route,
                                    const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                    const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                    const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
  
  void DumpNameChangedDescription(RouteSelection &route,
                                  const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription);

  void GenerateRouteSteps(RouteSelection &route);

  void ProcessRouteRequest(osmscout::MultiDBRoutingServiceRef &routingService,
                           const LocationEntryRef &start,
                           const LocationEntryRef &target,
                           osmscout::Vehicle vehicle,
                           int requestId,
                           const osmscout::BreakerRef &breaker);

  bool CalculateRoute(osmscout::MultiDBRoutingServiceRef &routingService,
                      const osmscout::RoutePosition& start,
                      const osmscout::RoutePosition& target,
                      osmscout::RouteData& route,
                      int requestId,
                      const osmscout::BreakerRef &breaker);

  bool TransformRouteDataToRouteDescription(osmscout::MultiDBRoutingServiceRef &routingService,
                                            const osmscout::RouteData& data,
                                            osmscout::RouteDescription& description,
                                            const std::string& start,
                                            const std::string& target);

  osmscout::MultiDBRoutingServiceRef MakeRoutingService(const std::list<DBInstanceRef>& databases,
                                                        const osmscout::Vehicle vehicle);

public:
  Router(QThread *thread,
         SettingsRef settings,
         DBThreadRef dbThread);

  virtual ~Router();

};

#endif /* ROUTER_H */
