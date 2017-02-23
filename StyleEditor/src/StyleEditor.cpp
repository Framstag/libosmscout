/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// Qt includes
#include <QGuiApplication>
#include <QQuickView>
#include <QApplication>

// Custom QML objects
#include "osmscout/MapWidget.h"
#include "osmscout/SearchLocationModel.h"
#include "osmscout/MapObjectInfoModel.h"
#include "FileIO.h"

// Application settings
#include "osmscout/Settings.h"

// Main Window
#include "MainWindow.h"

Q_DECLARE_METATYPE(osmscout::TileRef)

int main(int argc, char* argv[])
{
#ifdef Q_WS_X11
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

  QGuiApplication app(argc,argv);
  MainWindow      *window;
  int             result;

  app.setOrganizationName("libosmscout");
  app.setOrganizationDomain("libosmscout.sf.net");
  app.setApplicationName("StyleEditor");


  qRegisterMetaType<RenderMapRequest>();
  qRegisterMetaType<DatabaseLoadedResponse>();
  qRegisterMetaType<osmscout::TileRef>();

  qmlRegisterType<MapWidget>("net.sf.libosmscout.map", 1, 0, "Map");
  qmlRegisterType<LocationEntry>("net.sf.libosmscout.map", 1, 0, "Location");
  qmlRegisterType<LocationListModel>("net.sf.libosmscout.map", 1, 0, "LocationListModel");
  qmlRegisterType<MapObjectInfoModel>("net.sf.libosmscout.map", 1, 0, "MapObjectInfoModel");
  qmlRegisterType<FileIO, 1>("FileIO", 1, 0, "FileIO");
  qmlRegisterType<QmlSettings>("net.sf.libosmscout.map", 1, 0, "Settings");

  QThread thread;
  
  // load online tile providers
  SettingsRef settings=std::make_shared<Settings>();
  settings->loadOnlineTileProviders(
    ":/resources/online-tile-providers.json");

  // setup paths
  QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);  
  QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  QStringList cmdLineArgs = QApplication::arguments();
  
  QStringList mapLookupDirectories;
  if (cmdLineArgs.size() > 1){
    mapLookupDirectories << cmdLineArgs.at(1);
  }else{
    mapLookupDirectories << QDir::currentPath();
    mapLookupDirectories << documentsLocation + QDir::separator() + "Maps";
  }
  
  if (cmdLineArgs.size() > 2){
    QFileInfo stylesheetFile(cmdLineArgs.at(2));
    settings->SetStyleSheetDirectory(stylesheetFile.dir().path());
    settings->SetStyleSheetFile(stylesheetFile.fileName());
  }
  
  QString iconDirectory;
  if (cmdLineArgs.size() > 3){
    iconDirectory = cmdLineArgs.at(3);
  }else{
    if (cmdLineArgs.size() > 1){
      iconDirectory = cmdLineArgs.at(1) + "icons";
    }else{
      iconDirectory = "icons";
    }
  }

  if (!DBThread::InitializeTiledInstance(
          mapLookupDirectories,
          iconDirectory,
          settings,
          cacheLocation + QDir::separator() + "OSMScoutTileCache",
          /* onlineTileCacheSize  */ 100,
          /* offlineTileCacheSize */ 200
      )) {
    std::cerr << "Cannot initialize DBThread" << std::endl;
  }

  DBThread* dbThread=DBThread::GetInstance();

  window=new MainWindow(dbThread);

  dbThread->connect(&thread, SIGNAL(started()), SLOT(Initialize()));
  dbThread->connect(&thread, SIGNAL(finished()), SLOT(Finalize()));

  dbThread->moveToThread(&thread);
  thread.start();

  result=app.exec();

  delete window;

  QString tmpStylesheet(dbThread->GetStylesheetFilename()+TMP_SUFFIX);
  if(QFile::exists(tmpStylesheet)){
      QFile::remove(tmpStylesheet);
  }

  thread.quit();
  thread.wait();

  DBThread::FreeInstance();

  return result;
}

#if defined(__WIN32__) || defined(WIN32)
int CALLBACK WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/){
	main(0, NULL);
}
#endif
