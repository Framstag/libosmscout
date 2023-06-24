/*
  QtDemoApp - a part of demo programs for libosmscout
  Copyright (C) 2021  Lukas Karas

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

#include <QtDemoApp.h>

#include <osmscoutclientqt/OSMScoutQt.h>

#include <QGuiApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>
#include <QTranslator>

#include <iostream>

void QtDemoApp::Arguments::AddOptions(osmscout::CmdLineParser &argParser)
{
  std::vector<std::string>  helpArgs{"h","help"};

  argParser.AddOption(osmscout::CmdLineFlag([this](const bool& value) {
                        help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineFlag([this](const bool& value) {
                        debug=value;
                      }),
                      "debug",
                      "Enable debug output",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                        iconDirectory=QString::fromStdString(value);
                      }),
                      "icons",
                      "Icon directory",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                        stylesheet=QString::fromStdString(value);
                      }),
                      "stylesheet",
                      "Map style. Default: " + stylesheet.toStdString(),
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                        mapLookupDirectories.append(QString::fromStdString(value));
                      }),
                      "db",
                      "Map db directory",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                        basemapDir=QString::fromStdString(value);
                      }),
                      "basemap",
                      "World base map directory",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                        translationDir=QString::fromStdString(value);
                      }),
                      "translations",
                      "Directory with translation files (*.qm)",
                      false);
}

bool QtDemoApp::Arguments::Parse(int argc, char* argv[], int &exitCode)
{
  osmscout::CmdLineParser   argParser(QApplication::applicationDisplayName().toStdString(),
                                      argc,argv);

  AddOptions(argParser);
  osmscout::CmdLineParseResult argResult=argParser.Parse();

  QString documentsLocation = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  if (mapLookupDirectories.empty()){
    mapLookupDirectories << documentsLocation + QDir::separator() + "Maps";
  }
  if (basemapDir.isEmpty()){
    basemapDir = documentsLocation + QDir::separator() + "Maps" + QDir::separator() + "world";
  }

  if (argResult.HasError()) {
    std::cerr << "ERROR: " << argResult.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    exitCode = 1;
    return false;
  } else if (help) {
    std::cout << argParser.GetHelp() << std::endl;
    exitCode = 0;
    return false;
  }

  return true;
}

QtDemoApp::QtDemoApp(QString appName, int &argc, char* argv[]):
    app(argc, argv)
{
  QGuiApplication::setOrganizationName("libosmscout");
  QGuiApplication::setOrganizationDomain("libosmscout.sf.net");
  QGuiApplication::setApplicationName(appName);

  // setup c++ locale
  try {
    std::locale::global(std::locale(""));
  } catch (const std::runtime_error& e) {
    std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
  }

  // register OSMScout library QML types
  osmscout::OSMScoutQt::RegisterQmlTypes();
}

int QtDemoApp::Run(const Arguments &args, const QUrl &qmlFileUrl)
{
  osmscout::log.Debug(args.debug);
  osmscout::log.Info(true);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);

  // install translator
  QTranslator translator;
  QLocale locale;
  QString translationDir;
  if (args.translationDir.isEmpty()) {
    // translations are installed to <PREFIX>/share/libosmscout/OSMScout2/translations
    // Qt lookup app data (on Linux) in directories "~/.local/share/<APPNAME>", "/usr/local/share/<APPNAME>", "/usr/share/<APPNAME>"
    // when APPNAME is combination of <organisation>/<app name>
    translationDir = QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, "translations",
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

  QFileInfo stylesheetFile(args.stylesheet);

  osmscout::OSMScoutQtBuilder builder=osmscout::OSMScoutQt::NewInstance();

  builder
      .WithStyleSheetDirectory(stylesheetFile.dir().path())
      .WithStyleSheetFile(stylesheetFile.fileName())
      .WithIconDirectory(args.iconDirectory)
      .WithMapLookupDirectories(args.mapLookupDirectories)
      .WithBasemapLookupDirectory(args.basemapDir)
      .AddOnlineTileProviders(":/resources/online-tile-providers.json")
      .AddMapProviders(":/resources/map-providers.json")
      .AddVoiceProviders(":/resources/voice-providers.json")
      .WithUserAgent(QApplication::applicationName(), QApplication::applicationVersion());

  if (!builder.Init()){
    std::cerr << "Cannot initialize OSMScout library" << std::endl;
    return 1;
  }

  int result;
  {
    QQmlApplicationEngine window;
    SetupQmlContext(window.rootContext(), args);
    window.load(qmlFileUrl);
    result = QGuiApplication::exec();
  }

  osmscout::OSMScoutQt::FreeInstance();

  return result;
}
