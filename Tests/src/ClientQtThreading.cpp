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

#include <ClientQtThreading.h>

#include <osmscout/log/Logger.h>
#include <osmscout/cli/CmdLineParsing.h>

#include <osmscoutclientqt/OSMScoutQt.h>

#include <QApplication>
#include <QPixmap>
#include <QScreen>
#include <QtGui>
#include <QDebug>

#include <iostream>

using namespace osmscout;

struct Arguments
{
  bool                help{false};
  QString             databaseDirectory;
  QString             stylesheetDirectory;
  osmscout::GeoCoord  renderingCenter;
  size_t              magLevel{0};
};

ThreadingTest::ThreadingTest(const QList<QFileInfo> &stylesheets,
                             const osmscout::GeoCoord &renderingCenter,
                             const size_t &magLevel):
    QObject(nullptr),
    stylesheets(stylesheets),
    renderingCenter(renderingCenter),
    magLevel(magLevel),
    styleModule(OSMScoutQt::GetInstance().MakeStyleModule()),
    dbThread(OSMScoutQt::GetInstance().GetDBThread())
{
  connect(this, &ThreadingTest::loadStyleRequested,
        styleModule, &StyleModule::loadStyle,
        Qt::QueuedConnection);

  timer.setSingleShot(true);
  connect(&timer, &QTimer::timeout, this, &ThreadingTest::test);
  timer.start();
}

ThreadingTest::~ThreadingTest()
{
  styleModule->deleteLater();
}

void ThreadingTest::test()
{
  osmscout::MercatorProjection projection;
  // TODO: use coordinate from argument
  projection.Set(renderingCenter,
                 /*currentAngle*/ 0.0,
                 Magnification(MagnificationLevel(magLevel)),
                 /* dpi */ 160.0,
                 /* width */ 1024,
                 /* height */ 1024);

  loadJob=new DBLoadJob(projection,
                        /* maximumAreaLevel*/ 6,
                        /* lowZoomOptimization */ true,
                        /* closeOnFinish */ false);

  connect(loadJob, &DBLoadJob::finished,
          this, &ThreadingTest::onLoadJobFinished);

  dbThread->RunJob(loadJob);
}

void ThreadingTest::onLoadJobFinished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles)
{
  loadJob->deleteLater();
  loadJob=nullptr;
  size_t objectCount=0;
  for (const auto &dbPath:tiles.keys()){
    const auto &map=tiles[dbPath];

    for (const auto &tileKey:map.keys()){
      const auto &tile=map[tileKey];
      std::cout << "Stylesheet: " << dbThread->GetStylesheetFilename().toStdString()
                << ", " << dbPath.toStdString()
                << ", " << tileKey.GetDisplayText()
                << " object count: " << tile->GetAreaData().GetDataSize()
                << " / " << tile->GetWayData().GetDataSize()
                << " / " << tile->GetNodeData().GetDataSize()
                << std::endl;

      objectCount += tile->GetAreaData().GetDataSize() + tile->GetWayData().GetDataSize() + tile->GetNodeData().GetDataSize();
    }
  }
  std::cout << "Stylesheet: " << dbThread->GetStylesheetFilename().toStdString() << ", sum object count: " << objectCount << std::endl;
  if (objectCountPerStylesheet.contains(dbThread->GetStylesheetFilename())){
    size_t lastObjectCount = objectCountPerStylesheet[dbThread->GetStylesheetFilename()];
    if (objectCount < lastObjectCount){
      std::cerr << "Less objects! " << lastObjectCount << " > " << objectCount << std::endl;
      failure = true;
      QApplication::quit();
      return;
    }
  }

  objectCountPerStylesheet[dbThread->GetStylesheetFilename()] = objectCount;

  stylesheetCtn++;
  emit loadStyleRequested(stylesheets.at(stylesheetCtn % stylesheets.size()).absoluteFilePath(),
                          std::unordered_map<std::string,bool>());

  if (stylesheetCtn==100){
    QApplication::quit();
    return;
  }

  timer.setInterval(800);
  timer.start();
}

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  CmdLineParser argParser("ClientQtThreading", argc, argv);
  std::vector<std::string> helpArgs{"h","help"};
  Arguments args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=QString::fromStdString(value);
                          }),
                          "DATABASE",
                          "Directory for databases lookup");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.stylesheetDirectory=QString::fromStdString(value);
                          }),
                          "STYLESHEETS",
                          "Directory with stylesheets");

  argParser.AddPositional(osmscout::CmdLineGeoCoordOption([&args](const osmscout::GeoCoord& value) {
                            args.renderingCenter=value;
                          }),
                          "CENTER",
                          "Rendering center");

  argParser.AddPositional(osmscout::CmdLineUIntOption([&args](const unsigned int& value) {
                            args.magLevel=value;
                          }),
                          "LEVEL",
                          "Rendering magnification level");

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

  QStringList mapLookupDirectories;
  mapLookupDirectories << args.databaseDirectory;

  QList<QFileInfo> stylesheets;
  QString stylesheetDirectory = args.stylesheetDirectory;
  QDirIterator dirIt(stylesheetDirectory, QDir::Files, QDirIterator::FollowSymlinks);
  while (dirIt.hasNext()) {
    dirIt.next();
    QFileInfo fInfo(dirIt.filePath());
    if (fInfo.isFile() && fInfo.suffix() == "oss") {
      stylesheets << fInfo;
    }
  }
  if (stylesheets.empty()){
    std::cerr << "Can't found any stylesheet in directory " << stylesheetDirectory.toStdString() << std::endl;
    return 1;
  }

  bool initialised = OSMScoutQt::NewInstance()
      .WithStyleSheetDirectory(stylesheetDirectory)
      .WithStyleSheetFile(stylesheets.at(0).fileName())
      .WithMapLookupDirectories(mapLookupDirectories)
      .WithUserAgent("ClientQtThreadingTest", "v?")
      .Init();

  if (!initialised){
    std::cerr << "Cannot initialize OSMScout library" << std::endl;
    return 1;
  }

  int result;
  {
    ThreadingTest test(stylesheets, args.renderingCenter, args.magLevel);
    result = app.exec();
    result += test.isFailed() ? 1:0;
  }
  OSMScoutQt::FreeInstance();

  if (result==0){
    std::cout << "Success!" << std::endl;
  }else{
    std::cerr << "Failure!" << std::endl;
  }
  return result;
}


