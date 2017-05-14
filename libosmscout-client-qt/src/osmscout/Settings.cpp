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
#include <QFileInfo>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <osmscout/Settings.h>
#include <osmscout/OSMScoutQt.h>

Settings::Settings(QSettings *providedStorage):
  storage(providedStorage)
{
    if (storage==NULL){
      storage=new QSettings(this);
    }
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
    // QObject will delete view and settings if this is owner
}

double Settings::GetPhysicalDPI() const
{
    return physicalDpi;
}

void Settings::SetMapDPI(double dpi)
{
    storage->setValue("OSMScoutLib/Rendering/DPI", (unsigned int)dpi);
    emit MapDPIChange(dpi);
}

double Settings::GetMapDPI() const
{
  return (size_t)storage->value("OSMScoutLib/Rendering/DPI",physicalDpi).toDouble();
}

osmscout::Vehicle Settings::GetRoutingVehicle() const
{
  return (osmscout::Vehicle)storage->value("OSMScoutLib/Routing/Vehicle",osmscout::vehicleCar).toUInt();
}

void Settings::SetRoutingVehicle(const osmscout::Vehicle& vehicle)
{
  storage->setValue("OSMScoutLib/Routing/Vehicle", (unsigned int)vehicle);
}

bool Settings::GetOnlineTilesEnabled() const
{
  return storage->value("OSMScoutLib/Rendering/OnlineTiles", true).toBool();
}

