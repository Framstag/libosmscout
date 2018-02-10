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

#ifndef OSMSCOUT_CLIENT_QT_ROUTESTEP_H
#define OSMSCOUT_CLIENT_QT_ROUTESTEP_H

#include <osmscout/private/ClientQtImportExport.h>

#include <QObject>

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

signals:
  void update();

public:
  QString type;             //!< Type of route step
  double distance;          //!< Estimate distance [meters] from route start
  double distanceDelta;     //!< Estimate distance [meters] from previous route step
  double distanceTo;        //!< Estimate distance [meters] to this step (used with navigation)
  double time;              //!< Estimate time [seconds] from route start
  double timeDelta;         //!< Estimate time [seconds] from previous route step
  QString description;      //!< Formatted (html) verbose description (translated already)
  QString shortDescription; //!< Plain short description (translated already)

public:
  RouteStep() : RouteStep("") {};
  RouteStep(QString type);
  RouteStep(const RouteStep& other);

  RouteStep& operator=(const RouteStep& other);

  QString getType() const
  {
    return type;
  };

  double getDistance() const
  {
    return distance;
  }

  double getDistanceDelta() const
  {
    return distanceDelta;
  }

  double getDistanceTo() const
  {
    return distanceTo;
  }

  double getTime() const
  {
    return time;
  }

  double getTimeDelta() const
  {
    return timeDelta;
  }

  QString getDescription() const
  {
    return description;
  }

  QString getShortDescription() const
  {
    return shortDescription;
  }

private:
  void copyDynamicProperties(const RouteStep &other);
};


#endif //OSMSCOUT_CLIENT_QT_ROUTESTEP_H
