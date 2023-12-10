/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2010  Tim Teulings
 Copyright (C) 2016  Lukáš Karas

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

#include <osmscoutclientqt/LocationEntry.h>
#include <osmscoutclientqt/QtStdConverters.h>

#include <osmscout/util/Geometry.h>

#include <iostream>

namespace osmscout {

LocationEntry::LocationEntry(LocationInfo::Type type,
                             const QString& label,
                             const QString& altName,
                             const QString& objectType,
                             const QList<AdminRegionInfoRef>& adminRegionList,
                             const QString database,
                             const osmscout::GeoCoord coord,
                             const osmscout::GeoBox bbox,
                             QObject* parent)
    : QObject(parent),
      type(type),
      label(label),
      altName(altName),
      objectType(objectType),
      adminRegionList(adminRegionList),
      database(database),
      coord(coord),
      bbox(bbox)
{
    // no code
}

LocationEntry::LocationEntry(const QString& label,
                             const osmscout::GeoCoord& coord,
                             QObject* parent)
    : QObject(parent),
      type(LocationInfo::Type::typeCoordinate),
      label(label),
      coord(coord)
{
    // no code
}

LocationEntry::LocationEntry(QObject* parent)
    : QObject(parent),
      type(LocationInfo::Type::typeNone)
{
    // no code
}

LocationEntry::LocationEntry(const LocationInfo &info):
  LocationEntry(info.type,
                QString::fromStdString(info.label),
                QString::fromStdString(info.altName),
                QString::fromStdString(info.objectType),
                vectorToQList<>(info.adminRegionList),
                QString::fromStdString(info.database),
                info.coord,
                info.bbox,
                nullptr)
{
  for (const auto &ref: info.references) {
    addReference(ref);
  }
}

LocationEntry::LocationEntry(const LocationEntry& other)
 : QObject(other.parent()), // make copy of Qt ownership
   type(other.type),
   label(other.label),
   altName(other.altName),
   objectType(other.objectType),
   adminRegionList(other.adminRegionList),
   database(other.database),
   references(other.references),
   coord(other.coord),
   bbox(other.bbox)
{
    // no code
}

LocationEntry& LocationEntry::operator=(const LocationEntry& other)
{
    // Qt ownership is unchanged
    type=other.type;
    label=other.label;
    altName=other.altName;
    objectType=other.objectType;
    adminRegionList=other.adminRegionList;
    database=other.database;
    references=other.references;
    coord=other.coord;
    bbox=other.bbox;
    return *this;
}

LocationEntry::LocationEntry(LocationEntry&& other)
 : QObject(other.parent()), // make copy of Qt ownership
   type(std::move(other.type)),
   label(std::move(other.label)),
   altName(std::move(other.altName)),
   objectType(std::move(other.objectType)),
   adminRegionList(std::move(other.adminRegionList)),
   database(std::move(other.database)),
   references(std::move(other.references)),
   coord(std::move(other.coord)),
   bbox(std::move(other.bbox))
{
  // no code
}

LocationEntry& LocationEntry::operator=(LocationEntry&& other) {
    setParent(other.parent()); // make copy of Qt ownership
    type=std::move(other.type);
    label=std::move(other.label);
    altName=std::move(other.altName);
    objectType=std::move(other.objectType);
    adminRegionList=std::move(other.adminRegionList);
    database=std::move(other.database);
    references=std::move(other.references);
    coord=std::move(other.coord);
    bbox=std::move(other.bbox);
    return *this;
}

void LocationEntry::addReference(const osmscout::ObjectFileRef reference)
{
    assert(type==LocationInfo::Type::typeObject);
    references.push_back(reference);
}

void LocationEntry::mergeWith(const LocationEntry &location)
{
  assert(type==LocationInfo::Type::typeObject);
  assert(location.type==LocationInfo::Type::typeObject);

  bbox.Include(location.bbox);
  for (auto &ref:location.getReferences()) {
    addReference(ref);
  }
  if (adminRegionList.empty() && !location.adminRegionList.empty()){
    adminRegionList=location.adminRegionList;
  }
  coord=bbox.GetCenter();
}

Q_INVOKABLE double LocationEntry::distanceTo(double lat, double lon) const
{
  return osmscout::GetSphericalDistance(coord, osmscout::GeoCoord(lat, lon)).AsMeter();
}

LocationInfo::Type LocationEntry::getType() const
{
    return type;
}

QString LocationEntry::getTypeString() const
{
  switch (type){
    case LocationInfo::Type::typeObject:
      return "object";
    case LocationInfo::Type::typeCoordinate:
      return "coordinate";
    default:
      return "none";
  }
}

QString LocationEntry::getObjectType() const
{
    return objectType;
}

QList<AdminRegionInfoRef> LocationEntry::getAdminRegionList() const
{
    return adminRegionList;
}

QString LocationEntry::getDatabase() const
{
    return database;
}

QString LocationEntry::getLabel() const
{
    return label;
}

QString LocationEntry::getAltName() const
{
  return altName;
}

osmscout::GeoCoord LocationEntry::getCoord() const
{
    return coord;
}

osmscout::GeoBox LocationEntry::getBBox() const
{
    return bbox;
}

const QList<osmscout::ObjectFileRef>& LocationEntry::getReferences() const
{
    return references;
}

double LocationEntry::getLat() const
{
  return coord.GetLat();
}

double LocationEntry::getLon() const
{
  return coord.GetLon();
}
}
