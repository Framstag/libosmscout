/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2013  Tim Teulings

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

#include <QScreen>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QObject>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <osmscout/Settings.h>

Settings::Settings(): 
    view(NULL)
{
    /* Warning: Sailfish OS before version 2.0.1 reports incorrect DPI (100)
     *
     * Some DPI values:
     *
     * ~ 330 - Jolla tablet native
     *   242.236 - Jolla phone native
     *   130 - PC (24" FullHD)
     *   100 - Qt default (reported by SailfishOS < 2.0.1)
     */    
    QScreen *srn=QGuiApplication::screens().at(0);
    physicalDpi = (double)srn->physicalDotsPerInch();
}

Settings::~Settings()
{
    settings.sync();
    // QObject will delete view (child)
}

double Settings::GetPhysicalDPI() const
{
    return physicalDpi;
}

void Settings::SetMapDPI(double dpi)
{
    settings.setValue("settings/map/dpi", (unsigned int)dpi);
    emit MapDPIChange(dpi);
}

double Settings::GetMapDPI() const
{
  return (size_t)settings.value("settings/map/dpi",physicalDpi).toDouble();
}

MapView *Settings::GetMapView()
{
  if (view == NULL){
    double lat   = settings.value("settings/map/lat",   0).toDouble();
    double lon   = settings.value("settings/map/lon",   0).toDouble();
    double angle = settings.value("settings/map/angle", 0).toDouble();
    double mag   = settings.value("settings/map/mag",   osmscout::Magnification::magContinent).toDouble();
    view = new MapView(this, 
              osmscout::GeoCoord(lat, lon),
              angle,
              osmscout::Magnification(mag)
              );
  }
  return view;
}

void Settings::SetMapView(MapView *updated)
{
  bool changed = false;
  if (view == NULL){
    view = new MapView(this, 
              osmscout::GeoCoord(updated->GetLat(), updated->GetLon()),
              updated->GetAngle(),
              osmscout::Magnification(updated->GetMag())
              );
    changed = true;
  }else{
    changed = *view != *updated;  
    if (changed){
        view->operator =( *updated );
    }
  }
  if (changed){
    settings.setValue("settings/map/lat", view->GetLat());
    settings.setValue("settings/map/lon", view->GetLon());
    settings.setValue("settings/map/angle", view->GetAngle());
    settings.setValue("settings/map/mag", view->GetMag());
    emit MapViewChanged(view);
  }
}

osmscout::Vehicle Settings::GetRoutingVehicle() const
{
  return (osmscout::Vehicle)settings.value("routing/vehicle",osmscout::vehicleCar).toUInt();
}

void Settings::SetRoutingVehicle(const osmscout::Vehicle& vehicle)
{
  settings.setValue("routing/vehicle", (unsigned int)vehicle);
}

bool Settings::GetOnlineTilesEnabled() const
{
  return settings.value("onlineTiles", true).toBool();
}

void Settings::SetOnlineTilesEnabled(bool b)
{
  if (GetOnlineTilesEnabled() != b){
    settings.setValue("onlineTiles", b);
    emit OnlineTilesEnabledChanged(b);
  }
}

const QList<OnlineTileProvider> Settings::GetOnlineProviders() const
{
    return onlineProviders;
}

const QList<MapProvider> Settings::GetMapProviders() const
{
    return mapProviders;
}

const OnlineTileProvider Settings::GetOnlineTileProvider() const
{
    if (onlineProviderMap.contains(GetOnlineTileProviderId())){
        return onlineProviderMap[GetOnlineTileProviderId()];
    }
    return OnlineTileProvider();
}

const QString Settings::GetOnlineTileProviderId() const
{
    QString def = "?";
    if (!onlineProviders.isEmpty()){
        def = onlineProviders.begin()->getId();
    }
    return settings.value("onlineTileProvider", def).toString();
}

void Settings::SetOnlineTileProviderId(QString id){
    if (GetOnlineTileProviderId() != id){
        settings.setValue("onlineTileProvider", id);
        emit OnlineTileProviderIdChanged(id);
    }
}

bool Settings::loadOnlineTileProviders(QString path)
{
    // load online tile providers
    QFile loadFile(path);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open" << loadFile.fileName() << "file.";
        return false;
    }
    qDebug() << "Loading online tile providers from " << loadFile.fileName();
    
    QJsonDocument doc = QJsonDocument::fromJson(loadFile.readAll());
    for (auto obj: doc.array()){
        OnlineTileProvider provider = OnlineTileProvider::fromJson(obj);
        if (!provider.isValid()){
            qWarning() << "Can't parse online provider from json value" << obj;
        }else{
            if (!onlineProviderMap.contains(provider.getId())){
              onlineProviderMap[provider.getId()] = provider;
              onlineProviders << provider;
            }
        }
    }
    
    // check if current provider is valid...
    if (!onlineProviderMap.contains(GetOnlineTileProviderId())){
        // ...if not, setup first
        if (!onlineProviders.isEmpty()){
            SetOnlineTileProviderId(onlineProviders.begin()->getId());
        }
    }    
    
    emit OnlineTileProviderIdChanged(GetOnlineTileProviderId());
    return true;
}

