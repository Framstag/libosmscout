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

#ifndef OSMSCOUT_CLIENT_QT_ROUTEDESCRIPTIONBUILDER_H
#define OSMSCOUT_CLIENT_QT_ROUTEDESCRIPTIONBUILDER_H

#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/Route.h>
#include <osmscout/Way.h>

#include <osmscout/RouteStep.h>
#include <osmscout/QtRouteData.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QObject>
#include <QString>
#include <memory>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API RouteDescriptionBuilder : public QObject {
  Q_OBJECT

private:
  void DumpStartDescription(QList<RouteStep> &routeSteps,
                            const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                            const osmscout::RouteDescription::NameDescriptionRef& nameDescription);

  void DumpTargetDescription(QList<RouteStep> &routeSteps,
                             const osmscout::RouteDescription::TargetDescriptionRef& targetDescription);

  void DumpTurnDescription(QList<RouteStep> &routeSteps,
                           const osmscout::RouteDescription::TurnDescriptionRef& turnDescription,
                           const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                           const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                           const osmscout::RouteDescription::NameDescriptionRef& nameDescription);

  void DumpRoundaboutEnterDescription(QList<RouteStep> &routeSteps,
                                      const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                      const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

  void DumpRoundaboutLeaveDescription(QList<RouteStep> &routeSteps,
                                      const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                      const osmscout::RouteDescription::NameDescriptionRef& nameDescription);

  void DumpMotorwayEnterDescription(QList<RouteStep> &routeSteps,
                                    const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                    const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);

  void DumpMotorwayChangeDescription(QList<RouteStep> &routeSteps,
                                     const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription);

  void DumpMotorwayLeaveDescription(QList<RouteStep> &routeSteps,
                                    const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                    const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                    const osmscout::RouteDescription::NameDescriptionRef& nameDescription);

  void DumpNameChangedDescription(QList<RouteStep> &routeSteps,
                                  const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription);

public:
  RouteDescriptionBuilder();
  virtual ~RouteDescriptionBuilder();

  bool GenerateRouteStep(const osmscout::RouteDescription::Node &node,
                         QList<RouteStep> &routeSteps,
                         size_t &roundaboutCrossingCounter);

  void GenerateRouteSteps(const osmscout::RouteDescription &routeDescription,
                          QList<RouteStep> &routeSteps);
};

#endif //OSMSCOUT_CLIENT_QT_ROUTEDESCRIPTIONBUILDER_H
