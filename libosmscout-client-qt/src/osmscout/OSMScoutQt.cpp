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

#include <QMetaType>
#include <QQmlEngine>

#include <osmscout/OSMScoutQt.h>
#include <osmscout/Settings.h>
#include <osmscout/DBThread.h>
#include <osmscout/DataTileCache.h>
#include <osmscout/PlaneDBThread.h>
#include <osmscout/TiledDBThread.h>
#include <osmscout/MapWidget.h>

#include <osmscout/AvailableMapsModel.h>
#include <osmscout/LocationInfoModel.h>
#include <osmscout/MapDownloadsModel.h>
#include <osmscout/MapObjectInfoModel.h>
#include <osmscout/MapStyleModel.h>
#include <osmscout/OnlineTileProviderModel.h>
#include <osmscout/RoutingModel.h>
#include <osmscout/SearchLocationModel.h>
#include <osmscout/StyleFlagsModel.h>

static OSMScoutQt* osmScoutInstance=NULL;

OSMScoutQtBuilder::OSMScoutQtBuilder():
  settingsStorage(NULL),
  onlineTileCacheSize(100),
  offlineTileCacheSize(200),
  styleSheetDirectoryConfigured(false),
  styleSheetFileConfigured(false)
{
  QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  mapLookupDirectories << QDir::currentPath();
  mapLookupDirectories << QDir(documentsLocation).filePath("Maps");

  cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

  OSMScoutQtBuilder::~OSMScoutQtBuilder()
{
}

bool OSMScoutQtBuilder::Init(bool tiledInstance)
{
  if (osmScoutInstance!=NULL){
    return false;
  }

  SettingsRef settings=std::make_shared<Settings>(settingsStorage);

  // load online tile providers
  if (!onlineTileProviders.isEmpty()){
    settings->loadOnlineTileProviders(onlineTileProviders);
  }
  if (!mapProviders.isEmpty()){
    settings->loadMapProviders(mapProviders);
  }

  // setup style sheet
  if (styleSheetFileConfigured){
    settings->SetStyleSheetFile(styleSheetFile);
  }
  if (styleSheetDirectoryConfigured){
    settings->SetStyleSheetDirectory(styleSheetDirectory);
  }

  DBThread* dbThread;
  if (tiledInstance){
    dbThread=new TiledDBThread(mapLookupDirectories,
                               iconDirectory,
                               settings,
                               cacheLocation + QDir::separator() + "OSMScoutTileCache",
                               onlineTileCacheSize,
                               offlineTileCacheSize);
  }else{
    dbThread=new PlaneDBThread(mapLookupDirectories,
                               iconDirectory,
                               settings);
  }

  QThread *thread=new QThread();
  thread->setObjectName("DBThread");

  dbThread->connect(thread, SIGNAL(started()), SLOT(Initialize()));
  dbThread->connect(thread, SIGNAL(finished()), SLOT(Finalize()));

  dbThread->moveToThread(thread);
  thread->start();

  osmScoutInstance=new OSMScoutQt(thread,
                                  settings,
                                  std::shared_ptr<DBThread>(dbThread));

  return true;
}

void OSMScoutQt::RegisterQmlTypes(const char *uri,
                                  int versionMajor,
                                  int versionMinor)
{
  qRegisterMetaType<RenderMapRequest>();
  qRegisterMetaType<DatabaseLoadedResponse>();
  qRegisterMetaType<osmscout::TileRef>();

  qmlRegisterType<AvailableMapsModel>(uri, versionMajor, versionMinor, "AvailableMapsModel");
  qmlRegisterType<LocationEntry>(uri, versionMajor, versionMinor, "LocationEntry");
  qmlRegisterType<LocationInfoModel>(uri, versionMajor, versionMinor, "LocationInfoModel");
  qmlRegisterType<LocationListModel>(uri, versionMajor, versionMinor, "LocationListModel");
  qmlRegisterType<MapDownloadsModel>(uri, versionMajor, versionMinor, "MapDownloadsModel");
  qmlRegisterType<MapObjectInfoModel>(uri, versionMajor, versionMinor, "MapObjectInfoModel");
  qmlRegisterType<MapStyleModel>(uri, versionMajor, versionMinor, "MapStyleModel");
  qmlRegisterType<MapWidget>(uri, versionMajor, versionMinor, "Map");
  qmlRegisterType<OnlineTileProviderModel>(uri, versionMajor, versionMinor, "OnlineTileProviderModel");
  qmlRegisterType<QmlSettings>(uri, versionMajor, versionMinor, "Settings");
  qmlRegisterType<RouteStep>(uri, versionMajor, versionMinor, "RouteStep");
  qmlRegisterType<RoutingListModel>(uri, versionMajor, versionMinor, "RoutingListModel");
  qmlRegisterType<StyleFlagsModel>(uri, versionMajor, versionMinor, "StyleFlagsModel");
}

OSMScoutQtBuilder OSMScoutQt::NewInstance()
{
  return OSMScoutQtBuilder();
}

OSMScoutQt& OSMScoutQt::GetInstance()
{
  return *osmScoutInstance;
}

void OSMScoutQt::FreeInstance()
{
  delete osmScoutInstance;
  osmScoutInstance=NULL;
}

OSMScoutQt::OSMScoutQt(QThread *backgroundThread,
                       SettingsRef settings,
                       DBThreadRef dbThread):
        backgroundThread(backgroundThread),
        settings(settings),
        dbThread(dbThread)
{
}

OSMScoutQt::~OSMScoutQt()
{
  backgroundThread->quit();
  backgroundThread->wait();
  delete backgroundThread;
}

DBThreadRef OSMScoutQt::GetDBThread()
{
  return dbThread;
}

SettingsRef OSMScoutQt::GetSettings()
{
  return settings;
}

LookupModuleRef OSMScoutQt::MakeLookupModule()
{
  QThread *thread=new QThread();
  thread->setObjectName("LookupModule");
  LookupModuleRef module=std::make_shared<LookupModule>(thread,dbThread);
  module->moveToThread(thread);
  thread->start();
  return module;
}
