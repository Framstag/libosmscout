/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010 Tim Teulings
 Copyright (C) 2018 Lukas Karas

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

#include <osmscout/QtRouteData.h>

QtRouteData::QtRouteData(const QtRouteData &other,
                         QObject* parent):
  QObject(parent)
{
  data=other.data;
}

QtRouteData::QtRouteData(osmscout::RouteDescription &&routeDescription,
                         QList<RouteStep> &&routeSteps,
                         osmscout::Way &&routeWay,
                         QObject* parent):
  QObject(parent)
{
  data=std::make_shared<PrivateData>();
  data->routeDescription=std::move(routeDescription);
  data->routeSteps=std::move(routeSteps);
  data->routeWay=std::move(routeWay);
}

QtRouteData& QtRouteData::operator=(const QtRouteData& other)
{
  this->data=other.data;
  return *this;
}

void QtRouteData::clear()
{
  data.reset();
}

osmscout::Way QtRouteData::routeWayCopy() const
{
  if (!data){
    return osmscout::Way();
  }
  return data->routeWay;
}

const osmscout::Way& QtRouteData::routeWay() const
{
  return data->routeWay;
}

const QList<RouteStep>& QtRouteData::routeSteps() const
{
  return data->routeSteps;
}

const osmscout::RouteDescription& QtRouteData::routeDescription() const
{
  return data->routeDescription;
}
