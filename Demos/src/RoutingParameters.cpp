/*
  RoutingParameters - a demo program for libosmscout
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
#include <Theme.h>

#include <QDebug>
#include <QFileInfo>
#include <QMetaType>
#include <QQmlEngine>

static QObject *ThemeProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)

  Theme *theme = new Theme();

  return theme;
}

class RoutingParamsArgs: public QtDemoApp::Arguments {
public:
  RoutingParamsArgs() = default;
  ~RoutingParamsArgs() override = default;
};

class RoutingParameters: public QtDemoApp {
public:
  RoutingParameters(int &argc, char* argv[]):
      QtDemoApp("RoutingParameters", argc, argv)
  {}

  ~RoutingParameters() override = default;

  void SetupQmlContext(QQmlContext *context, const Arguments &args) override
  {
    QtDemoApp::SetupQmlContext(context, args);
    assert(context);

    qmlRegisterSingletonType<Theme>("net.sf.libosmscout.map", 1, 0, "Theme", ThemeProvider);
  }
};

int main(int argc, char* argv[])
{
  RoutingParameters app( argc, argv);

  RoutingParamsArgs args;
  int result=0;
  if (!args.Parse(argc, argv, result)){
    return result;
  }

  return app.Run(args, QUrl("qrc:/qml/RoutingParameters.qml"));
}
