#ifndef OSMSCOUT_CLIENT_QT_ELEVATIONMODULE_H
#define OSMSCOUT_CLIENT_QT_ELEVATIONMODULE_H

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

#include <osmscout/OverlayObject.h>
#include <osmscout/DBThread.h>
#include <osmscout/ElevationService.h>

#include <osmscout/ClientQtImportExport.h>

#include <QThread>
#include <memory>

namespace osmscout {

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API ElevationModule : public QObject
{
  Q_OBJECT
public:
  using ElevationPoints = std::vector<osmscout::ElevationPoint>;

  class DataLoader
  {
  private:
    std::vector<osmscout::DatabaseRef> database;
    std::vector<osmscout::TypeInfoSet> contourTypes;
    std::vector<osmscout::EleFeatureValueReader> reader;
  public:
    explicit DataLoader(const std::list<DBInstanceRef> &databases);

    std::vector<osmscout::ContoursData> LoadContours(const osmscout::GeoBox &box);
  };

public slots:
  void onElevationProfileRequest(std::shared_ptr<OverlayWay> way,
                                 int requestId,
                                 osmscout::BreakerRef breaker);

signals:
  void error(int requestId);

  void elevationProfileAppend(ElevationModule::ElevationPoints points, int requestId);
  void loadingFinished(int requestId);

public:
  ElevationModule(QThread *thread,DBThreadRef dbThread);

  ~ElevationModule() override;

private:
  QThread          *thread;
  DBThreadRef      dbThread;
};
}

#endif // OSMSCOUT_CLIENT_QT_ELEVATIONMODULE_H
