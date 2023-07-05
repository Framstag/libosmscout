/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2021 Lukas Karas

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

#include <osmscoutclientqt/ElevationModule.h>
#include <osmscout/elevation/ElevationService.h>

namespace osmscout {

ElevationModule::ElevationModule(QThread *thread,DBThreadRef dbThread):
    thread(thread), dbThread(dbThread)
{

}

ElevationModule::~ElevationModule()
{
  if (thread!=QThread::currentThread()){
    qWarning() << "Destroy" << this << "from non incorrect thread;" << thread << "!=" << QThread::currentThread();
  }
  if (thread!=nullptr){
    thread->quit();
  }
}

ElevationModule::DataLoader::DataLoader(const std::list<DBInstanceRef> &databases)
{
  database.reserve(databases.size());
  contourTypes.reserve(databases.size());
  reader.reserve(databases.size());

  for (const DBInstanceRef &dbInst: databases){
    DatabaseRef db=dbInst->GetDatabase();
    database.push_back(db);
    reader.push_back(osmscout::EleFeatureValueReader(*db->GetTypeConfig()));
    osmscout::TypeInfoSet types;
    for (const auto &type:db->GetTypeConfig()->GetWayTypes()){
      if (type->HasFeature(osmscout::EleFeature::NAME)){
        osmscout::log.Debug() << "Using type " << type->GetName();
        types.Set(type);
      }
    }
    contourTypes.push_back(types);
  }
}

std::vector<osmscout::ContoursData> ElevationModule::DataLoader::LoadContours(const osmscout::GeoBox &box)
{
  assert(database.size() == contourTypes.size());
  assert(database.size() == reader.size());

  std::vector<osmscout::ContoursData> result;
  result.reserve(database.size());
  for (size_t i=0; i<database.size(); i++) {
    osmscout::StopClock stopClock;
    std::vector<osmscout::FileOffset> offsets;
    osmscout::TypeInfoSet loadedTypes;
    if (!database[i]->GetAreaWayIndex()->GetOffsets(box, contourTypes[i], offsets, loadedTypes)) {
      assert(false);
    }
    std::vector<osmscout::WayRef> contours;
    if (!database[i]->GetWaysByOffset(offsets, contours)) {
      assert(false);
    }
    result.push_back({reader[i],contours});
  }
  return result;
}

void ElevationModule::onElevationProfileRequest(std::shared_ptr<OverlayWay> overlayWay,
                                                int requestId,
                                                osmscout::BreakerRef breaker)
{
  assert(overlayWay);
  dbThread->RunSynchronousJob([&](const std::list<DBInstanceRef> &databases){
    DataLoader dataLoader(databases);
    osmscout::ElevationService<DataLoader> eleService(dataLoader, osmscout::Magnification::magSuburb);

    eleService.ElevationProfile(overlayWay->getCoords(), [&](const osmscout::Distance&, const std::vector<osmscout::ElevationPoint> &points){
      emit elevationProfileAppend( points, requestId);
    }, breaker);

    emit loadingFinished(requestId);
  });
}


}
