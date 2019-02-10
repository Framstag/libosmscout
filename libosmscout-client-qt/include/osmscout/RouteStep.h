#ifndef OSMSCOUT_CLIENT_QT_ROUTESTEP_H
#define OSMSCOUT_CLIENT_QT_ROUTESTEP_H

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

#include <osmscout/ClientQtImportExport.h>

#include <osmscout/util/Time.h>
#include <osmscout/util/Distance.h>

#include <QObject>

namespace osmscout {

/**
 * Human representation of route step commands.
 * It contains time, disntace and two variants of translated description:
 *  - simple `shortTranslation`
 *  - formatted `description` with simple html formatting (just subset supported by Qt components)
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API RouteStep : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString type             READ getType             NOTIFY update)
  Q_PROPERTY(double  distance         READ getDistance         NOTIFY update)
  Q_PROPERTY(double  distanceDelta    READ getDistanceDelta    NOTIFY update)
  Q_PROPERTY(double  distanceTo       READ getDistanceTo       NOTIFY update)
  Q_PROPERTY(double  time             READ getTime             NOTIFY update)
  Q_PROPERTY(double  timeDelta        READ getTimeDelta        NOTIFY update)
  Q_PROPERTY(QString description      READ getDescription      NOTIFY update)
  Q_PROPERTY(QString shortDescription READ getShortDescription NOTIFY update)
  Q_PROPERTY(QStringList streetNames  READ getStreetNames()    NOTIFY update)

signals:
  void update();

public:
  QString type;             //!< Type of route step
  Distance distance;        //!< Estimate distance from route start
  Distance distanceDelta;   //!< Estimate distance from previous route step
  Distance distanceTo;      //!< Estimate distance to this step (used with navigation)
  Duration time;            //!< Estimate time from route start
  Duration timeDelta;       //!< Estimate time from previous route step
  QString description;      //!< Formatted (html) verbose description (translated already)
  QString shortDescription; //!< Plain short description (translated already)
  QStringList streetNames;  //!< Street names leading to this step

public:
  inline RouteStep() : RouteStep("", Distance::Zero(), Distance::Zero(),
                                 Duration::zero(), Duration::zero(),
                                 QStringList())
  {};

  RouteStep(const QString &type,
            const Distance &distance,
            const Distance &distanceDelta,
            const Duration &time,
            const Duration &timeDelta,
            const QStringList &streetNames);

  RouteStep(const RouteStep& other);

  RouteStep& operator=(const RouteStep& other);

  QString getType() const
  {
    return type;
  };

  Distance GetDistance() const
  {
    return distance;
  }

  double getDistance() const
  {
    return distance.AsMeter();
  }

  double getDistanceDelta() const
  {
    return distanceDelta.AsMeter();
  }

  double getDistanceTo() const
  {
    return distanceTo.AsMeter();
  }

  double getTime() const
  {
    return DurationAsSeconds(time);
  }

  double getTimeDelta() const
  {
    return DurationAsSeconds(timeDelta);
  }

  QString getDescription() const
  {
    return description;
  }

  QString getShortDescription() const
  {
    return shortDescription;
  }

  QStringList getStreetNames() const
  {
    return streetNames;
  }

private:
  void copyDynamicProperties(const RouteStep &other);
};

}

#endif //OSMSCOUT_CLIENT_QT_ROUTESTEP_H
