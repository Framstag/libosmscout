#ifndef OSMSCOUT_CLIENT_QT_POILOOKUPSERVICE_H
#define OSMSCOUT_CLIENT_QT_POILOOKUPSERVICE_H
/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2018 Lukas Karas

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

#include <osmscout/DBThread.h>

#include <osmscout/private/ClientQtImportExport.h>

#include <QObject>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API POILookupModule: public QObject
{
  Q_OBJECT

signals:
  void lookupAborted(int requestId);
  void lookupFinished(int requestId);
  void lookupResult(int requestId, QList<LocationEntry> locations);

public slots:
  void lookupPOIRequest(int requestId,
                        osmscout::BreakerRef breaker,
                        osmscout::GeoCoord searchCenter,
                        QStringList types,
                        double maxDistance);

private:
  QThread          *thread;
  DBThreadRef      dbThread;

public:
  POILookupModule(QThread *thread,DBThreadRef dbThread);

  virtual ~POILookupModule();

private:
  QList<LocationEntry> lookupPOIRequest(DBInstanceRef database,
                                        osmscout::GeoBox searchBoundingBox,
                                        osmscout::BreakerRef breaker,
                                        QStringList types);

};

#endif //OSMSCOUT_CLIENT_QT_POILOOKUPSERVICE_H
