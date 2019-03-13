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

#include <osmscout/util/Logger.h>

#include <osmscout/OSMScoutQt.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QScreen>
#include <QtGui>
#include <QDebug>

#include <iostream>

using namespace osmscout;


ThreadingTest::ThreadingTest(const QList<QFileInfo> &stylesheets):
    QObject(nullptr),
    stylesheets(stylesheets),
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
  projection.Set(GeoCoord(50.07213, 14.407) /* Prague */,
                 /*currentAngle*/ 0.0,
                 Magnification(Magnification::magCloser),
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
  for (const auto &map:tiles){
    for (const auto &tile:map){
      objectCount += tile->GetAreaData().GetDataSize() + tile->GetWayData().GetDataSize() + tile->GetNodeData().GetDataSize();
    }
  }
  std::cout << "Stylesheet: " << dbThread->GetStylesheetFilename().toStdString() << ", object count: " << objectCount << std::endl;
  if (objectCount < lastObjectCount){
    std::cerr << "Less objects! " << lastObjectCount << " > " << objectCount << std::endl;
    //QApplication::quit();
    //return;
  }

    lastObjectCount = objectCount;

    stylesheetCtn++;
    emit loadStyleRequested(stylesheets.at(stylesheetCtn % stylesheets.size()).absoluteFilePath(),
                            std::unordered_map<std::string,bool>());

    timer.setInterval(10000);
    timer.start();

}

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  if (app.arguments().size() < 3) {
    qWarning() << "No enough arguments!";
    std::cout << "Usage:" << std::endl;
    std::cout << app.arguments().at(0).toStdString() << " MapDirectory stylesheetDirectory" << std::endl;
    return 1;
  }

  osmscout::log.Info(true);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);
  osmscout::log.Debug(true);

  // register OSMScout library QML types
  OSMScoutQt::RegisterQmlTypes();

  QStringList mapLookupDirectories;
  mapLookupDirectories << app.arguments().at(1);

  QList<QFileInfo> stylesheets;
  QString stylesheetDirectory = app.arguments().at(2);
  QDirIterator dirIt(stylesheetDirectory, QDirIterator::FollowSymlinks);
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
    ThreadingTest test(stylesheets);
    result = app.exec();
  }
  OSMScoutQt::FreeInstance();
  return result;
}


