/*
  This source is part of the libosmscout library
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

#include <osmscoutclient/Settings.h>

#include <osmscout/io/File.h>
#include <osmscoutclient/json/json.hpp>

namespace osmscout {

Settings::Settings(SettingsStoragePtr storage, double physicalDpi, const std::string &defaultUnits):
  storage(storage), physicalDpi(physicalDpi), defaultUnits(defaultUnits)
{
  // no code
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

bool Settings::loadOnlineTileProviders(const std::vector<std::string> &paths)
{
    // load online tile providers
    bool result = true;
    for (const auto &path : paths) {
      std::vector<char> content;
      if (!ReadFile(path, content)) {
        log.Warn() << "Couldn't open" << path << "file.";
        result = false;
        continue;
      }
      log.Debug() << "Loading online tile providers from " << path;

      using json = nlohmann::json;
      try {
        auto doc = json::parse(content);
        if (!doc.is_array()) {
          log.Warn() << "Json is not array " << doc.dump();
          result = false;
          continue;
        }
        for (auto obj: doc) {
          OnlineTileProvider provider = OnlineTileProvider::fromJson(obj);
          if (!provider.isValid()) {
            log.Warn() << "Can't parse online provider from json value" << obj.dump();
            result = false;
          } else {
            if (onlineProviderMap.find(provider.getId())==onlineProviderMap.end()) {
              onlineProviderMap[provider.getId()] = provider;
              onlineProviders.push_back(provider);
            }
          }
        }
      } catch (const json::exception &e) {
        log.Warn() << "Failed to parse json from" << path << ":" << e.what();
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
bool loadResourceProviders(const std::string &path, std::vector<Provider> &providers)
{
  std::vector<char> content;
  if (!ReadFile(path, content)) {
    log.Warn() << "Couldn't open " << path << " file.";
    return false;
  }
  log.Debug() << "Loading providers from " << path;

  using json = nlohmann::json;
  try {
    auto doc = json::parse(content);
    if (!doc.is_array()) {
      log.Warn() << "Json is not array " << doc.dump();
    } else {
      for (auto obj: doc) {
        Provider provider = Provider::fromJson(obj);
        if (!provider.isValid()) {
          log.Warn() << "Can't parse online provider from json value " << obj.dump();
        } else {
          providers.push_back(provider);
        }
      }
    }
  } catch (const json::exception &e) {
    log.Warn() << "Failed to parse json from " << path << ": " << e.what();
    return false;
  }
  return true;
}
}

bool Settings::loadMapProviders(const std::vector<std::string> &paths)
{
  bool result = true;
  for (const auto &path:paths) {
    result &= loadResourceProviders<MapProvider>(path, mapProviders);
  }
  return !mapProviders.empty() && result;
}

bool Settings::loadVoiceProviders(const std::vector<std::string> &paths)
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
  return GetStyleSheetDirectory() + "/" + GetStyleSheetFile();
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
  return storage->GetString("OSMScoutLib/General/Units", defaultUnits);
}

void Settings::SetUnits(const std::string &units)
{
  if (GetUnits()!=units){
    storage->SetValue("OSMScoutLib/General/Units", units);
    unitsChanged.Emit(units);
  }
}
}
