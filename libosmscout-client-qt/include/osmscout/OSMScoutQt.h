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

#ifndef QTOSMSCOUT_H
#define QTOSMSCOUT_H

#include <QSettings>

#include <osmscout/DataTileCache.h>
#include <osmscout/DBThread.h>
#include <osmscout/LookupModule.h>

#include <osmscout/private/ClientQtImportExport.h>

class OSMScoutQt;
Q_DECLARE_METATYPE(osmscout::TileRef)

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API OSMScoutQtBuilder{
private:
  QSettings *settingsStorage;

  QString onlineTileProviders;
  QString mapProviders;
  QStringList mapLookupDirectories;
  QString cacheLocation;
  QString iconDirectory;

  size_t onlineTileCacheSize;
  size_t offlineTileCacheSize;

  QString styleSheetDirectory;
  bool styleSheetDirectoryConfigured;

  QString styleSheetFile;
  bool styleSheetFileConfigured;

public:
  OSMScoutQtBuilder();
 
  virtual ~OSMScoutQtBuilder();

  inline OSMScoutQtBuilder& WithSettingsStorage(QSettings *providedStorage)
  {
    this->settingsStorage=providedStorage;
    return *this;
  };

  inline OSMScoutQtBuilder& WithOnlineTileProviders(QString onlineTileProviders)
  {
    this->onlineTileProviders=onlineTileProviders;
    return *this;
  };

  inline OSMScoutQtBuilder& WithMapProviders(QString mapProviders)
  {
    this->mapProviders=mapProviders;
    return *this;
  };

  inline OSMScoutQtBuilder& WithMapLookupDirectories(QStringList mapLookupDirectories)
  {
    this->mapLookupDirectories=mapLookupDirectories;
    return *this;
  };

  inline OSMScoutQtBuilder& AddMapLookupDirectories(QString mapLookupDirectory)
  {
    this->mapLookupDirectories << mapLookupDirectory;
    return *this;
  };

  inline OSMScoutQtBuilder& WithCacheLocation(QString cacheLocation)
  {
    this->cacheLocation=cacheLocation;
    return *this;
  };

  inline OSMScoutQtBuilder& WithIconDirectory(QString iconDirectory)
  {
    this->iconDirectory=iconDirectory;
    return *this;
  };

  inline OSMScoutQtBuilder& WithStyleSheetDirectory(QString styleSheetDirectory)
  {
    this->styleSheetDirectory=styleSheetDirectory;
    this->styleSheetDirectoryConfigured=true;
    return *this;
  };

  inline OSMScoutQtBuilder& WithStyleSheetFile(QString styleSheetFile)
  {
    this->styleSheetFile=styleSheetFile;
    this->styleSheetFileConfigured=true;
    return *this;
  };

  inline OSMScoutQtBuilder& WithTileCacheSizes(size_t onlineTileCacheSize,
                                               size_t offlineTileCacheSize){
    this->onlineTileCacheSize=onlineTileCacheSize;
    this->offlineTileCacheSize=offlineTileCacheSize;
    return *this;
  }

  bool Init(bool tiledInstance=false);
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<OSMScoutQtBuilder> OSMScoutQtBuilderRef;

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API OSMScoutQt{
  friend class OSMScoutQtBuilder;

private:
  QThread *backgroundThread;
  SettingsRef settings;
  DBThreadRef dbThread;

private:
  OSMScoutQt(QThread *backgroundThread,
             SettingsRef settings,
             DBThreadRef dbThread);
public:
  virtual ~OSMScoutQt();

  DBThreadRef GetDBThread();
  SettingsRef GetSettings();
  LookupModuleRef MakeLookupModule();

  static void RegisterQmlTypes(const char *uri="net.sf.libosmscout.map",
                               int versionMajor=1,
                               int versionMinor=0);

  static OSMScoutQtBuilder NewInstance();
  static OSMScoutQt& GetInstance();
  static void FreeInstance();
};

#endif /* QTOSMSCOUT_H */

