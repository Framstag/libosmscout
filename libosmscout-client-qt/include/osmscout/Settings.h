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

#include <QSettings>

#include <osmscout/RoutingProfile.h>
#include <osmscout/InputHandler.h>
#include <osmscout/OnlineTileProvider.h>
#include <osmscout/MapProvider.h>

#include <osmscout/private/ClientQtImportExport.h>

// these variables should be defined by build system
#ifndef OSMSCOUT_VERSION_STRING
#define OSMSCOUT_VERSION_STRING "v?"
#endif

#ifndef OSMSCOUT_USER_AGENT
#define OSMSCOUT_USER_AGENT "OSMScout demo app %1"
#endif

/**
 * \ingroup QtAPI
 * 
 * Settings provides central point mutable configuration of OSMScout library.
 * It uses Qt's QSettings for persistency. It may be accessed from DBThread instance.
 * 
 * List of online tile providers should be initialized at applicaiton start.
 * ```
 *   Settings::GetInstance()->loadOnlineTileProviders(
 *     ":/resources/online-tile-providers.json");
 * ```
 * 
 * Before program exit, resources shoudl be released by calling Settings::FreeInstance.
 */
class OSMSCOUT_CLIENT_QT_API Settings: public QObject
{
  Q_OBJECT
  Q_PROPERTY(double   mapDPI      READ GetMapDPI              WRITE SetMapDPI       NOTIFY MapDPIChange)
  Q_PROPERTY(bool     onlineTiles READ GetOnlineTilesEnabled WRITE SetOnlineTilesEnabled NOTIFY OnlineTilesEnabledChanged)
  Q_PROPERTY(QString  onlineTileProviderId READ GetOnlineTileProviderId WRITE SetOnlineTileProviderId NOTIFY OnlineTileProviderIdChanged)
  Q_PROPERTY(bool     offlineMap  READ GetOfflineMap          WRITE SetOfflineMap   NOTIFY OfflineMapChanged)
  Q_PROPERTY(bool     renderSea   READ GetRenderSea           WRITE SetRenderSea    NOTIFY RenderSeaChanged)
  Q_PROPERTY(QString  styleSheetDirectory READ GetStyleSheetDirectory WRITE SetStyleSheetDirectory NOTIFY StyleSheetDirectoryChanged)
  Q_PROPERTY(QString  styleSheetFile      READ GetStyleSheetFile      WRITE SetStyleSheetFile      NOTIFY StyleSheetFileChanged)
  Q_PROPERTY(QString  fontName    READ GetFontName            WRITE SetFontName     NOTIFY FontNameChanged)
  Q_PROPERTY(double   fontSize    READ GetFontSize            WRITE SetFontSize     NOTIFY FontSizeChanged)

signals:
  void MapDPIChange(double dpi);
  void OnlineTilesEnabledChanged(bool);
  void OnlineTileProviderIdChanged(const QString id);
  void OfflineMapChanged(bool);
  void RenderSeaChanged(bool);
  void StyleSheetDirectoryChanged(const QString dir);
  void StyleSheetFileChanged(const QString file);
  void FontNameChanged(const QString fontName);
  void FontSizeChanged(double fontSize);

private:
  QSettings *storage;
  double    physicalDpi;
  QMap<QString, OnlineTileProvider> onlineProviderMap;
  QList<OnlineTileProvider> onlineProviders;
  QList<MapProvider> mapProviders;

public:
  Settings(QSettings *providedStorage=NULL);
  virtual ~Settings();

  double GetPhysicalDPI() const;
  
  void SetMapDPI(double dpi);
  double GetMapDPI() const;

  osmscout::Vehicle GetRoutingVehicle() const;
  void SetRoutingVehicle(const osmscout::Vehicle& vehicle);
  
  bool GetOnlineTilesEnabled() const;
  void SetOnlineTilesEnabled(bool b);
  
  const QList<OnlineTileProvider> GetOnlineProviders() const;
  const OnlineTileProvider GetOnlineTileProvider() const; 

  const QList<MapProvider> GetMapProviders() const;

  const QString GetOnlineTileProviderId() const; 
  void SetOnlineTileProviderId(QString id);
  
  bool loadOnlineTileProviders(QString path);
  bool loadMapProviders(QString path);
  
  bool GetOfflineMap() const;
  void SetOfflineMap(bool);
  
  bool GetRenderSea() const;
  void SetRenderSea(bool);

  const QString GetStyleSheetDirectory() const;
  void SetStyleSheetDirectory(const QString dir);

  const QString GetStyleSheetFile() const;
  const QString GetStyleSheetAbsoluteFile() const;
  void SetStyleSheetFile(const QString file);

  const std::unordered_map<std::string,bool> GetStyleSheetFlags(const QString styleSheetFile);
  const std::unordered_map<std::string,bool> GetStyleSheetFlags();
  void SetStyleSheetFlags(const QString styleSheetFile, std::unordered_map<std::string,bool> flags);
  void SetStyleSheetFlags(std::unordered_map<std::string,bool> flags);

  QString GetFontName() const;
  void SetFontName(const QString fontName);

  double GetFontSize() const;
  void SetFontSize(double fontSize);

  const QString GetHttpCacheDir() const;  
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<Settings> SettingsRef;

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
  Q_PROPERTY(bool     renderSea  READ GetRenderSea  WRITE SetRenderSea  NOTIFY RenderSeaChanged)
  Q_PROPERTY(QString  fontName    READ GetFontName            WRITE SetFontName     NOTIFY FontNameChanged)
  Q_PROPERTY(double   fontSize    READ GetFontSize            WRITE SetFontSize     NOTIFY FontSizeChanged)

private:
  SettingsRef settings;

signals:
  void MapDPIChange(double dpi);
  void OnlineTilesEnabledChanged(bool enabled);
  void OnlineTileProviderIdChanged(const QString id);
  void OfflineMapChanged(bool);
  void RenderSeaChanged(bool);
  void FontNameChanged(const QString fontName);
  void FontSizeChanged(double fontSize);

public:
  QmlSettings();
  
  virtual inline ~QmlSettings(){};

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
  
  bool GetRenderSea() const;
  void SetRenderSea(bool);

  QString GetFontName() const;
  void SetFontName(const QString fontName);

  double GetFontSize() const;
  void SetFontSize(double fontSize);
};

#endif
