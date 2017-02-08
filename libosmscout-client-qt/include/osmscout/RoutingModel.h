#ifndef OSMSCOUT_CLIENT_QT_ROUTINGMODEL_H
#define OSMSCOUT_CLIENT_QT_ROUTINGMODEL_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2014  Tim Teulings

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

#include <map>

#include <QObject>
#include <QAbstractListModel>

#include <osmscout/Location.h>
#include <osmscout/Route.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <osmscout/SearchLocationModel.h>
#include <osmscout/DBThread.h>

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
class OSMSCOUT_CLIENT_QT_API RoutingListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount)

public slots:
    void setStartAndTarget(LocationEntry* start,
                           LocationEntry* target);
    void clear();

private:
    struct RouteSelection
    {
      osmscout::RouteData        routeData;
      osmscout::RouteDescription routeDescription;
      QList<RouteStep>           routeSteps;
    };

    RouteSelection route;

public:
    enum Roles {
        LabelRole = Qt::UserRole
    };

private:
    void GetCarSpeedTable(std::map<std::string,double>& map);

    void DumpStartDescription(const osmscout::RouteDescription::StartDescriptionRef& startDescription,
                              const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
    void DumpTargetDescription(const osmscout::RouteDescription::TargetDescriptionRef& targetDescription);
    void DumpTurnDescription(const osmscout::RouteDescription::TurnDescriptionRef& turnDescription,
                             const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription,
                             const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                             const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
    void DumpRoundaboutEnterDescription(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef& roundaboutEnterDescription,
                                        const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);
    void DumpRoundaboutLeaveDescription(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef& roundaboutLeaveDescription,
                                        const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
    void DumpMotorwayEnterDescription(const osmscout::RouteDescription::MotorwayEnterDescriptionRef& motorwayEnterDescription,
                                      const osmscout::RouteDescription::CrossingWaysDescriptionRef& crossingWaysDescription);
    void DumpMotorwayChangeDescription(const osmscout::RouteDescription::MotorwayChangeDescriptionRef& motorwayChangeDescription);
    void DumpMotorwayLeaveDescription(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef& motorwayLeaveDescription,
                                      const osmscout::RouteDescription::DirectionDescriptionRef& directionDescription,
                                      const osmscout::RouteDescription::NameDescriptionRef& nameDescription);
    void DumpNameChangedDescription(const osmscout::RouteDescription::NameChangedDescriptionRef& nameChangedDescription);

public:
    RoutingListModel(QObject* parent = 0);
    virtual ~RoutingListModel();

    QVariant data(const QModelIndex &index, int role) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE RouteStep* get(int row) const;
};

#endif
