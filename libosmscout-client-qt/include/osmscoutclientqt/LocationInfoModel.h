#ifndef OSMSCOUT_CLIENT_QT_LOCATIONINFOMODEL_H
#define OSMSCOUT_CLIENT_QT_LOCATIONINFOMODEL_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2016  Lukas Karas

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
#include <QAbstractListModel>

#include <osmscout/GeoCoord.h>
#include <osmscout/util/GeoBox.h>

#include <osmscoutclientqt/LookupModule.h>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
struct ObjectKey{
  QString                  database;
  osmscout::ObjectFileRef  ref;
};

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API LocationInfoModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChange)

signals:
    void locationDescriptionRequested(const osmscout::GeoCoord location);
    void readyChange(bool ready);
    void regionLookupRequested(osmscout::GeoCoord);

public slots:
    void setLocation(const double lat, const double lon);
    void dbInitialized(const osmscout::GeoBox&);
    void onLocationDescription(const osmscout::GeoCoord location, 
                               const QString database, 
                               const osmscout::LocationDescription description,
                               const QList<AdminRegionInfoRef> regions);
    void onLocationDescriptionFinished(const osmscout::GeoCoord);

    void onLocationAdminRegions(const osmscout::GeoCoord,QList<AdminRegionInfoRef>);
    void onLocationAdminRegionFinished(const osmscout::GeoCoord);

public:
    enum Roles {
        LabelRole = Qt::UserRole,
        RegionRole = Qt::UserRole+1,
        AddressRole = Qt::UserRole+2,
        InPlaceRole = Qt::UserRole+3,
        DistanceRole = Qt::UserRole+4,
        BearingRole = Qt::UserRole+5,
        PoiRole = Qt::UserRole+6,
        TypeRole = Qt::UserRole+7,
        PostalCodeRole = Qt::UserRole+8,
        WebsiteRole = Qt::UserRole+9,
        PhoneRole = Qt::UserRole+10,
        AddressLocationRole = Qt::UserRole+11,
        AddressNumberRole = Qt::UserRole+12,
        IndexedAdminRegionRole = Qt::UserRole+13,
        AltLangName = Qt::UserRole+14,
        OpeningHours = Qt::UserRole+15
    };
    Q_ENUM(Roles)

public:
    LocationInfoModel();
    ~LocationInfoModel() override;

    Q_INVOKABLE int inline rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return model.size();
    };
    
    Q_INVOKABLE QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE Qt::ItemFlags flags(const QModelIndex &index) const override;
    
    bool inline isReady() const 
    {
        return ready;
    };
    
    Q_INVOKABLE double distance(double lat1, double lon1, 
                                double lat2, double lon2);
    Q_INVOKABLE QString bearing(double lat1, double lon1, 
                                double lat2, double lon2);
    
    static bool distanceComparator(const QMap<int, QVariant> &obj1,
                                   const QMap<int, QVariant> &obj2);

    static bool adminRegionComparator(const AdminRegionInfoRef& reg1,
                                      const AdminRegionInfoRef& reg2);
private: 
   void addToModel(const QString database,
                   const osmscout::LocationAtPlaceDescriptionRef description,
                   const QList<AdminRegionInfoRef> regions);
 
private:
    bool ready;
    bool setup;
    osmscout::GeoCoord location;

    QList<ObjectKey> objectSet; // set of objects already inserted to model
    QList<QMap<int, QVariant>> model;
    LookupModule* lookupModule;
    SettingsRef settings;
};

}

Q_DECLARE_METATYPE(osmscout::ObjectKey)

#endif	/* OSMSCOUT_CLIENT_QT_LOCATIONINFOMODEL_H */
