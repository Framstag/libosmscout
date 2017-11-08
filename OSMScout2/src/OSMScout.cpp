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

#include <iostream>

// Qt includes
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QApplication>
#include <QFileInfo>

// OSM Scout library singleton
#include <osmscout/OSMScoutQt.h>

// Custom QML objects
#include <osmscout/MapWidget.h>
#include <osmscout/SearchLocationModel.h>
#include <osmscout/RoutingModel.h>
#include <osmscout/AvailableMapsModel.h>
#include <osmscout/MapDownloadsModel.h>

// Application settings
#include <osmscout/Settings.h>

// Application theming
#include "Theme.h"

#include "AppSettings.h"

#include <osmscout/util/Logger.h>

static QObject *ThemeProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    Theme *theme = new Theme();

    return theme;
}

#define DEBUG   4
#define INFO    3
#define WARNING 2
#define ERROR   1

static int LogEnv(QString env)
{
  if (env.toUpper()=="DEBUG") {
    return DEBUG;
  }
  else if (env.toUpper()=="INFO") {
    return INFO;
  }
  else if (env.toUpper()=="WARNING") {
    return INFO;
  }
  else if (env.toUpper()=="ERROR") {
    return ERROR;
  }

  return WARNING;
}

int main(int argc, char* argv[])
{
#ifdef Q_WS_X11
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

  QGuiApplication app(argc,argv);
  int             result;

  app.setOrganizationName("libosmscout");
  app.setOrganizationDomain("libosmscout.sf.net");
  app.setApplicationName("OSMScout");

  // register OSMScout library QML types
  OSMScoutQt::RegisterQmlTypes();

  // register application QML types
  qmlRegisterType<AppSettings>("net.sf.libosmscout.map", 1, 0, "AppSettings");
  qmlRegisterSingletonType<Theme>("net.sf.libosmscout.map", 1, 0, "Theme", ThemeProvider);

  // init logger by system system variable
  QString logLevelName=QProcessEnvironment::systemEnvironment().value("OSMSCOUT_LOG", "WARNING");

  std::cout << "Setting libosmscout logging to level: " << logLevelName.toStdString() << std::endl;

  int logEnv=LogEnv(logLevelName);

  osmscout::log.Debug(logEnv>=DEBUG);
  osmscout::log.Info(logEnv>=INFO);
  osmscout::log.Warn(logEnv>=WARNING);
  osmscout::log.Error(logEnv>=ERROR);

  OSMScoutQtBuilder builder=OSMScoutQt::NewInstance();

  // setup paths
  QStringList cmdLineArgs = QApplication::arguments();

  QStringList mapLookupDirectories;
  if (cmdLineArgs.size() > 1){
    mapLookupDirectories << cmdLineArgs.at(1);

    QDir dir(cmdLineArgs.at(1));

    if (dir.cdUp()) {
      if (dir.cd("world")) {
        builder.WithBasemapLookupDirectory(dir.absolutePath());
      }
    }
  }

  if (cmdLineArgs.size() > 2){
    QFileInfo stylesheetFile(cmdLineArgs.at(2));
    builder.WithStyleSheetDirectory(stylesheetFile.dir().path())
     .WithStyleSheetFile(stylesheetFile.fileName());
  }

  QString iconDirectory;
  if (cmdLineArgs.size() > 3){
    iconDirectory = cmdLineArgs.at(3);
  }
  else {
    if (cmdLineArgs.size() > 1) {
      iconDirectory = QDir(cmdLineArgs.at(1)).filePath("icons");
    }
    else {
      iconDirectory = "icons";
    }
  }
  builder
    .WithIconDirectory(iconDirectory)
    .WithMapLookupDirectories(mapLookupDirectories)
    .WithOnlineTileProviders(":/resources/online-tile-providers.json")
    .WithMapProviders(":/resources/map-providers.json")
    .WithUserAgent("OSMScout2DemoApp", "v?");

  if (!builder.Init()){
    std::cerr << "Cannot initialize OSMScout library" << std::endl;
    return 1;
  }

  {
    QQmlApplicationEngine window(QUrl("qrc:/qml/main.qml"));
    result = app.exec();
  }

  OSMScoutQt::FreeInstance();

  return result;
}
