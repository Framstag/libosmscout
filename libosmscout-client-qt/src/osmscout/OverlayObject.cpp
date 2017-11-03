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

#include <osmscout/OverlayObject.h>
#include <osmscout/util/Geometry.h>

OverlayObject::OverlayObject(QObject *parent):
  QObject(parent),
  typeName("_route")
{
}

OverlayObject::OverlayObject(const std::vector<osmscout::Point> &nodes,
                       QString typeName,
                       QObject *parent):
  QObject(parent),
  typeName(typeName),
  nodes(nodes)
{
}

OverlayObject::~OverlayObject()
{
}

bool OverlayObject::set(const OverlayObject &other)
{
  if (getObjectType()!=other.getObjectType()){
    return false;
  }

  QMutexLocker locker(&lock);
  QMutexLocker locker2(&other.lock);
  typeName=other.typeName;
  nodes=other.nodes;
  return true;
}

void OverlayObject::clear()
{
  QMutexLocker locker(&lock);
  nodes.clear();
  box.Invalidate();
}

void OverlayObject::addPoint(double lat, double lon)
{
  QMutexLocker locker(&lock);
  nodes.push_back(osmscout::Point(0,osmscout::GeoCoord(lat,lon)));
  box.Invalidate();
}

osmscout::GeoBox OverlayObject::boundingBox()
{
  QMutexLocker locker(&lock);
  if (!box.IsValid() && !nodes.empty()){
    osmscout::GetBoundingBox(nodes,box);
  }
  return box;
}

OverlayWay::OverlayWay(QObject *parent):
  OverlayObject(parent){}

OverlayWay::OverlayWay(const std::vector<osmscout::Point> &nodes,
                       QString typeName,
                       QObject *parent):
  OverlayObject(nodes, typeName, parent){}

OverlayWay::~OverlayWay()
{
}

bool OverlayWay::toWay(osmscout::WayRef &way,
                       const osmscout::TypeConfig &typeConfig) const
{
  QMutexLocker locker(&lock);
  osmscout::TypeInfoRef type=typeConfig.GetTypeInfo(typeName.toStdString());
  if (!type){
    return false;
  }
  way->SetType(type);
  way->SetLayerToMax();
  way->nodes=nodes;
  return true;
}

OverlayArea::OverlayArea(QObject *parent):
  OverlayObject(parent){}

OverlayArea::OverlayArea(const std::vector<osmscout::Point> &nodes,
                       QString typeName,
                       QObject *parent):
  OverlayObject(nodes, typeName, parent){}

OverlayArea::~OverlayArea()
{
}

bool OverlayArea::toArea(osmscout::AreaRef &area,
                         const osmscout::TypeConfig &typeConfig) const
{
  QMutexLocker locker(&lock);
  osmscout::TypeInfoRef type=typeConfig.GetTypeInfo(typeName.toStdString());
  if (!type){
    return false;
  }
  osmscout::Area::Ring outerRing;
  outerRing.SetType(type);
  outerRing.MarkAsOuterRing();
  outerRing.nodes=nodes;

  area->rings.push_back(outerRing);

  return true;
}

OverlayNode::OverlayNode(QObject *parent):
  OverlayObject(parent){}

OverlayNode::OverlayNode(const std::vector<osmscout::Point> &nodes,
                         QString typeName,
                         QObject *parent):
  OverlayObject(nodes, typeName, parent){}

OverlayNode::~OverlayNode()
{
}

bool OverlayNode::toNode(osmscout::NodeRef &node,
                         const osmscout::TypeConfig &typeConfig) const
{
  QMutexLocker locker(&lock);
  osmscout::TypeInfoRef type=typeConfig.GetTypeInfo(typeName.toStdString());
  if (!type){
    return false;
  }
  if (nodes.empty()){
    return false;
  }
  node->SetType(type);
  //node->SetLayerToMax();
  node->SetCoords(nodes.begin()->GetCoord());
  return true;
}
