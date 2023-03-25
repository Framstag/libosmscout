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

#include <osmscoutclientqt/RouteStep.h>

#include <QMetaType>
#include <QVariant>
#include <QAbstractListModel>

namespace osmscout {

RouteStep::RouteStep(const QString &type,
                     const GeoCoord &coord,
                     const Distance &distance,
                     const Distance &distanceDelta,
                     const Duration &timestamp,
                     const Duration &timestampDelta,
                     const QStringList &streetNames):
    type(type),
    coord(coord),
    distance(distance),
    distanceDelta(distanceDelta),
    distanceTo(Distance::Zero()),
    timestamp(timestamp),
    timestampDelta(timestampDelta),
    streetNames(streetNames)
{

}

RouteStep::RouteStep(const RouteStep& other)
    : QObject(other.parent()),
      type(other.type),
      coord(other.coord),
      distance(other.distance),
      distanceDelta(other.distanceDelta),
      distanceTo(other.distanceTo),
      timestamp(other.timestamp),
      timestampDelta(other.timestampDelta),
      description(other.description),
      shortDescription(other.shortDescription),
      streetNames(other.streetNames),
      destinations(other.destinations),
      roundaboutExit(other.roundaboutExit),
      roundaboutClockwise(other.roundaboutClockwise)
{
  copyDynamicProperties(other);
}

QVariant RouteStep::data(int role) const
{
  switch (role) {
    case Qt::DisplayRole:
    case ShortDescriptionRole:
      return getShortDescription();
    case DescriptionRole:
      return getDescription();
    case TypeRole:
      return getType();
    case RoundaboutExitRole:
      return getRoundaboutExit();
    case RoundaboutClockwiseRole:
      return getRoundaboutClockwise();
    case latRole:
      return getLat();
    case lonRole:
      return getLon();
    case distanceRole:
      return getDistance();
    case distanceDeltaRole:
      return getDistanceDelta();
    case distanceToRole:
      return getDistanceTo();
    case timeRole:
      return getTime();
    case timeDeltaRole:
      return getTimeDelta();
    case streetNamesRole:
      return streetNames;
    case destinationsRole:
      return destinations;
    default:
      break;
  }

  return QVariant();
}

QHash<int, QByteArray> RouteStep::roleNames(QHash<int, QByteArray> roles)
{
  roles[ShortDescriptionRole] = "shortDescription";
  roles[DescriptionRole] = "description";
  roles[TypeRole] = "type";
  roles[RoundaboutExitRole] = "roundaboutExit";
  roles[RoundaboutClockwiseRole] = "roundaboutClockwise";
  roles[latRole] = "lat";
  roles[lonRole] = "lon";
  roles[distanceRole] = "distance";
  roles[distanceDeltaRole] = "distanceDelta";
  roles[distanceToRole] = "distanceTo";
  roles[timeRole] = "time";
  roles[timeDeltaRole] = "timeDelta";
  roles[streetNamesRole] = "streetNames";
  roles[destinationsRole] = "destinations";

  return roles;
}

void RouteStep::copyDynamicProperties(const RouteStep &other) {
  for (auto const &propertyName:other.dynamicPropertyNames()){
    setProperty(propertyName,other.property(propertyName));
  }
}

RouteStep& RouteStep::operator=(const RouteStep& other)
{
  if (this!=&other) {
    setParent(other.parent());
    type=other.type;
    coord=other.coord;
    distance=other.distance;
    distanceDelta=other.distanceDelta;
    distanceTo=other.distanceTo;
    timestamp=other.timestamp;
    timestampDelta=other.timestampDelta;
    description=other.description;
    shortDescription=other.shortDescription;
    streetNames=other.streetNames;
    destinations=other.destinations;
    roundaboutExit=other.roundaboutExit;
    roundaboutClockwise=other.roundaboutClockwise;
    for (auto const &propertyName:dynamicPropertyNames()){
      setProperty(propertyName, QVariant());
    }
    copyDynamicProperties(other);
  }

  return *this;
}

}
