/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map

 Copyright (C) 2017 Lukáš Karas

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

#include <osmscout/OverlayWay.h>
#include <osmscout/util/Geometry.h>

OverlayWay::OverlayWay(QObject *parent):
  QObject(parent),
  typeName("_route")
{
}

OverlayWay::OverlayWay(const std::vector<osmscout::Point> &nodes,
                       QString typeName,
                       QObject *parent):
  QObject(parent),
  typeName(typeName),
  nodes(nodes)
{
}

OverlayWay::OverlayWay(const OverlayWay &other):
  typeName(other.typeName),
  nodes(other.nodes)
{
}

OverlayWay::~OverlayWay()
{
}

bool OverlayWay::toWay(osmscout::Way &way,
                       const osmscout::TypeConfig typeConfig) const
{
  osmscout::TypeInfoRef type=typeConfig.GetTypeInfo(typeName.toStdString());
  if (!type){
    return false;
  }
  way.SetType(type);
  way.nodes=nodes;
  return true;
}

void OverlayWay::clear()
{
  nodes.clear();
  box.Invalidate();
}

void OverlayWay::addPoint(double lat, double lon)
{
  nodes.push_back(osmscout::Point(0,osmscout::GeoCoord(lat,lon)));
  box.Invalidate();
}

osmscout::GeoBox OverlayWay::boundingBox()
{
  if (!box.IsValid() && !nodes.empty()){
    osmscout::GetBoundingBox(nodes,box);
  }
  return box;
}