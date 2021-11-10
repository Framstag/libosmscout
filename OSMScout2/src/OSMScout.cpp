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
#include <QApplication>
#include <QFileInfo>
#include <QQmlContext>

// OSM Scout library singleton
#include <osmscoutclientqt/OSMScoutQt.h>

// Application theming
#include "Theme.h"

#include "AppSettings.h"

#include <osmscout/util/Logger.h>
#include <osmscout/util/CmdLineParsing.h>

using namespace osmscout;

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

static int LogEnv(const QString& env)
{
  if (env.toUpper()=="DEBUG") {
    return DEBUG;
  }

  if (env.toUpper()=="INFO") {
    return INFO;
  }

  if (env.toUpper()=="WARNING") {
    return INFO;
  }

  if (env.toUpper()=="ERROR") {
    return ERROR;
  }

  return WARNING;
}

struct Arguments {
  bool    help=false;
  QString databaseDirectory=".";
  QString style="stylesheets/standard.oss";
  QString iconDirectory="icons";
  QString translationDir;
};

int main(int argc, char* argv[])
{
#ifdef Q_WS_X11
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

  QGuiApplication app(argc,argv);
  int             result;

  QGuiApplication::setOrganizationName("libosmscout");
  QGuiApplication::setOrganizationDomain("libosmscout.sf.net");
  QGuiApplication::setApplicationName("OSMScout2");

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

  osmscout::CmdLineParser   argParser("OSMScout2",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.iconDirectory=QString::fromStdString(value);
                      }),
                      "icons",
                      "Icon directory",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.translationDir=QString::fromStdString(value);
                      }),
                      "translations",
                      "Directory with translation files (*.qm)",
                      false);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=QString::fromStdString(value);
                          }),
                          "databaseDir",
                          "Database directory");
  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.style=QString::fromStdString(value);
                          }),
                          "stylesheet",
                          "Map stylesheet");

  osmscout::CmdLineParseResult argResult=argParser.Parse();
  if (argResult.HasError()) {
    std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  // setup c++ locale
  try {
    std::locale::global(std::locale(""));
  } catch (const std::runtime_error& e) {
    std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
  }

  // install translator
  QTranslator translator;
  QLocale locale;
  QString translationDir;
  if (args.translationDir.isEmpty()) {
    // translations are installed to <PREFIX>/share/libosmscout/OSMScout2/translations
    // Qt lookup app data (on Linux) in directories "~/.local/share/<APPNAME>", "/usr/local/share/<APPNAME>", "/usr/share/<APPNAME>"
    // when APPNAME is combination of <organisation>/<app name>
    translationDir = QStandardPaths::locate(QStandardPaths::DataLocation, "translations",
                                            QStandardPaths::LocateDirectory);
  }else{
    translationDir = args.translationDir;
  }
  if (translator.load(locale.name(), translationDir)) {
    qDebug() << "Install translator for locale " << locale << "/" << locale.name();
    QGuiApplication::installTranslator(&translator);
  }else{
    qWarning() << "Can't load translator for locale" << locale << "/" << locale.name() <<
               "(" << translationDir << ")";
  }

  QStringList mapLookupDirectories;

  mapLookupDirectories << args.databaseDirectory;

  QDir dir(args.databaseDirectory);

  if (dir.cdUp()) {
    if (dir.cd("world")) {
      builder.WithBasemapLookupDirectory(dir.absolutePath());
    }
  }

  QFileInfo stylesheetFile(args.style);

  builder
    .WithStyleSheetDirectory(stylesheetFile.dir().path())
    .WithStyleSheetFile(stylesheetFile.fileName())
    .WithIconDirectory(args.iconDirectory)
    .WithMapLookupDirectories(mapLookupDirectories)
    .WithOnlineTileProviders(":/resources/online-tile-providers.json")
    .WithMapProviders(":/resources/map-providers.json")
    .WithVoiceProviders(":/resources/voice-providers.json")
    .WithUserAgent("OSMScout2DemoApp", "v?");

  if (!builder.Init()){
    std::cerr << "Cannot initialize OSMScout library" << std::endl;
    return 1;
  }

  {
    QQmlApplicationEngine window(QUrl("qrc:/qml/main.qml"));
    result = QGuiApplication::exec();
  }

  OSMScoutQt::FreeInstance();

  return result;
}
