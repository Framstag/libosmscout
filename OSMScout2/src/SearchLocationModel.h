#ifndef SEARCHLOCATIONMODEL_H
#define SEARCHLOCATIONMODEL_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2014  Tim Teulings

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <QObject>
#include <QAbstractListModel>

#include <osmscout/GeoCoord.h>
#include <osmscout/Location.h>

class Location : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)
public:
    enum Type {
        typeNone,
        typeObject,
        typeCoordinate
    };

private:
    Type                           type;
    QString                        name;
    QString                        label;
    QList<osmscout::ObjectFileRef> references;
    osmscout::GeoCoord             coord;

public:
    Location(Type type,
             const QString& name,
             const QString& label,
             QObject* parent = 0);

    Location(const QString& name,
             const QString& label,
             const osmscout::GeoCoord& coord,
             QObject* parent = 0);

    Location(QObject* parent = 0);
    Location(const Location& other);
    virtual ~Location();

    void addReference(const osmscout::ObjectFileRef reference);

    Type getType() const;
    QString getName() const;
    QString getLabel() const;
    osmscout::GeoCoord getCoord() const;
    const QList<osmscout::ObjectFileRef>& getReferences() const;
};

class LocationListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount)

public slots:
    void setPattern(const QString& pattern);

private:
    QList<Location*> locations;

public:
    enum Roles {
        LabelRole = Qt::UserRole,
        TextRole = Qt::UserRole+1
    };

public:
    LocationListModel(QObject* parent = 0);
    ~LocationListModel();

    QVariant data(const QModelIndex &index, int role) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE Location* get(int row) const;
};

#endif
