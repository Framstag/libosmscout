#ifndef OSMSCOUT_CLIENT_SETTINGS_H
#define OSMSCOUT_CLIENT_SETTINGS_H

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

#include <osmscout/async/Signal.h>
#include <osmscout/routing/RoutingProfile.h>

#include <osmscoutclient/ClientImportExport.h>
#include <osmscoutclient/MapProvider.h>
#include <osmscoutclient/OnlineTileProvider.h>
#include <osmscoutclient/VoiceProvider.h>

#include <memory>
#include <map>

// this variable should be defined by build system
#ifndef LIBOSMSCOUT_VERSION_STRING
#define LIBOSMSCOUT_VERSION_STRING "v?"
#endif

namespace osmscout {

/**
 * \ingroup ClientAPI
 *
 * Abstract settings storage.
 */
struct OSMSCOUT_CLIENT_API SettingsStorage
{
public:
  virtual void SetValue(const std::string &key, double d) = 0;
  virtual void SetValue(const std::string &key, uint32_t i) = 0;
  virtual void SetValue(const std::string &key, const std::string &str) = 0;
  virtual void SetValue(const std::string &key, bool b) = 0;
  virtual void SetValue(const std::string &key, std::vector<char> bytes) = 0;
  virtual double GetDouble(const std::string &key, double defaultValue = 0) = 0;
  virtual uint32_t GetUInt(const std::string &key, uint32_t defaultValue = 0) = 0;
  virtual std::string GetString(const std::string &key, const std::string &defaultValue = "") = 0;
  virtual bool GetBool(const std::string &key, bool defaultValue = 0) = 0;
  virtual std::vector<char> GetBytes(const std::string &key) = 0;

  /** Get all configuration keys with given prefix.
   * @param prefix
   * @return keys (containing prefix)
   */
  virtual std::vector<std::string> Keys(const std::string &prefix) = 0;
};

using SettingsStoragePtr = std::shared_ptr<SettingsStorage>;

/**
 * \ingroup ClientAPI
 *
 * Settings provides central point mutable configuration of OSMScout library.
 * It uses Qt's QSettings for persistency. It may be accessed from DBThread instance.
 *
 * List of online tile providers should be initialized at application start.
 * ```
 *   OSMScoutQt::GetInstance().GetSettings()->loadOnlineTileProviders(
 *     ":/resources/online-tile-providers.json");
 * ```
 *
 * Before program exit, resources should be released by calling Settings::FreeInstance.
 */
class OSMSCOUT_CLIENT_API Settings
{
private:
  SettingsStoragePtr storage;
  double physicalDpi;
  std::string defaultUnits;
  std::map<std::string, OnlineTileProvider> onlineProviderMap;
  std::vector<OnlineTileProvider> onlineProviders;
  std::vector<MapProvider> mapProviders;
  std::vector<VoiceProvider> voiceProviders;

public:
  Signal<double> mapDPIChange;
  Signal<bool> onlineTilesEnabledChanged;
  Signal<std::string> onlineTileProviderIdChanged;
  Signal<OnlineTileProvider> onlineTileProviderChanged;
  Signal<bool> offlineMapChanged;
  Signal<bool> renderSeaChanged;
  Signal<std::string> styleSheetDirectoryChanged;
  Signal<std::string> styleSheetFileChanged;
  Signal<std::string> voiceLookupDirectoryChanged;
  Signal<std::string> voiceDirChanged;
  Signal<std::string> fontNameChanged;
  Signal<double> fontSizeChanged;
  Signal<bool> showAltLanguageChanged;
  Signal<std::string> unitsChanged;

public:
  /**
   *
   * @param storage
   * @param physicalDpi
   * @param defaultUnits - metrics or imperial
   */
  Settings(SettingsStoragePtr storage, double physicalDpi, const std::string &defaultUnits);
  virtual ~Settings() = default;

  double GetPhysicalDPI() const;

  void SetMapDPI(double dpi);
  double GetMapDPI() const;

  osmscout::Vehicle GetRoutingVehicle() const;
  void SetRoutingVehicle(const osmscout::Vehicle& vehicle);

  bool GetOnlineTilesEnabled() const;
  void SetOnlineTilesEnabled(bool b);

  const std::vector<OnlineTileProvider> GetOnlineProviders() const;
  const OnlineTileProvider GetOnlineTileProvider() const;

  const std::vector<MapProvider> GetMapProviders() const;
  const std::vector<VoiceProvider> GetVoiceProviders() const;

  const std::string GetOnlineTileProviderId() const;
  void SetOnlineTileProviderId(const std::string &id);

  bool loadOnlineTileProviders(const std::vector<std::string> &paths);
  bool loadMapProviders(const std::vector<std::string> &paths);
  bool loadVoiceProviders(const std::vector<std::string> &paths);

  bool GetOfflineMap() const;
  void SetOfflineMap(bool);

  bool GetRenderSea() const;
  void SetRenderSea(bool);

  const std::string GetStyleSheetDirectory() const;
  void SetStyleSheetDirectory(const std::string &dir);

  const std::string GetVoiceLookupDirectory() const;
  void SetVoiceLookupDirectory(const std::string &voiceLookupDirectory);

  const std::string GetVoiceDir() const;
  void SetVoiceDir(const std::string &voice);

  const std::string GetStyleSheetFile() const;
  const std::string GetStyleSheetAbsoluteFile() const;
  void SetStyleSheetFile(const std::string &file);

  const std::unordered_map<std::string,bool> GetStyleSheetFlags(const std::string &styleSheetFile);
  const std::unordered_map<std::string,bool> GetStyleSheetFlags();
  void SetStyleSheetFlags(const std::string &styleSheetFile, std::unordered_map<std::string,bool> flags);
  void SetStyleSheetFlags(std::unordered_map<std::string,bool> flags);

  std::string GetFontName() const;
  void SetFontName(const std::string &fontName);

  double GetFontSize() const;
  void SetFontSize(double fontSize);

  bool GetShowAltLanguage() const;
  void SetShowAltLanguage(bool showAltLanguage);

  const std::vector<char> GetCookieData() const;
  void SetCookieData(const std::vector<char> &data);

  std::string GetUnits() const;
  void SetUnits(const std::string &units);
};

/**
 * \ingroup ClientAPI
 */
using SettingsRef = std::shared_ptr<Settings>;

}

#endif
