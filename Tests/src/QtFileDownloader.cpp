/*
  ClientQtThreading - a test program for libosmscout
  Copyright (C) 2019  Lukas Karas

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

#include <DownloaderTest.h>

#include <osmscout/util/Logger.h>
#include <osmscout/util/CmdLineParsing.h>

#include <osmscoutclientqt/OSMScoutQt.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QScreen>
#include <QtGui>
#include <QDebug>

#include <iostream>

using namespace osmscout;

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  CmdLineParser argParser("QtFileDownloader", argc, argv);
  std::vector<std::string> helpArgs{"h", "help"};
  Arguments args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool &value) {
                        args.help = value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineUIntOption([&args](const uint64_t &value) {
                        args.from = value;
                      }),
                      "from",
                      "Start downloading from given byte",
                      false);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string &value) {
                            args.url = QString::fromStdString(value);
                          }),
                          "URL",
                          "Url for download");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string &value) {
                            args.destination = QString::fromStdString(value);
                          }),
                          "FILE",
                          "Local file for output");

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

  osmscout::log.Info(true);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);
  osmscout::log.Debug(true);

  // register OSMScout library QML types
  OSMScoutQt::RegisterQmlTypes();

  bool initialised = OSMScoutQt::NewInstance()
      .WithUserAgent("QtDownloadingTest", "v?")
      .Init();

  if (!initialised){
    std::cerr << "Cannot initialize OSMScout library" << std::endl;
    return 1;
  }

  int result;
  {
    DownloaderTest test(args);
    test.connect(&test, &DownloaderTest::finished, &app, &QApplication::quit);
    result = app.exec();
    result += test.result();
  }
  OSMScoutQt::FreeInstance();

  if (result==0){
    std::cout << "Success!" << std::endl;
  }else{
    std::cerr << "Failure!" << std::endl;
  }
  return result;
}