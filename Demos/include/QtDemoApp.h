#ifndef DEMO_LIBOSMSCOUT_QTDEMOAPP_H
#define DEMO_LIBOSMSCOUT_QTDEMOAPP_H

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

#include <osmscout/cli/CmdLineParsing.h>

#include <QStringList>
#include <QGuiApplication>
#include <QUrl>
#include <QQmlContext>

class QtDemoApp {
public:

  class Arguments {
  public:
    virtual ~Arguments() = default;

    virtual void AddOptions(osmscout::CmdLineParser &argParser);

    bool Parse(int argc, char* argv[], int &exitCode);

  public:
    bool help=false;

#ifdef NDEBUG
    bool debug=false;
#else
    bool debug=true;
#endif

    QStringList mapLookupDirectories;
    QString stylesheet="stylesheets/standard.oss";
    QString iconDirectory="icons";
    QString translationDir;
    QString basemapDir;
  };

public:
  /**
   * @param appName
   *
   * @param argc - be aware that this argument is reference!
   *    QGuiApplication may "consume" some arguments,
   *    like "--style", "--plugin" (see QGuiApplicationPrivate::init() source).
   *    So, these arguments cannot be used by demo application itself.
   *
   * @param argv
   */
  QtDemoApp(QString appName, int &argc, char* argv[]);

  QtDemoApp(const QtDemoApp &) = delete;
  QtDemoApp(const QtDemoApp &&) = delete;
  QtDemoApp &operator=(const QtDemoApp &) = delete;
  QtDemoApp &operator=(const QtDemoApp &&) = delete;

  virtual ~QtDemoApp() = default;

  int Run(const Arguments &args, const QUrl &qmlFileUrl);

  virtual void SetupQmlContext(QQmlContext *, const Arguments &)
  {
    // no-op
  };

private:
  QGuiApplication app;
};

#endif //DEMO_LIBOSMSCOUT_QTDEMOAPP_H
