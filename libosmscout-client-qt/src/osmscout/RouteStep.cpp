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

#include <osmscout/RouteStep.h>

#include <QMetaType>
#include <QVariant>

RouteStep::RouteStep(QString type):
    type(type),
    distance(-1),
    distanceDelta(-1),
    distanceTo(-1),
    time(-1),
    timeDelta(-1)
{

}

RouteStep::RouteStep(const RouteStep& other)
    : QObject(other.parent()),
      type(other.type),
      distance(other.distance),
      distanceDelta(other.distanceDelta),
      distanceTo(other.distanceTo),
      time(other.time),
      timeDelta(other.timeDelta),
      description(other.description),
      shortDescription(other.shortDescription)
{
  copyDynamicProperties(other);
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
    distance=other.distance;
    distanceDelta=other.distanceDelta;
    distanceTo=other.distanceTo;
    time=other.time;
    timeDelta=other.timeDelta;
    description=other.description;
    shortDescription=other.shortDescription;
    for (auto const &propertyName:dynamicPropertyNames()){
      setProperty(propertyName, QVariant());
    }
    copyDynamicProperties(other);
  }

  return *this;
}

