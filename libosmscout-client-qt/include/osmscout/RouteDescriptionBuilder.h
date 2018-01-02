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

#ifndef LIBOSMSCOUT_ROUTEDESCRIPTIONBUILDER_H
#define LIBOSMSCOUT_ROUTEDESCRIPTIONBUILDER_H

#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/Route.h>
#include <osmscout/Way.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QObject>
#include <QString>
#include <memory>

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
class OSMSCOUT_CLIENT_QT_API RouteDescriptionBuilder : public QObject {
  Q_OBJECT

private:
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

public:
  RouteDescriptionBuilder();
  virtual ~RouteDescriptionBuilder();

  void GenerateRouteSteps(RouteSelection &route);
};

#endif //LIBOSMSCOUT_ROUTEDESCRIPTIONBUILDER_H
