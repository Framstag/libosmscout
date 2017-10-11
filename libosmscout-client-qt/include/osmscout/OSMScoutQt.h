#ifndef OSMSCOUT_CLIENT_QT_OSMSCOUTQT_H
#define OSMSCOUT_CLIENT_QT_OSMSCOUTQT_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2017 Lukas Karas

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

#include <QSettings>

#include <osmscout/DataTileCache.h>
#include <osmscout/DBThread.h>
#include <osmscout/LookupModule.h>
#include <osmscout/MapRenderer.h>
#include <osmscout/Router.h>
#include <osmscout/SearchModule.h>
#include <osmscout/StyleModule.h>

#include <osmscout/private/ClientQtImportExport.h>

class OSMScoutQt;
Q_DECLARE_METATYPE(osmscout::TileRef)
Q_DECLARE_METATYPE(osmscout::BreakerRef)

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API OSMScoutQtBuilder{
private:
  QSettings *settingsStorage;

  QString onlineTileProviders;
  QString mapProviders;
  QStringList mapLookupDirectories;
  QString basemapLookupDirectory;
  QString cacheLocation;
  QString iconDirectory;

  size_t onlineTileCacheSize;
  size_t offlineTileCacheSize;

  QString styleSheetDirectory;
  bool styleSheetDirectoryConfigured;

  QString styleSheetFile;
  bool styleSheetFileConfigured;

  QString appName;
  QString appVersion;

public:
  OSMScoutQtBuilder();

  virtual ~OSMScoutQtBuilder();

  inline OSMScoutQtBuilder& WithSettingsStorage(QSettings *providedStorage)
  {
    this->settingsStorage=providedStorage;
    return *this;
  }

  inline OSMScoutQtBuilder& WithOnlineTileProviders(QString onlineTileProviders)
  {
    this->onlineTileProviders=onlineTileProviders;
    return *this;
  }

  inline OSMScoutQtBuilder& WithMapProviders(QString mapProviders)
  {
    this->mapProviders=mapProviders;
    return *this;
  }

  inline OSMScoutQtBuilder& WithMapLookupDirectories(QStringList mapLookupDirectories)
  {
    this->mapLookupDirectories=mapLookupDirectories;
    return *this;
  }

  inline OSMScoutQtBuilder& WithBasemapLookupDirectory(QString basemapLookupDirectory)
  {
    this->basemapLookupDirectory=basemapLookupDirectory;
    return *this;
  }

  inline OSMScoutQtBuilder& AddMapLookupDirectories(QString mapLookupDirectory)
  {
    this->mapLookupDirectories << mapLookupDirectory;
    return *this;
  }

  inline OSMScoutQtBuilder& WithCacheLocation(QString cacheLocation)
  {
    this->cacheLocation=cacheLocation;
    return *this;
  }

  inline OSMScoutQtBuilder& WithIconDirectory(QString iconDirectory)
  {
    this->iconDirectory=iconDirectory;
    return *this;
  }

  inline OSMScoutQtBuilder& WithStyleSheetDirectory(QString styleSheetDirectory)
  {
    this->styleSheetDirectory=styleSheetDirectory;
    this->styleSheetDirectoryConfigured=true;
    return *this;
  }

  inline OSMScoutQtBuilder& WithStyleSheetFile(QString styleSheetFile)
  {
    this->styleSheetFile=styleSheetFile;
    this->styleSheetFileConfigured=true;
    return *this;
  }

  inline OSMScoutQtBuilder& WithTileCacheSizes(size_t onlineTileCacheSize,
                                               size_t offlineTileCacheSize){
    this->onlineTileCacheSize=onlineTileCacheSize;
    this->offlineTileCacheSize=offlineTileCacheSize;
    return *this;
  }

  inline OSMScoutQtBuilder& WithUserAgent(QString appName,
                                          QString appVersion){
    this->appName=appName;
    this->appVersion=appVersion;
    return *this;
  }

  bool Init();
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<OSMScoutQtBuilder> OSMScoutQtBuilderRef;

/**
 * \ingroup QtAPI
 */
enum RenderingType {
  PlaneRendering = 0,
  TiledRendering = 1
};

/**
 * \ingroup QtAPI
 *
 * Singleton that provides access to high level modules of OSMScout library.
 * On application start should be registered Qt types by static method \ref RegisterQmlTypes().
 * OSMScoutQt instance may be created by \ref NewInstance() and accessed
 * by \ref GetInstance() then.
 * To free resources should be called \ref FreeInstance()
 * before program exits.
 *
 * Example:
 * ```
 * OSMScoutQt::RegisterQmlTypes();
 *
 * bool success=OSMScoutQt::NewInstance()
 *      .WithBasemapLookupDirectory(basemapDir)
 *      .WithStyleSheetDirectory(stylesheetDir)
 *      .WithStyleSheetFile(stylesheetFileName)
 *      .WithIconDirectory(iconDirectory)
 *      .WithMapLookupDirectories(mapLookupDirectories)
 *      .WithOnlineTileProviders(":/resources/online-tile-providers.json")
 *      .WithMapProviders(":/resources/map-providers.json")
 *      .Init();
 *
 * if (!success){
 *   // terminate program, or just report error - something is really bad
 * }
 *
 * // now it is possible to access OSMScoutQt by OSMScoutQt::GetInstance()
 *
 * OSMScoutQt::FreeInstance();
 * ```
 */
class OSMSCOUT_CLIENT_QT_API OSMScoutQt{
  friend class OSMScoutQtBuilder;

private:
  QThread       *backgroundThread;
  SettingsRef   settings;
  MapManagerRef mapManager;
  DBThreadRef   dbThread;
  QString       iconDirectory;
  QString       cacheLocation;
  size_t        onlineTileCacheSize;
  size_t        offlineTileCacheSize;
  QString       userAgent;

private:
  OSMScoutQt(SettingsRef settings,
             MapManagerRef mapManager,
             DBThreadRef dbThread,
             QString iconDirectory,
             QString cacheLocation,
             size_t onlineTileCacheSize,
             size_t offlineTileCacheSize,
             QString userAgent);
public:
  virtual ~OSMScoutQt();

  DBThreadRef GetDBThread() const;
  SettingsRef GetSettings() const;
  MapManagerRef GetMapManager() const;

  LookupModule* MakeLookupModule();
  MapRenderer* MakeMapRenderer(RenderingType type);
  Router* MakeRouter();
  SearchModule *MakeSearchModule();
  StyleModule *MakeStyleModule();

  QString GetUserAgent();

  static void RegisterQmlTypes(const char *uri="net.sf.libosmscout.map",
                               int versionMajor=1,
                               int versionMinor=0);

  static OSMScoutQtBuilder NewInstance();
  static OSMScoutQt& GetInstance();
  static void FreeInstance();
};

#endif /* OSMSCOUT_CLIENT_QT_OSMSCOUTQT_H */

