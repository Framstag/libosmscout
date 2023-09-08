/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2017 Lukas Karas

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <QDebug>
#include <osmscoutclientqt/DBJob.h>

namespace osmscout {

DBJob::DBJob():
  QObject(),
  thread(QThread::currentThread()),
  locker(nullptr)
{
}

DBJob::~DBJob()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non Job thread" << thread << " in " << QThread::currentThread();
  }
  Close();
}

void DBJob::Run(const osmscout::BasemapDatabaseRef& basemapDatabase,
                const std::list<DBInstanceRef> &databases,
                QReadLocker *locker)
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Run" << this << "from non Job thread" << thread << " in " << QThread::currentThread();
  }
  this->basemapDatabase=basemapDatabase;
  this->databases=databases;
  this->locker=locker;
}

void DBJob::Close()
{
  if (locker==nullptr){
    return;
  }
  if (thread!=QThread::currentThread()){
    qWarning() << "Closing" << this << "from non Job thread" << thread << " in " << QThread::currentThread();
  }
  delete locker;
  locker=nullptr;
  databases.clear();
}

DBLoadJob::DBLoadJob(osmscout::MercatorProjection lookupProjection,
                     unsigned long maximumAreaLevel,
                     bool lowZoomOptimization,
                     bool closeOnFinish):
  DBJob(),
  closeOnFinish(closeOnFinish),
  breaker(std::make_shared<ThreadedBreaker>()),
  lookupProjection(lookupProjection)
{
  //qDebug() << "create: " << this << " in " << QThread::currentThread();

  searchParameter.SetMaximumAreaLevel(maximumAreaLevel);
  searchParameter.SetUseMultithreading(true);
  searchParameter.SetUseLowZoomOptimization(lowZoomOptimization);
  searchParameter.SetBreaker(breaker);

  connect(this, &DBLoadJob::tileStateChanged,
          this, &DBLoadJob::onTileStateChanged,
          Qt::QueuedConnection);
}

DBLoadJob::~DBLoadJob()
{
  //qDebug() << "destroying:" << this << "in" << QThread::currentThread();

  // we have to call Close from ~DBLoadJob
  // it (DBLoadJob::Close) is unreachable when ~DBJob is called
  Close();
  //qDebug() << "destroyed:" << this << "in" << QThread::currentThread();
}

void DBLoadJob::Run(const osmscout::BasemapDatabaseRef& basemapDatabase,
                    const std::list<DBInstanceRef> &databases,
                    QReadLocker *locker)
{
  osmscout::GeoBox lookupBox(lookupProjection.GetDimensions());
  std::list<DBInstanceRef> relevantDatabases;
  for (const auto &db:databases){
    if (!db->IsOpen() || (!db->GetStyleConfig())) {
      log.Warn() << "Database is not ready" << db->path;
      continue;
    }
    osmscout::GeoBox dbBox=db->GetDBGeoBox();
    if (!dbBox.Intersects(lookupBox)){
      log.Debug() << "Skip db" << db->path;
      continue;
    }
    relevantDatabases.push_back(db);
  }

  DBJob::Run(basemapDatabase,relevantDatabases,locker);
  for (auto &db:relevantDatabases){
    std::list<osmscout::TileRef> tiles;
    db->GetMapService()->LookupTiles(lookupProjection,tiles);

    QString path=QString::fromStdString(db->path);
    osmscout::MapService::TileStateCallback callback=[this,path](const osmscout::TileRef& tile) {
      //std::cout << "callback called for job: " << this << std::endl;
      emit tileStateChanged(path,tile);
    };

    osmscout::MapService::CallbackId callbackId=db->GetMapService()->RegisterTileStateCallback(callback);
    //std::cout << "callback registered for job: " << this << " " << db->path.toStdString() << ": " << callbackId  << std::endl ;
    callbacks[path]=callbackId;
    loadedTiles[path]=QMap<osmscout::TileKey,osmscout::TileRef>();
    QMap<osmscout::TileKey,osmscout::TileRef> tileMap;
    for (const auto &tile:tiles){
      tileMap[tile->GetKey()]=tile;
    }
    allTiles[path]=tileMap;
    loadingTiles[path]=tileMap;

    // load tiles asynchronous
    db->GetMapService()->LoadMissingTileDataAsync(searchParameter,
                                             *db->GetStyleConfig(),
                                             tiles);

    // process already completed tiles (state callback is not called in such case)
    for (auto &tile:tiles){
      if (tile->IsComplete()){
        emit tileStateChanged(path,tile);
      }
    }

  }
  if (relevantDatabases.empty()){
    emit finished(loadedTiles);
    //qDebug() << "Loaded completely (no relevant databases):" << this << "in" << QThread::currentThread();
    if (closeOnFinish){
      Close();
    }
  }
}

void DBLoadJob::onTileStateChanged(QString dbPath,const osmscout::TileRef tile)
{
  if (!tile->IsComplete()){
    return; // ignore incomplete
  }
  // qDebug() << "Callback:" << this << "in" << QThread::currentThread();
  if (thread!=QThread::currentThread()){
    qWarning() << "Tile callback" << this << "from non Job thread" << thread << " in " << QThread::currentThread();
  }
  if (!loadingTiles.contains(dbPath)){
    return; // loaded already
  }

  QMap<osmscout::TileKey,osmscout::TileRef> &loadingTileMap=loadingTiles[dbPath];
  auto tileIt=loadingTileMap.find(tile->GetKey());
  if (tileIt==loadingTileMap.end()){
    return; // not our request, ignore
  }

  // mark as complete
  QMap<osmscout::TileKey,osmscout::TileRef> &loadedTileMap=loadedTiles[dbPath];
  loadedTileMap[tile->GetKey()]=tileIt.value();
  loadingTileMap.remove(tile->GetKey());

  if (loadingTileMap.isEmpty()){ // this db is finished
    loadingTiles.remove(dbPath);
    emit databaseLoaded(dbPath,loadedTileMap.values());
    if (loadingTiles.isEmpty()){ // all databases are finished
      emit finished(loadedTiles);
      //qDebug() << "Loaded completely:" << this << "in" << QThread::currentThread();
      if (closeOnFinish){
        Close();
      }
    }
  }
}

void DBLoadJob::Close()
{
  // stop asynchronous loading if it is still running
  breaker->Break();

  // deregister callbacks
  for (auto &db:databases){
    QString path=QString::fromStdString(db->path);
    if (callbacks.contains(path)){
      //qDebug() << "Remove callback for job:" << this << ":" << callbacks[db->path] << "in" << QThread::currentThread();
      db->GetMapService()->DeregisterTileStateCallback(callbacks[path]);
      callbacks.remove(path);
    }
  }

  DBJob::Close();
}

bool DBLoadJob::IsFinished() const
{
  return loadingTiles.isEmpty();
}

QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> DBLoadJob::GetAllTiles() const
{
  return allTiles;
}

bool DBLoadJob::AddTileDataToMapData(QString dbPath,
                                     const QList<osmscout::TileRef> &tiles,
                                     osmscout::MapData &data)
{
  for (auto &db:databases){
    if (db->path==dbPath.toStdString()){
      std::list<osmscout::TileRef> tileList;
      for (const auto &tile:tiles){
        tileList.push_back(tile);
      }
      db->GetMapService()->AddTileDataToMapData(tileList,data);
      return true;
    }
  }
  return false;
}
}