void Settings::SetOnlineTilesEnabled(bool b)
{
  if (GetOnlineTilesEnabled() != b){
    storage->setValue("OSMScoutLib/Rendering/OnlineTiles", b);
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
    return storage->value("OSMScoutLib/Rendering/OnlineTileProvider", def).toString();
}

void Settings::SetOnlineTileProviderId(QString id){
    if (GetOnlineTileProviderId() != id){
        storage->setValue("OSMScoutLib/Rendering/OnlineTileProvider", id);
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
    return storage->value("OSMScoutLib/Rendering/OfflineMap", true).toBool();
}
void Settings::SetOfflineMap(bool b)
{
  if (GetOfflineMap() != b){
    storage->setValue("OSMScoutLib/Rendering/OfflineMap", b);
    emit OfflineMapChanged(b);
  }
}

bool Settings::GetRenderSea() const
{
  return storage->value("OSMScoutLib/Rendering/RenderSea", true).toBool();
}
void Settings::SetRenderSea(bool b)
{
  if (GetRenderSea() != b){
    storage->setValue("OSMScoutLib/Rendering/RenderSea", b);
    emit RenderSeaChanged(b);
  }    
}

const QString Settings::GetStyleSheetDirectory() const
{
  return storage->value("OSMScoutLib/Rendering/StylesheetDirectory", "stylesheets").toString();
}
void Settings::SetStyleSheetDirectory(const QString dir)
{
  if (GetStyleSheetDirectory() != dir){
    storage->setValue("OSMScoutLib/Rendering/StylesheetDirectory", dir);
    emit StyleSheetDirectoryChanged(dir);
  }
}

const QString Settings::GetStyleSheetFile() const
{
  return storage->value("OSMScoutLib/Rendering/StylesheetFile", "standard.oss").toString();
}
const QString Settings::GetStyleSheetAbsoluteFile() const
{
  return QFileInfo(GetStyleSheetDirectory(), GetStyleSheetFile()).absoluteFilePath();
}
void Settings::SetStyleSheetFile(const QString file)
{
  if (GetStyleSheetFile() != file){
    storage->setValue("OSMScoutLib/Rendering/StylesheetFile", file);
    emit StyleSheetFileChanged(file);
  }
}

const std::unordered_map<std::string,bool> Settings::GetStyleSheetFlags(const QString styleSheetFile)
{
  std::unordered_map<std::string,bool> stylesheetFlags; // TODO: read from config
  storage->beginGroup("OSMScoutLib/Rendering/StylesheetFlags/"+styleSheetFile);
  for (const QString key:storage->allKeys()){
    stylesheetFlags[key.toStdString()]=storage->value(key, false).toBool();
  }
  storage->endGroup();
  return stylesheetFlags;
}
const std::unordered_map<std::string,bool> Settings::GetStyleSheetFlags()
{
  return GetStyleSheetFlags(GetStyleSheetFile());
}
void Settings::SetStyleSheetFlags(const QString styleSheetFile, std::unordered_map<std::string,bool> flags)
{
  storage->beginGroup("OSMScoutLib/Rendering/StylesheetFlags/"+styleSheetFile);
  for (const auto &entry:flags){
    storage->setValue(QString::fromStdString(entry.first), entry.second);
  }
  storage->endGroup();
}
void Settings::SetStyleSheetFlags(std::unordered_map<std::string,bool> flags)
{
  SetStyleSheetFlags(GetStyleSheetFile(), flags);
}

QString Settings::GetFontName() const
{
  return storage->value("OSMScoutLib/Rendering/FontName", "sans-serif").toString();
}
void Settings::SetFontName(const QString fontName)
{
  if (GetFontName()!=fontName){
    storage->setValue("OSMScoutLib/Rendering/FontName", fontName);
    emit FontNameChanged(fontName);
  }
}

double Settings::GetFontSize() const
{
  return storage->value("OSMScoutLib/Rendering/FontSize", 2.0).toDouble();
}
void Settings::SetFontSize(double fontSize)
{
  if (GetFontSize()!=fontSize){
    storage->setValue("OSMScoutLib/Rendering/FontSize", fontSize);
    emit FontSizeChanged(fontSize);
  }
}

const QString Settings::GetHttpCacheDir() const
{
  QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);  
  return cacheLocation + QDir::separator() + "OSMScoutHttpCache";
}

QmlSettings::QmlSettings()
{
    settings=OSMScoutQt::GetInstance().GetSettings();

    connect(settings.get(), SIGNAL(MapDPIChange(double)),
            this, SIGNAL(MapDPIChange(double)));
    connect(settings.get(), SIGNAL(OnlineTilesEnabledChanged(bool)),
            this, SIGNAL(OnlineTilesEnabledChanged(bool)));
    connect(settings.get(), SIGNAL(OnlineTileProviderIdChanged(const QString)),
            this, SIGNAL(OnlineTileProviderIdChanged(const QString)));
    connect(settings.get(), SIGNAL(OfflineMapChanged(bool)),
            this, SIGNAL(OfflineMapChanged(bool)));
    connect(settings.get(), SIGNAL(RenderSeaChanged(bool)),
            this, SIGNAL(RenderSeaChanged(bool)));
    connect(settings.get(), SIGNAL(FontNameChanged(const QString)),
            this, SIGNAL(FontNameChanged(const QString)));
    connect(settings.get(), SIGNAL(FontSizeChanged(double)),
            this, SIGNAL(FontSizeChanged(double)));
}

double QmlSettings::GetPhysicalDPI() const
{
    return settings->GetPhysicalDPI();
}

void QmlSettings::SetMapDPI(double dpi)
{
    settings->SetMapDPI(dpi);
}

double QmlSettings::GetMapDPI() const
{
    return settings->GetMapDPI();
}

bool QmlSettings::GetOnlineTilesEnabled() const
{
    return settings->GetOnlineTilesEnabled();
}

void QmlSettings::SetOnlineTilesEnabled(bool b)
{
    settings->SetOnlineTilesEnabled(b);
}

const QString QmlSettings::GetOnlineTileProviderId() const
{
    return settings->GetOnlineTileProviderId();
}

void QmlSettings::SetOnlineTileProviderId(QString id)
{
    settings->SetOnlineTileProviderId(id);
}

QString QmlSettings::onlineProviderCopyright()
{
    OnlineTileProvider provider = settings->GetOnlineTileProvider();
    if (provider.isValid()){
        return provider.getCopyright();
    }
    return "";
}

bool QmlSettings::GetOfflineMap() const
{
    return settings->GetOfflineMap();
}
void QmlSettings::SetOfflineMap(bool b)
{
    settings->SetOfflineMap(b);
}

bool QmlSettings::GetRenderSea() const
{
    return settings->GetRenderSea();
}
void QmlSettings::SetRenderSea(bool b)
{
    settings->SetRenderSea(b);
}
QString QmlSettings::GetFontName() const
{
    return settings->GetFontName();
}
void QmlSettings::SetFontName(const QString fontName)
{
    settings->SetFontName(fontName);
}
double QmlSettings::GetFontSize() const
{
    return settings->GetFontSize();
}
void QmlSettings::SetFontSize(double fontSize)
{
    settings->SetFontSize(fontSize);
}
