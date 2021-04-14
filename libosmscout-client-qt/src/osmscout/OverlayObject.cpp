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
#include <osmscout/util/Logger.h>
#include <osmscout/TypeFeatures.h>

#include <iostream>

namespace osmscout {

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

OverlayObject::OverlayObject(const OverlayObject &other):
  QObject(nullptr)
{
  QMutexLocker locker(&lock);
  QMutexLocker locker2(&other.lock);

  typeName = other.typeName;
  nodes = other.nodes;
  layer = other.layer;
  name = other.name;
  color = other.color;
  colorValue = other.colorValue;
}

OverlayObject::~OverlayObject()
{
}

void OverlayObject::setColor(const QString &c)
{
  QMutexLocker locker(&lock);
  color = c;
  if (c.isEmpty()) {
    colorValue.reset();
  } else {
    Color cv;
    std::string str(c.toLower().toStdString());
    if (Color::FromHexString(str, cv) ||
        Color::FromW3CKeywordString(str, cv)) {
      colorValue = std::optional<Color>(cv);
    } else {
      colorValue.reset();
      osmscout::log.Warn() << "Not a valid color value: " << c.toStdString();
    }
  }
}

void OverlayObject::clear()
{
  QMutexLocker locker(&lock);
  nodes.clear();
  box.Invalidate();
  emit bboxChanged();
}

void OverlayObject::addPoint(double lat, double lon)
{
  QMutexLocker locker(&lock);
  nodes.push_back(osmscout::Point(0,osmscout::GeoCoord(lat,lon)));
  box.Invalidate();
  segmentsBoxes.clear();
  emit bboxChanged();
}

LocationEntry* OverlayObject::getBBoxAsLocation() const
{
  GeoBox bbox = boundingBox();

  // QML will take ownership
  return new LocationEntry(LocationEntry::Type::typeNone,
                           "bbox",
                           "bbox",
                           QStringList(),
                           "",
                           bbox.GetCenter(),
                           bbox);
}

osmscout::GeoBox OverlayObject::boundingBox() const
{
  QMutexLocker locker(&lock);
  return boundingBoxInternal();
}

std::vector<osmscout::GeoCoord> OverlayObject::getCoords() const
{
  QMutexLocker locker(&lock);
  std::vector<osmscout::GeoCoord> coords;
  coords.reserve(nodes.size());
  for (const auto &point: nodes){
    coords.push_back(point.GetCoord());
  }
  return coords;
}

std::vector<osmscout::Point> OverlayObject::getPoints() const
{
  QMutexLocker locker(&lock);
  return nodes;
}


osmscout::GeoBox OverlayObject::boundingBoxInternal() const
{
  if (!box.IsValid() && !nodes.empty()){
    osmscout::GetBoundingBox(nodes,box);
  }
  return box;
}

std::vector<SegmentGeoBox> OverlayObject::segments() const
{
  if (segmentsBoxes.empty() && !nodes.empty()){
    osmscout::ComputeSegmentBoxes(nodes, segmentsBoxes, nodes.size(), 1000);
  }
  return segmentsBoxes;
}

void OverlayObject::setupFeatures(const osmscout::TypeInfoRef &type,
                                  osmscout::FeatureValueBuffer &features) const
{
  size_t featureIndex;

  features.SetType(type);

  if (type->GetFeature(osmscout::LayerFeature::NAME,
                       featureIndex)) {
    auto*value=dynamic_cast<osmscout::LayerFeatureValue*>(features.AllocateValue(featureIndex));
    assert(value);
    value->SetLayer(layer);
  }

  if (!name.isEmpty() &&
      type->GetFeature(osmscout::NameFeature::NAME,
                       featureIndex)) {
    auto*value=dynamic_cast<osmscout::NameFeatureValue*>(features.AllocateValue(featureIndex));
    assert(value);
    value->SetName(name.toStdString());
  }

  if (colorValue.has_value() &&
      type->GetFeature(osmscout::ColorFeature::NAME,
                       featureIndex)) {
    auto*value=dynamic_cast<osmscout::ColorFeatureValue*>(features.AllocateValue(featureIndex));
    assert(value);
    value->SetColor(colorValue.value());
  }
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
    // see OSMScoutQtBuilder::AddCustomPoiType
    osmscout::log.Warn() << "Type " << typeName.toStdString() << " is not registered for way";
    return false;
  }

  way->SetType(type);

  osmscout::FeatureValueBuffer features;
  setupFeatures(type, features);
  way->SetFeatures(features);

  way->nodes=nodes;
  way->bbox=boundingBoxInternal();
  way->segments=segments();
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
    // see OSMScoutQtBuilder::AddCustomPoiType
    osmscout::log.Warn() << "Type " << typeName.toStdString() << " is not registered for area";
    return false;
  }
  osmscout::Area::Ring outerRing;
  outerRing.SetType(type);
  outerRing.MarkAsOuterRing();
  outerRing.nodes=nodes;
  outerRing.bbox=boundingBoxInternal();
  outerRing.segments=segments();

  osmscout::FeatureValueBuffer features;
  setupFeatures(type, features);
  outerRing.SetFeatures(features);

  area->rings.push_back(std::move(outerRing));

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
    // see OSMScoutQtBuilder::AddCustomPoiType
    osmscout::log.Warn() << "Type " << typeName.toStdString() << " is not registered for node";
    return false;
  }
  if (nodes.empty()){
    return false;
  }

  node->SetType(type);
  //node->SetLayerToMax();
  node->SetCoords(nodes.begin()->GetCoord());

  osmscout::FeatureValueBuffer features;
  setupFeatures(type, features);
  node->SetFeatures(features);

  return true;
}
}