bool Settings::loadMapProviders(QString path)
{
    QFile loadFile(path);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open" << loadFile.fileName() << "file.";
        return false;
    }
    qDebug() << "Loading map providers from " << loadFile.fileName();
    
    QJsonDocument doc = QJsonDocument::fromJson(loadFile.readAll());
    for (auto obj: doc.array()){
        MapProvider provider = MapProvider::fromJson(obj);
        if (!provider.isValid()){
            qWarning() << "Can't parse online provider from json value" << obj;
        }else{    
            mapProviders.append(provider);
        }
    }
    return true;
}

bool Settings::GetOfflineMap() const
{
    return settings.value("offlineMap", true).toBool();
}
void Settings::SetOfflineMap(bool b)
{
  if (GetOfflineMap() != b){
    settings.setValue("offlineMap", b);
    emit OfflineMapChanged(b);
  }
}

bool Settings::GetRenderSea() const
{
  return settings.value("renderSea", true).toBool();
}
void Settings::SetRenderSea(bool b)
{
  if (GetRenderSea() != b){
    settings.setValue("renderSea", b);
    emit RenderSeaChanged(b);
  }    
}

const QString Settings::GetGpsFormat() const
{
  return settings.value("gpsFormat", "degrees").toString();
}
void Settings::SetGpsFormat(const QString formatId)
{
  if (GetGpsFormat() != formatId){
    settings.setValue("gpsFormat", formatId);
    emit GpsFormatChanged(formatId);
  }
}

const QString Settings::GetHttpCacheDir() const
{
  QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);  
  return cacheLocation + QDir::separator() + "OSMScoutHttpCache";
}

static Settings* settingsInstance=NULL;

Settings* Settings::GetInstance()
{
    if (settingsInstance == NULL){
        settingsInstance = new Settings();
    }
    return settingsInstance;
}

void Settings::FreeInstance()
{
    if (settingsInstance != NULL){
        delete settingsInstance;
        settingsInstance = NULL;
    }
}

QmlSettings::QmlSettings()
{
    connect(Settings::GetInstance(), SIGNAL(MapDPIChange(double)), 
            this, SIGNAL(MapDPIChange(double)));
    connect(Settings::GetInstance(), SIGNAL(MapViewChanged(MapView *)),
            this, SIGNAL(MapViewChanged(MapView *)));
    connect(Settings::GetInstance(), SIGNAL(OnlineTilesEnabledChanged(bool)),
            this, SIGNAL(OnlineTilesEnabledChanged(bool)));
    connect(Settings::GetInstance(), SIGNAL(OnlineTileProviderIdChanged(const QString)),
            this, SIGNAL(OnlineTileProviderIdChanged(const QString)));
    connect(Settings::GetInstance(), SIGNAL(OfflineMapChanged(bool)),
            this, SIGNAL(OfflineMapChanged(bool)));
    connect(Settings::GetInstance(), SIGNAL(RenderSeaChanged(bool)),
            this, SIGNAL(RenderSeaChanged(bool)));
    connect(Settings::GetInstance(), SIGNAL(GpsFormatChanged(const QString)),
            this, SIGNAL(GpsFormatChanged(const QString)));
}

double QmlSettings::GetPhysicalDPI() const
{
    return Settings::GetInstance()->GetPhysicalDPI();
}

void QmlSettings::SetMapDPI(double dpi)
{
    Settings::GetInstance()->SetMapDPI(dpi);
}

double QmlSettings::GetMapDPI() const
{
    return Settings::GetInstance()->GetMapDPI();
}

MapView *QmlSettings::GetMapView() const
{
    return Settings::GetInstance()->GetMapView();
}

void QmlSettings::SetMapView(QObject *o)
{
    MapView *view = dynamic_cast<MapView*>(o);
    if (view == NULL){
        qWarning() << "Failed to cast " << o << " to MapView*.";
        return;
    }
    Settings::GetInstance()->SetMapView(view);
}

bool QmlSettings::GetOnlineTilesEnabled() const
{
    return Settings::GetInstance()->GetOnlineTilesEnabled();
}

void QmlSettings::SetOnlineTilesEnabled(bool b)
{
    Settings::GetInstance()->SetOnlineTilesEnabled(b);
}

const QString QmlSettings::GetOnlineTileProviderId() const
{
    return Settings::GetInstance()->GetOnlineTileProviderId();
}

void QmlSettings::SetOnlineTileProviderId(QString id)
{
    Settings::GetInstance()->SetOnlineTileProviderId(id);
}

QString QmlSettings::onlineProviderCopyright()
{
    OnlineTileProvider provider = Settings::GetInstance()->GetOnlineTileProvider();
    if (provider.isValid()){
        return provider.getCopyright();
    }
    return "";
}

bool QmlSettings::GetOfflineMap() const
{
    return Settings::GetInstance()->GetOfflineMap();
}
void QmlSettings::SetOfflineMap(bool b)
{
    Settings::GetInstance()->SetOfflineMap(b);
}

bool QmlSettings::GetRenderSea() const
{
    return Settings::GetInstance()->GetRenderSea();
}
void QmlSettings::SetRenderSea(bool b)
{
    Settings::GetInstance()->SetRenderSea(b);
}
const QString QmlSettings::GetGpsFormat() const
{
    return Settings::GetInstance()->GetGpsFormat();
}
void QmlSettings::SetGpsFormat(const QString formatId)
{
    Settings::GetInstance()->SetGpsFormat(formatId);  
}
