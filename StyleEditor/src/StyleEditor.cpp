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

#include <osmscout/log/Logger.h>

// Qt includes
#include <QGuiApplication>
#include <QQuickView>
#include <QApplication>
#include <QStandardPaths>

// OSMScout library
#include <osmscoutclient/Settings.h>
#include <osmscoutclientqt/OSMScoutQt.h>

// Main Window
#include <MainWindow.h>

// Custom QML objects
#include <DocumentHandler.h>

using namespace osmscout;

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
  qmlRegisterType<DocumentHandler, 1>("DocumentHandler", 1, 0, "DocumentHandler");

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
    builder.WithStyleSheetDirectory(stylesheetFile.dir().absolutePath())
     .WithStyleSheetFile(stylesheetFile.fileName());
  }

  if (cmdLineArgs.size() > 3){
    QFileInfo iconDirectory(cmdLineArgs.at(3));
    builder.WithIconDirectory(iconDirectory.absoluteFilePath());
  }else{
    if (cmdLineArgs.size() > 1){
      QFileInfo iconDirectory(cmdLineArgs.at(1) + QDir::separator() + "icons");
      builder.WithIconDirectory(iconDirectory.absoluteFilePath());
    }else{
      QFileInfo iconDirectory(QDir::currentPath() + QDir::separator() + "icons");
      builder.WithIconDirectory(iconDirectory.absoluteFilePath());
    }
  }

  builder
    .WithMapLookupDirectories(mapLookupDirectories)
    .AddOnlineTileProviders(":/resources/online-tile-providers.json")
    .WithUserAgent("OSMScoutStyleEditor", "v2");

  if (!builder.Init()){
    osmscout::log.Error() << "Cannot initialize OSMScout library";
    return 1;
  }


  {
    DBThreadRef dbThread=OSMScoutQt::GetInstance().GetDBThread();

    MainWindow window(dbThread);

    result = app.exec();

    QString tmpStylesheet = QString::fromStdString(dbThread->GetStylesheetFilename()).append(DocumentHandler::tmpSuffix());
    if(QFile::exists(tmpStylesheet)){
      QFile::remove(tmpStylesheet);
    }
  }

  OSMScoutQt::FreeInstance();

  return result;
}

#if defined(__WIN32__) || defined(WIN32)
int CALLBACK WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/){
        main(0, nullptr);

  return 0;
}
#endif
