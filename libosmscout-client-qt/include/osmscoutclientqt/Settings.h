#ifndef OSMSCOUT_CLIENT_QT_SETTINGS_H
#define OSMSCOUT_CLIENT_QT_SETTINGS_H

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

#include <memory>
#include <map>

#include <QSettings>
#include <QStringList>

#include <osmscout/async/Signal.h>
#include <osmscout/routing/RoutingProfile.h>

#include <osmscoutclient/OnlineTileProvider.h>
#include <osmscoutclient/VoiceProvider.h>

#include <osmscoutclientqt/MapProvider.h>
#include <osmscoutclientqt/ClientQtImportExport.h>

// this variable should be defined by build system
#ifndef LIBOSMSCOUT_VERSION_STRING
#define LIBOSMSCOUT_VERSION_STRING "v?"
#endif

namespace osmscout {

struct OSMSCOUT_CLIENT_QT_API SettingsStorage
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
 * \ingroup QtAPI
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
class OSMSCOUT_CLIENT_QT_API Settings
{
private:
  SettingsStoragePtr storage;
  double    physicalDpi;
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
  explicit Settings(SettingsStoragePtr storage);
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

  bool loadOnlineTileProviders(const QStringList &paths);
  bool loadMapProviders(const QStringList &paths);
  bool loadVoiceProviders(const QStringList &paths);

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

  const std::string GetHttpCacheDir() const;

  const std::vector<char> GetCookieData() const;
  void SetCookieData(const std::vector<char> &data);

  std::string GetUnits() const;
  void SetUnits(const std::string &units);
};

/**
 * \ingroup QtAPI
 */
using SettingsRef = std::shared_ptr<Settings>;

/**
 * \ingroup QtAPI
 *
 * Provides interface to Settings object from QML. It should be registered
 * by qmlRegisterType before first use.
 *
 * ```
 * qmlRegisterType<QmlSettings>("net.sf.libosmscout.map", 1, 0, "Settings");
 * ```
 *
 * It may be imported and used in QML then:
 * ```
 * import net.sf.libosmscout.map 1.0
 *
 * Settings {
 *   id: settings
 * }
 * ```
 */
class OSMSCOUT_CLIENT_QT_API QmlSettings: public QObject{
  Q_OBJECT
  Q_PROPERTY(double   physicalDPI READ GetPhysicalDPI CONSTANT)
  Q_PROPERTY(double   mapDPI    READ GetMapDPI  WRITE SetMapDPI   NOTIFY MapDPIChange)
  Q_PROPERTY(bool     onlineTiles READ GetOnlineTilesEnabled WRITE SetOnlineTilesEnabled NOTIFY OnlineTilesEnabledChanged)
  Q_PROPERTY(QString  onlineTileProviderId READ GetOnlineTileProviderId WRITE SetOnlineTileProviderId NOTIFY OnlineTileProviderIdChanged)
  Q_PROPERTY(bool     offlineMap READ GetOfflineMap WRITE SetOfflineMap NOTIFY OfflineMapChanged)
  Q_PROPERTY(QString  styleSheetFile READ GetStyleSheetFile WRITE SetStyleSheetFile NOTIFY StyleSheetFileChanged)
  Q_PROPERTY(bool     renderSea  READ GetRenderSea  WRITE SetRenderSea  NOTIFY RenderSeaChanged)
  Q_PROPERTY(QString  fontName    READ GetFontName            WRITE SetFontName     NOTIFY FontNameChanged)
  Q_PROPERTY(double   fontSize    READ GetFontSize            WRITE SetFontSize     NOTIFY FontSizeChanged)
  Q_PROPERTY(bool     showAltLanguage READ GetShowAltLanguage WRITE SetShowAltLanguage NOTIFY ShowAltLanguageChanged)
  /// metrics or imperial
  Q_PROPERTY(QString  units       READ GetUnits               WRITE SetUnits        NOTIFY UnitsChanged)

private:
  SettingsRef settings;

  // slots
  Slot<double> mapDPISlot{
    [this](const double &d){ this->MapDPIChange(d); }
  };

  Slot<bool> onlineTilesEnabledSlot{
    [this](const bool &b){this->OnlineTilesEnabledChanged(b);}
  };

  Slot<std::string> onlineTileProviderIdSlot{
    [this](const std::string &str){ this->OnlineTileProviderIdChanged(QString::fromStdString(str));}
  };

  Slot<bool> offlineMapSlot{
    [this](const bool &b){ this->OfflineMapChanged(b);}
  };

  Slot<std::string> styleSheetFileSlot{
    [this](const std::string &str){ this->StyleSheetFileChanged(QString::fromStdString(str));}
  };

  Slot<bool> renderSeaSlot{
    [this](const bool &b){ this->RenderSeaChanged(b);}
  };

  Slot<std::string> fontNameSlot{
    [this](const std::string &str){ this->FontNameChanged(QString::fromStdString(str));}
  };

  Slot<double> fontSizeSlot{
    [this](const double &d){ this->FontSizeChanged(d);}
  };

  Slot<bool> showAltLanguageSlot{
    [this](const bool &b){ this->ShowAltLanguageChanged(b);}
  };

  Slot<std::string> unitsSlot{
    [this](const std::string &str){ this->UnitsChanged(QString::fromStdString(str));}
  };

signals:
  void MapDPIChange(double dpi);
  void OnlineTilesEnabledChanged(bool enabled);
  void OnlineTileProviderIdChanged(const QString id);
  void OfflineMapChanged(bool);
  void StyleSheetFileChanged(const QString file);
  void RenderSeaChanged(bool);
  void FontNameChanged(const QString fontName);
  void FontSizeChanged(double fontSize);
  void ShowAltLanguageChanged(bool showAltLanguage);
  void UnitsChanged(const QString units);

public:
  QmlSettings();

  ~QmlSettings() override = default;

  double GetPhysicalDPI() const;

  void SetMapDPI(double dpi);
  double GetMapDPI() const;

  bool GetOnlineTilesEnabled() const;
  void SetOnlineTilesEnabled(bool b);

  const QString GetOnlineTileProviderId() const;
  void SetOnlineTileProviderId(QString id);

  Q_INVOKABLE QString onlineProviderCopyright();

  bool GetOfflineMap() const;
  void SetOfflineMap(bool);

  QString GetStyleSheetFile() const;
  void SetStyleSheetFile(const QString file);

  bool GetRenderSea() const;
  void SetRenderSea(bool);

  QString GetFontName() const;
  void SetFontName(const QString fontName);

  double GetFontSize() const;
  void SetFontSize(double fontSize);

  bool GetShowAltLanguage() const;
  void SetShowAltLanguage(bool showAltLanguage);

  QString GetUnits() const;
  void SetUnits(const QString units);
};

}

#endif
