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

#include <osmscout/LookupModule.h>

LookupModule::LookupModule(QThread *thread,DBThreadRef dbThread):
  QObject(),
  thread(thread),
  dbThread(dbThread),
  loadJob(NULL)
{

  connect(dbThread.get(), SIGNAL(InitialisationFinished(const DatabaseLoadedResponse&)),
          this, SIGNAL(InitialisationFinished(const DatabaseLoadedResponse&)));
}

LookupModule::~LookupModule()
{
  if (thread!=NULL){
    thread->quit();
    thread->deleteLater();
  }
}

void LookupModule::requestObjectsOnView(const RenderMapRequest &view)
{
  double mapDpi=dbThread->GetMapDpi();

  // setup projection for data lookup
  osmscout::MercatorProjection lookupProjection;
  lookupProjection.Set(view.coord,  0, view.magnification, mapDpi, view.width*1.5, view.height*1.5);
  lookupProjection.SetLinearInterpolationUsage(view.magnification.GetLevel() >= 10);

  if (loadJob!=NULL){
    delete loadJob;
  }

  unsigned long maximumAreaLevel=4;
  if (view.magnification.GetLevel() >= 15) {
    maximumAreaLevel=6;
  }

  loadJob=new DBLoadJob(lookupProjection,maximumAreaLevel,/* lowZoomOptimization */ true);
  this->view=view;
  
  connect(loadJob, SIGNAL(databaseLoaded(QString,QList<osmscout::TileRef>)),
          this, SLOT(onDatabaseLoaded(QString,QList<osmscout::TileRef>)));
  connect(loadJob, SIGNAL(finished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)),
          this, SLOT(onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>>)));

  dbThread->RunJob(loadJob);
}

void LookupModule::onDatabaseLoaded(QString dbPath,QList<osmscout::TileRef> tiles)
{
  osmscout::MapData data;
  loadJob->AddTileDataToMapData(dbPath,tiles,data);
  emit viewObjectsLoaded(view, data);
}

void LookupModule::onLoadJobFinished(QMap<QString,QMap<osmscout::TileId,osmscout::TileRef>> /*tiles*/)
{
  emit viewObjectsLoaded(view, osmscout::MapData());
}
