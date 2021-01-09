/*
  ElevationProfileChart - a demo program for libosmscout
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

#include <QDebug>

class EleChartArgs: public QtDemoApp::Arguments {
public:
  EleChartArgs() = default;
  ~EleChartArgs() override = default;

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
                              routeFrom=QString::fromStdString(value);
                            }),
                            "routeFrom",
                            "Route from");

    argParser.AddPositional(osmscout::CmdLineStringOption([this](const std::string& value) {
                              routeTo=QString::fromStdString(value);
                            }),
                            "routeTo",
                            "Route to");
  }

public:
  QString routeFrom;
  QString routeTo;
  QString vehicle="car";
};

class ElevationProfileChart: public QtDemoApp {
public:
  ElevationProfileChart(int &argc, char* argv[]):
      QtDemoApp("ElevationProfileChart", argc, argv)
  {}

  ~ElevationProfileChart() override = default;

  void SetupQmlContext(QQmlContext *context, const Arguments &args) override
  {
    QtDemoApp::SetupQmlContext(context, args);
    assert(context);
    const EleChartArgs &eleArgs = static_cast<const EleChartArgs&>(args);
    context->setContextProperty("routeFrom", eleArgs.routeFrom);
    context->setContextProperty("routeTo", eleArgs.routeTo);
    context->setContextProperty("routeVehicle", eleArgs.vehicle);
  }
};

int main(int argc, char* argv[])
{
  ElevationProfileChart app( argc, argv);

  EleChartArgs args;
  int result=0;
  if (!args.Parse(argc, argv, result)){
    return result;
  }

  return app.Run(args, QUrl("qrc:/qml/ElevationProfileChart.qml"));
}
