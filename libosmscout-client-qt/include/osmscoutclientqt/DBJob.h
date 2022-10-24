#ifndef OSMSCOUT_CLIENT_QT_DBOPERATION_H
#define OSMSCOUT_CLIENT_QT_DBOPERATION_H

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

#include <QObject>
#include <QHash>
#include <QList>
#include <QThread>
#include <QReadWriteLock>

#include <osmscout/BasemapDatabase.h>
#include <osmscoutclientqt/DBInstance.h>
#include <osmscoutmap/DataTileCache.h>

#include <osmscoutclientqt/ClientQtImportExport.h>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API DBJob : public QObject{
  Q_OBJECT

protected:
  osmscout::BasemapDatabaseRef basemapDatabase; //!< Optional reference to the basemap database
  std::list<DBInstanceRef>     databases;       //!< borrowed databases
  QThread                      *thread;         //!< job thread

private:
  QReadLocker                  *locker;         //!< database locker

public:
  DBJob();
  ~DBJob() override;

  virtual void Run(const osmscout::BasemapDatabaseRef& basempaDatabase,
                   const std::list<DBInstanceRef> &databases, QReadLocker *locker);
  virtual void Close();
};

class OSMSCOUT_CLIENT_QT_API DBLoadJob : public DBJob{
  Q_OBJECT

private:
  bool                                            closeOnFinish;
  osmscout::BreakerRef                            breaker;
  osmscout::MercatorProjection                    lookupProjection;
  osmscout::AreaSearchParameter                   searchParameter;
  QMap<QString,osmscout::MapService::CallbackId>  callbacks;

  QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> allTiles;
  QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> loadingTiles;
  QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> loadedTiles;

protected slots:
  void onTileStateChanged(QString dbPath,const osmscout::TileRef tile);

signals:
  /**
   * This signal is not called in Job thread context!
   */
  void tileStateChanged(QString dbPath,const osmscout::TileRef tile);

  void databaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles);

  void finished(QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> tiles);

public:
  DBLoadJob(osmscout::MercatorProjection lookupProjection,
            unsigned long maximumAreaLevel,
            bool lowZoomOptimization,
            bool closeOnFinish=true);
  ~DBLoadJob() override;

  void Run(const osmscout::BasemapDatabaseRef& basempaDatabase,
                   const std::list<DBInstanceRef> &databases,
                   QReadLocker *locker) override;
  void Close() override;

  bool IsFinished() const;
  QMap<QString,QMap<osmscout::TileKey,osmscout::TileRef>> GetAllTiles() const;

  /**
   * Add tile data to map data.
   *
   * @param dbPath
   * @param tiles
   * @param data
   * @return true on success
   *         false when given database was not added to this job, or job was closed
   */
  bool AddTileDataToMapData(QString dbPath,
                            const QList<osmscout::TileRef> &tiles,
                            osmscout::MapData &data);
};

}

#endif /* OSMSCOUT_CLIENT_QT_DBOPERATION_H */
