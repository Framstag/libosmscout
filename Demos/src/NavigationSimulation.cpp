/*
  NavigationSimulation - a demo program for libosmscout
  Copyright (C) 2021 Lukas Karas

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
#include <PositionSimulator.h>
#include <Theme.h>

#include <QDebug>
#include <QFileInfo>
#include <QMetaType>
#include <QQmlEngine>
#include <QQmlApplicationEngine>

#include <iostream>

static QObject *ThemeProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    Theme *theme = new Theme();

    return theme;
}

class NavSimArgs: public QtDemoApp::Arguments {
public:
  NavSimArgs() = default;
  ~NavSimArgs() override = default;

  void AddOptions(osmscout::CmdLineParser &argParser) override
  {
    QtDemoApp::Arguments::AddOptions(argParser);

    using namespace std::string_literals;

    argParser.AddOption(osmscout::CmdLineStringOption([this](const std::string& value) {
                          vehicle=QString::fromStdString(value);
                        }),
                        "vehicle",
                        "Used vehicle (car, foot, bicycle). Default is "s + vehicle.toStdString(),
                        false);

    argParser.AddPositional(osmscout::CmdLineStringOption([this](const std::string& value) {
                          track=QString::fromStdString(value);
                        }),
                        "simulate-track",
                        "GPX file for navigation simulation");
  }

public:
  QString track;
  QString vehicle="car";
};

class NavigationSimulation: public QtDemoApp {
public:
  NavigationSimulation(int &argc, char* argv[]):
      QtDemoApp("NavigationSimulation", argc, argv)
  {}

  ~NavigationSimulation() override = default;

  void SetupQmlContext(QQmlContext *context, const Arguments &args) override
  {
    qmlRegisterType<PositionSimulator>("net.sf.libosmscout.map", 1, 0, "PositionSimulator");
    qmlRegisterSingletonType<Theme>("net.sf.libosmscout.map", 1, 0, "Theme", ThemeProvider);

    QtDemoApp::SetupQmlContext(context, args);
    assert(context);
    const NavSimArgs &eleArgs = static_cast<const NavSimArgs&>(args);
    context->setContextProperty("routeVehicle", eleArgs.vehicle);
    context->setContextProperty("PositionSimulationTrack", eleArgs.track);
  }
};

int main(int argc, char* argv[])
{
  NavigationSimulation app(argc, argv);

  NavSimArgs args;
  int result=0;
  if (!args.Parse(argc, argv, result)){
    return result;
  }

  QFileInfo gpxFile(args.track);
  if (!gpxFile.exists()){
    std::cerr << args.track.toStdString() << " don't exists" << std::endl;
    return 1;
  }

  return app.Run(args, QUrl("qrc:/qml/NavigationSimulation.qml"));
}
