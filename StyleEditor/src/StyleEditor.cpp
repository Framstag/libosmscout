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

#include <osmscout/util/Logger.h>

// Qt includes
#include <QGuiApplication>
#include <QQuickView>
#include <QApplication>

// OSMScout library
#include <osmscout/Settings.h>
#include <osmscout/OSMScoutQt.h>

// Main Window
#include "MainWindow.h"

// Custom QML objects
#include "FileIO.h"

int main(int argc, char* argv[])
{
#ifdef Q_WS_X11
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

  QGuiApplication app(argc,argv);
  int             result;

  app.setOrganizationName("libosmscout");
  app.setOrganizationDomain("libosmscout.sf.net");
  app.setApplicationName("StyleEditor");

  // register OSMScout library QML types
  OSMScoutQt::RegisterQmlTypes();

  qRegisterMetaType<QSet<int>>("QSet<int>");
  qmlRegisterType<FileIO, 1>("FileIO", 1, 0, "FileIO");

  OSMScoutQtBuilder builder=OSMScoutQt::NewInstance();

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
    builder.WithStyleSheetDirectory(stylesheetFile.dir().path())
     .WithStyleSheetFile(stylesheetFile.fileName());
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

  builder
    .WithIconDirectory(iconDirectory)
    .WithMapLookupDirectories(mapLookupDirectories)
    .WithOnlineTileProviders(":/resources/online-tile-providers.json")
    .WithUserAgent("OSMScoutStyleEditor", "v?");

  if (!builder.Init()){
    osmscout::log.Error() << "Cannot initialize OSMScout library";
    return 1;
  }


  {
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();

    MainWindow window(dbThread);
    result = app.exec();

    QString tmpStylesheet(dbThread->GetStylesheetFilename()+TMP_SUFFIX);
    if(QFile::exists(tmpStylesheet)){
      QFile::remove(tmpStylesheet);
    }
  }

  OSMScoutQt::FreeInstance();

  return result;
}

#if defined(__WIN32__) || defined(WIN32)
int CALLBACK WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/){
	main(0, NULL);
}
#endif
