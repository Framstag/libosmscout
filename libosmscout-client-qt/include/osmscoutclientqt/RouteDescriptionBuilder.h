#ifndef OSMSCOUT_CLIENT_QT_ROUTEDESCRIPTIONBUILDER_H
#define OSMSCOUT_CLIENT_QT_ROUTEDESCRIPTIONBUILDER_H

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

#include <osmscout/routing/RouteData.h>
#include <osmscout/routing/RouteDescription.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>
#include <osmscout/Way.h>

#include <osmscoutclientqt/RouteStep.h>
#include <osmscoutclientqt/QtRouteData.h>

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <QObject>
#include <QString>
#include <memory>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API RouteDescriptionBuilder : public QObject {
  Q_OBJECT

public:
  class Callback: public RouteDescriptionPostprocessor::Callback
  {
  private:
    QList<RouteStep> &routeSteps; //!< route step output container, not owning reference
    Distance stopAfter;           //!< stop processing when distance of last step from the start is greater
                                  //!< stopAfter < 0 : unlimited

    bool skipInformative;         //!< Skip informative instructions like "Street change its name"
                                  //!< that are useless during navigation

    QStringList streetNames;       //!< Stack of route names to next steps

    GeoCoord coord;
    Distance distance;
    Distance distancePrevious;
    Duration time;
    Duration timePrevious;

  public:
    Callback(QList<RouteStep> &routeSteps,
             const Distance &stopAfter = Distance::Lowest(),
             bool skipInformative=false);

    ~Callback() override;

    void OnStart(const RouteDescription::StartDescriptionRef& startDescription,
                 const RouteDescription::TypeNameDescriptionRef& typeNameDescription,
                 const RouteDescription::NameDescriptionRef& nameDescription) override;

    void OnTargetReached(const RouteDescription::TargetDescriptionRef& targetDescription) override;

    void OnTurn(const RouteDescription::TurnDescriptionRef& turnDescription,
                const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                const RouteDescription::DirectionDescriptionRef& directionDescription,
                const RouteDescription::TypeNameDescriptionRef& typeNameDescription,
                const RouteDescription::NameDescriptionRef& nameDescription) override;

    void OnRoundaboutEnter(const RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                           const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription) override;

    void OnRoundaboutLeave(const RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                           const RouteDescription::NameDescriptionRef& nameDescription) override;

    void OnMotorwayEnter(const RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                         const RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription) override;

    void OnMotorwayChange(const RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription,
                          const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                          const RouteDescription::DestinationDescriptionRef& crossingDestinationDescription) override;

    void OnMotorwayLeave(const RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                         const RouteDescription::MotorwayJunctionDescriptionRef& motorwayJunctionDescription,
                         const RouteDescription::DirectionDescriptionRef& directionDescription,
                         const RouteDescription::NameDescriptionRef& nameDescription) override;

    void OnPathNameChange(const RouteDescription::NameChangedDescriptionRef& nameChangedDescription) override;

    void PushStreetName(const RouteDescription::NameDescriptionRef& nameDescription);

    void BeforeNode(const RouteDescription::Node& node) override;

    bool Continue() const override;

    RouteStep MkStep(const QString &name);
  };
public:
  RouteDescriptionBuilder();

  ~RouteDescriptionBuilder() override;

  void GenerateRouteSteps(const osmscout::RouteDescription &routeDescription,
                          QList<RouteStep> &routeSteps) const;

  std::list<RouteStep> GenerateRouteInstructions(const RouteDescription::NodeIterator &first,
                                                 const RouteDescription::NodeIterator &last) const;

  RouteStep GenerateNextRouteInstruction(const RouteDescription::NodeIterator &previous,
                                         const RouteDescription::NodeIterator &last,
                                         const GeoCoord &coord) const;

};

}

#endif //OSMSCOUT_CLIENT_QT_ROUTEDESCRIPTIONBUILDER_H
