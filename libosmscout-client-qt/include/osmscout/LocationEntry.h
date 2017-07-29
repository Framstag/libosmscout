#ifndef OSMSCOUT_CLIENT_QT_LOCATIONENTRY_H
#define OSMSCOUT_CLIENT_QT_LOCATIONENTRY_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2014  Tim Teulings
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

#include <QObject>
#include <QStringList>

#include <osmscout/GeoCoord.h>
#include <osmscout/Location.h>
#include <osmscout/LocationService.h>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 * 
 * Qt abstraction for various objects on map, 
 * used for search and routing
 */
class OSMSCOUT_CLIENT_QT_API LocationEntry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString label READ getLabel)
public:
    enum Type {
        typeNone,
        typeObject,
        typeCoordinate
    };

private:
    Type                           type;
    QString                        label;
    QString                        objectType;
    QStringList                    adminRegionList;
    QString                        database;
    QList<osmscout::ObjectFileRef> references;
    osmscout::GeoCoord             coord;
    osmscout::GeoBox               bbox;

public:
    LocationEntry(Type type,
                  const QString& label,
                  const QString& objectType,
                  const QStringList& adminRegionList,
                  const QString database,
                  const osmscout::GeoCoord coord,
                  const osmscout::GeoBox bbox,
                  QObject* parent = 0);

    LocationEntry(const QString& label,
                  const osmscout::GeoCoord& coord,
                  QObject* parent = 0);

    LocationEntry(QObject* parent = 0);
    LocationEntry(const LocationEntry& other);
    virtual ~LocationEntry();

    void operator=(const LocationEntry&);
    
    void addReference(const osmscout::ObjectFileRef reference);

    Type getType() const;
    QString getObjectType() const;
    QString getLabel() const;
    QStringList getAdminRegionList() const;
    QString getDatabase() const;
    osmscout::GeoCoord getCoord() const;
    osmscout::GeoBox getBBox() const;
    const QList<osmscout::ObjectFileRef>& getReferences() const;
};

typedef std::shared_ptr<LocationEntry> LocationEntryRef;
Q_DECLARE_METATYPE(LocationEntryRef)

#endif
