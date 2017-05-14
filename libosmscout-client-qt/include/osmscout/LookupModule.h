#ifndef LOOKUPMODULE_H
#define LOOKUPMODULE_H

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
#include <QThread>
#include <osmscout/DBThread.h>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API LookupModule:public QObject{
  Q_OBJECT

private:
  QMutex           mutex;
  QThread          *thread;
  DBThreadRef      dbThread;
  DBLoadJob        *loadJob;
  RenderMapRequest view;

signals:
  void InitialisationFinished(const DatabaseLoadedResponse& response);
  void viewObjectsLoaded(const RenderMapRequest&, const osmscout::MapData&);

public slots:
  void requestObjectsOnView(const RenderMapRequest&);
  void onDatabaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles);
  void onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> tiles);

public:
  LookupModule(QThread *thread,DBThreadRef dbThread);
  virtual ~LookupModule();
};

/**
 * \ingroup QtAPI
 */
typedef std::shared_ptr<LookupModule> LookupModuleRef;

#endif /* LOOKUPMODULE_H */

