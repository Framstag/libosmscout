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

#include <osmscoutclientqt/Settings.h>
#include <osmscoutclientqt/OSMScoutQt.h>

#include <osmscoutclient/json/json.hpp>

#include <QScreen>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QObject>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLocale>

namespace osmscout {

QtSettingsStorage::QtSettingsStorage(QSettings *providedStorage):
  storage(providedStorage)
{
  if (storage==nullptr){
    storage=new QSettings(this);
  } else {
    providedStorage->setParent(this);
  }
}

void QtSettingsStorage::SetValue(const std::string &key, double d)
{
  storage->setValue(QString::fromStdString(key), d);
}

void QtSettingsStorage::SetValue(const std::string &key, uint32_t i)
{
  storage->setValue(QString::fromStdString(key), i);
}

void QtSettingsStorage::SetValue(const std::string &key, const std::string &str)
{
  storage->setValue(QString::fromStdString(key), QString::fromStdString(str));
}

void QtSettingsStorage::SetValue(const std::string &key, bool b)
{
  storage->setValue(QString::fromStdString(key), b);
}

void QtSettingsStorage::SetValue(const std::string &key, std::vector<char> bytes)
{
  storage->setValue(QString::fromStdString(key), QByteArray(bytes.data(), bytes.size()));
}

double QtSettingsStorage::GetDouble(const std::string &key, double defaultValue)
{
  bool ok;
  double d = storage->value(QString::fromStdString(key), defaultValue).toDouble(&ok);
  if (!ok) {
    return defaultValue;
  }
  return d;
}

uint32_t QtSettingsStorage::GetUInt(const std::string &key, uint32_t defaultValue)
{
  bool ok;
  uint32_t i = storage->value(QString::fromStdString(key), defaultValue).toUInt(&ok);
  if (!ok) {
    return defaultValue;
  }
  return i;
}

std::string QtSettingsStorage::GetString(const std::string &key, const std::string &defaultValue)
{
  return storage->value(QString::fromStdString(key), QString::fromStdString(defaultValue)).toString().toStdString();
}

bool QtSettingsStorage::GetBool(const std::string &key, bool defaultValue)
{
  return storage->value(QString::fromStdString(key), defaultValue).toBool();
}

std::vector<char> QtSettingsStorage::GetBytes(const std::string &key)
{
  QByteArray arr = storage->value(QString::fromStdString(key)).toByteArray();
  return std::vector<char>(arr.data(), arr.data() + arr.size());
}

std::vector<std::string> QtSettingsStorage::Keys(const std::string &prefix)
{
  std::vector<std::string> result;
  storage->beginGroup(QString::fromStdString(prefix));
  for (const QString& key:storage->allKeys()){
    result.push_back(prefix + key.toStdString());
  }
  storage->endGroup();
  return result;
}

Settings::Settings(SettingsStoragePtr storage):
  storage(storage)
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

double Settings::GetPhysicalDPI() const
{
    return physicalDpi;
}

void Settings::SetMapDPI(double dpi)
{
    storage->SetValue("OSMScoutLib/Rendering/DPI", dpi);
    mapDPIChange.Emit(dpi);
}

double Settings::GetMapDPI() const
{
  return storage->GetDouble("OSMScoutLib/Rendering/DPI",physicalDpi);
}

osmscout::Vehicle Settings::GetRoutingVehicle() const
{
  return osmscout::Vehicle(storage->GetUInt("OSMScoutLib/Routing/Vehicle",osmscout::vehicleCar));
}

void Settings::SetRoutingVehicle(const osmscout::Vehicle& vehicle)
{
  storage->SetValue("OSMScoutLib/Routing/Vehicle", (uint32_t)vehicle);
}

bool Settings::GetOnlineTilesEnabled() const
{
  return storage->GetBool("OSMScoutLib/Rendering/OnlineTiles", true);
}

void Settings::SetOnlineTilesEnabled(bool b)
{
  if (GetOnlineTilesEnabled() != b){
    storage->SetValue("OSMScoutLib/Rendering/OnlineTiles", b);
    onlineTilesEnabledChanged.Emit(b);
  }
}

const std::vector<OnlineTileProvider> Settings::GetOnlineProviders() const
{
  return onlineProviders;
}

const std::vector<MapProvider> Settings::GetMapProviders() const
{
  return mapProviders;
}

const std::vector<VoiceProvider> Settings::GetVoiceProviders() const
{
  return voiceProviders;
}

const OnlineTileProvider Settings::GetOnlineTileProvider() const
{
    if (auto it=onlineProviderMap.find(GetOnlineTileProviderId());
        it!=onlineProviderMap.end()){
        return it->second;
    }
    return OnlineTileProvider();
}

const std::string Settings::GetOnlineTileProviderId() const
{
    std::string def = "?";
    if (!onlineProviders.empty()){
        def = onlineProviders.begin()->getId();
    }
    return storage->GetString("OSMScoutLib/Rendering/OnlineTileProvider", def);
}

void Settings::SetOnlineTileProviderId(const std::string &id){
    if (GetOnlineTileProviderId() != id){
        storage->SetValue("OSMScoutLib/Rendering/OnlineTileProvider", id);
        onlineTileProviderIdChanged.Emit(id);
        onlineTileProviderChanged.Emit(GetOnlineTileProvider());
    }
}

bool Settings::loadOnlineTileProviders(const QStringList &paths)
{
    // load online tile providers
    bool result = true;
    for (const auto &path : paths) {
      std::vector<char> content;
      if (!ReadFile(path.toStdString(), content)) {
        qWarning() << "Couldn't open" << path << "file.";
        result = false;
        continue;
      }
      qDebug() << "Loading online tile providers from " << path;

      using json = nlohmann::json;
      try {
        auto doc = json::parse(content);
        if (!doc.is_array()) {
          qWarning() << "Json is not array " << QString::fromStdString(doc.dump());
          result = false;
          continue;
        }
        for (auto obj: doc) {
          OnlineTileProvider provider = OnlineTileProvider::fromJson(obj);
          if (!provider.isValid()) {
            qWarning() << "Can't parse online provider from json value" << QString::fromStdString(obj.dump());
            result = false;
          } else {
            if (onlineProviderMap.find(provider.getId())==onlineProviderMap.end()) {
              onlineProviderMap[provider.getId()] = provider;
              onlineProviders.push_back(provider);
            }
          }
        }
      } catch (const json::exception &e) {
        qWarning() << "Failed to parse json from" << path << ":" << e.what();
        result = false;
      }
    }

    // check if current provider is valid...
    if (onlineProviderMap.find(GetOnlineTileProviderId())==onlineProviderMap.end()){
        // ...if not, setup first
        if (!onlineProviders.empty()){
            SetOnlineTileProviderId(onlineProviders.begin()->getId());
        }
    }

    onlineTileProviderIdChanged.Emit(GetOnlineTileProviderId());
    return result && !onlineProviders.empty();
}

namespace { // anonymous namespace

template <typename Provider>
bool loadResourceProviders(const QString &path, std::vector<Provider> &providers)
{
  std::vector<char> content;
  if (!ReadFile(path.toStdString(), content)) {
    qWarning() << "Couldn't open" << path << "file.";
    return false;
  }
  qDebug() << "Loading providers from " << path;

  using json = nlohmann::json;
  try {
    auto doc = json::parse(content);
    if (!doc.is_array()) {
      qWarning() << "Json is not array " << QString::fromStdString(doc.dump());
    } else {
      for (auto obj: doc) {
        Provider provider = Provider::fromJson(obj);
        if (!provider.isValid()) {
          qWarning() << "Can't parse online provider from json value" << QString::fromStdString(obj.dump());
        } else {
          providers.push_back(provider);
        }
      }
    }
  } catch (const json::exception &e) {
    qWarning() << "Failed to parse json from" << path << ":" << e.what();
    return false;
  }
  return true;
}
}

bool Settings::loadMapProviders(const QStringList &paths)
{
  bool result = true;
  for (const auto &path:paths) {
    result &= loadResourceProviders<MapProvider>(path, mapProviders);
  }
  return !mapProviders.empty() && result;
}

bool Settings::loadVoiceProviders(const QStringList &paths)
{
  bool result = true;
  for (const auto &path:paths) {
    result &= loadResourceProviders<VoiceProvider>(path, voiceProviders);
  }
  return !voiceProviders.empty() && result;
}

bool Settings::GetOfflineMap() const
{
  return storage->GetBool("OSMScoutLib/Rendering/OfflineMap", true);
}
void Settings::SetOfflineMap(bool b)
{
  if (GetOfflineMap() != b){
    storage->SetValue("OSMScoutLib/Rendering/OfflineMap", b);
    offlineMapChanged.Emit(b);
  }
}

bool Settings::GetRenderSea() const
{
  return storage->GetBool("OSMScoutLib/Rendering/RenderSea", true);
}
void Settings::SetRenderSea(bool b)
{
  if (GetRenderSea() != b){
    storage->SetValue("OSMScoutLib/Rendering/RenderSea", b);
    renderSeaChanged.Emit(b);
  }
}

const std::string Settings::GetStyleSheetDirectory() const
{
  return storage->GetString("OSMScoutLib/Rendering/StylesheetDirectory", "stylesheets");
}
void Settings::SetStyleSheetDirectory(const std::string &dir)
{
  if (GetStyleSheetDirectory() != dir){
    storage->SetValue("OSMScoutLib/Rendering/StylesheetDirectory", dir);
    styleSheetDirectoryChanged.Emit(dir);
  }
}

const std::string Settings::GetVoiceLookupDirectory() const
{
  return storage->GetString("OSMScoutLib/Voice/LooukupDirectory", ".voices");
}
void Settings::SetVoiceLookupDirectory(const std::string &dir)
{
  if (GetVoiceLookupDirectory() != dir){
    storage->SetValue("OSMScoutLib/Voice/LooukupDirectory", dir);
    voiceLookupDirectoryChanged.Emit(dir);
  }
}

const std::string Settings::GetVoiceDir() const
{
  return storage->GetString("OSMScoutLib/Voice/VoiceDir", "disabled");
}
void Settings::SetVoiceDir(const std::string &voice)
{
  if (GetVoiceDir() != voice){
    storage->SetValue("OSMScoutLib/Voice/VoiceDir", voice);
    voiceDirChanged.Emit(voice);
  }
}

const std::string Settings::GetStyleSheetFile() const
{
  return storage->GetString("OSMScoutLib/Rendering/StylesheetFile", "standard.oss");
}
const std::string Settings::GetStyleSheetAbsoluteFile() const
{
  return QFileInfo(QString::fromStdString(GetStyleSheetDirectory()), QString::fromStdString(GetStyleSheetFile()))
    .absoluteFilePath().toStdString();
}
void Settings::SetStyleSheetFile(const std::string &file)
{
  if (GetStyleSheetFile() != file){
    storage->SetValue("OSMScoutLib/Rendering/StylesheetFile", file);
    styleSheetFileChanged.Emit(file);
  }
}

const std::unordered_map<std::string,bool> Settings::GetStyleSheetFlags(const std::string &styleSheetFile)
{
  std::unordered_map<std::string,bool> stylesheetFlags;
  std::string prefix = "OSMScoutLib/Rendering/StylesheetFlags/" + styleSheetFile + "/";
  auto keys = storage->Keys(prefix);
  for (const std::string& key:keys){
    stylesheetFlags[key.substr(prefix.length())]=storage->GetBool(key, false);
  }
  return stylesheetFlags;
}
const std::unordered_map<std::string,bool> Settings::GetStyleSheetFlags()
{
  return GetStyleSheetFlags(GetStyleSheetFile());
}
void Settings::SetStyleSheetFlags(const std::string &styleSheetFile, std::unordered_map<std::string,bool> flags)
{
  std::string prefix = "OSMScoutLib/Rendering/StylesheetFlags/" + styleSheetFile + "/";
  for (const auto &entry:flags){
    storage->SetValue(prefix + entry.first, entry.second);
  }
}
void Settings::SetStyleSheetFlags(std::unordered_map<std::string,bool> flags)
{
  SetStyleSheetFlags(GetStyleSheetFile(), flags);
}

std::string Settings::GetFontName() const
{
  return storage->GetString("OSMScoutLib/Rendering/FontName", "sans-serif");
}
void Settings::SetFontName(const std::string &fontName)
{
  if (GetFontName()!=fontName){
    storage->SetValue("OSMScoutLib/Rendering/FontName", fontName);
    fontNameChanged.Emit(fontName);
  }
}

double Settings::GetFontSize() const
{
  return storage->GetDouble("OSMScoutLib/Rendering/FontSize", 2.0);
}
void Settings::SetFontSize(double fontSize)
{
  if (GetFontSize()!=fontSize){
    storage->SetValue("OSMScoutLib/Rendering/FontSize", fontSize);
    fontSizeChanged.Emit(fontSize);
  }
}

bool Settings::GetShowAltLanguage() const
{
  return storage->GetBool("OSMScoutLib/Rendering/ShowAltLanguage", false);
}
void Settings::SetShowAltLanguage(bool showAltLanguage)
{
  if (GetShowAltLanguage()!=showAltLanguage){
    storage->SetValue("OSMScoutLib/Rendering/ShowAltLanguage", showAltLanguage);
    showAltLanguageChanged.Emit(showAltLanguage);
  }
}

const std::string Settings::GetHttpCacheDir() const
{
  QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  return (cacheLocation + QDir::separator() + "OSMScoutHttpCache").toStdString();
}

const std::vector<char> Settings::GetCookieData() const
{
  return storage->GetBytes("OSMScoutLib/General/Cookies");
}

void Settings::SetCookieData(const std::vector<char> &data)
{
  storage->SetValue("OSMScoutLib/General/Cookies", data);
}

std::string Settings::GetUnits() const
{
  QLocale locale;
  QString defaultUnits;
  switch (locale.measurementSystem()){
    case QLocale::ImperialUSSystem:
    case QLocale::ImperialUKSystem:
      defaultUnits="imperial";
      break;
    case QLocale::MetricSystem:
    default:
      defaultUnits="metrics";
  }
  return storage->GetString("OSMScoutLib/General/Units", defaultUnits.toStdString());
}

void Settings::SetUnits(const std::string &units)
{
  if (GetUnits()!=units){
    storage->SetValue("OSMScoutLib/General/Units", units);
    unitsChanged.Emit(units);
  }
}

QmlSettings::QmlSettings()
{
    settings=OSMScoutQt::GetInstance().GetSettings();

    settings->mapDPIChange.Connect(mapDPISlot);
    settings->onlineTilesEnabledChanged.Connect(onlineTilesEnabledSlot);
    settings->onlineTileProviderIdChanged.Connect(onlineTileProviderIdSlot);
    settings->offlineMapChanged.Connect(offlineMapSlot);
    settings->styleSheetFileChanged.Connect(styleSheetFileSlot);
    settings->renderSeaChanged.Connect(renderSeaSlot);
    settings->fontNameChanged.Connect(fontNameSlot);
    settings->fontSizeChanged.Connect(fontSizeSlot);
    settings->showAltLanguageChanged.Connect(showAltLanguageSlot);
    settings->unitsChanged.Connect(unitsSlot);
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
    return QString::fromStdString(settings->GetOnlineTileProviderId());
}

void QmlSettings::SetOnlineTileProviderId(QString id)
{
    settings->SetOnlineTileProviderId(id.toStdString());
}

QString QmlSettings::onlineProviderCopyright()
{
    OnlineTileProvider provider = settings->GetOnlineTileProvider();
    if (provider.isValid()){
        return QString::fromStdString(provider.getCopyright());
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

QString QmlSettings::GetStyleSheetFile() const
{
    return QString::fromStdString(settings->GetStyleSheetFile());
}
void QmlSettings::SetStyleSheetFile(const QString file)
{
    settings->SetStyleSheetFile(file.toStdString());
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
    return QString::fromStdString(settings->GetFontName());
}
void QmlSettings::SetFontName(const QString fontName)
{
    settings->SetFontName(fontName.toStdString());
}
double QmlSettings::GetFontSize() const
{
    return settings->GetFontSize();
}
void QmlSettings::SetFontSize(double fontSize)
{
    settings->SetFontSize(fontSize);
}
bool QmlSettings::GetShowAltLanguage() const
{
    return settings->GetShowAltLanguage();
}
void QmlSettings::SetShowAltLanguage(bool showAltLanguage)
{
    settings->SetShowAltLanguage(showAltLanguage);
}
QString QmlSettings::GetUnits() const
{
    return QString::fromStdString(settings->GetUnits());
}
void QmlSettings::SetUnits(const QString units)
{
    settings->SetUnits(units.toStdString());
}
}
