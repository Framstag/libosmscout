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

#include <osmscout/LocationEntry.h>
#include <osmscout/util/Geometry.h>

#include <iostream>

LocationEntry::LocationEntry(Type type,
                             const QString& label,
                             const QString& objectType,
                             const QStringList& adminRegionList,
                             const QString database,
                             const osmscout::GeoCoord coord,
                             const osmscout::GeoBox bbox,
                             QObject* parent)
    : QObject(parent),
      type(type),
      label(label),
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
      type(typeCoordinate),
      label(label),
      coord(coord)
{
    // no code
}

LocationEntry::LocationEntry(QObject* parent)
    : QObject(parent),
      type(typeNone)
{
    // no code
}

LocationEntry::LocationEntry(const LocationEntry& other)
 : QObject(other.parent()),
   type(other.type),
   label(other.label),
   objectType(other.objectType),
   adminRegionList(other.adminRegionList),
   database(other.database),
   references(other.references),
   coord(other.coord),
   bbox(other.bbox)
{
    // no code
}

LocationEntry::~LocationEntry()
{
    // no code
}

void LocationEntry::operator=(const LocationEntry& other)
{
    type=other.type;
    label=other.label;
    objectType=other.objectType;
    adminRegionList=other.adminRegionList;
    database=other.database;
    references=other.references;
    coord=other.coord;
    bbox=other.bbox;
}

void LocationEntry::addReference(const osmscout::ObjectFileRef reference)
{
    assert(type==typeObject);
    references.push_back(reference);
}

void LocationEntry::mergeWith(const LocationEntry &location)
{
  assert(type==typeObject);
  assert(location.type==typeObject);

  bbox.Include(location.bbox);
  for (auto &ref:location.getReferences()) {
    addReference(ref);
  }
  coord=bbox.GetCenter();
}

Q_INVOKABLE double LocationEntry::distanceTo(double lat, double lon) const
{
  return osmscout::GetSphericalDistance(coord, osmscout::GeoCoord(lat, lon)) * 1000;
}

LocationEntry::Type LocationEntry::getType() const
{
    return type;
}

QString LocationEntry::getTypeString() const
{
  switch (type){
    case typeObject:
      return "object";
    case typeCoordinate:
      return "coordinate";
    default:
      return "none";
  }
}

QString LocationEntry::getObjectType() const
{
    return objectType;
}

QStringList LocationEntry::getAdminRegionList() const
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
