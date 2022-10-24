#ifndef OSMSCOUT_CLIENT_QT_ONLINETILEPROVIDER_H
#define	OSMSCOUT_CLIENT_QT_ONLINETILEPROVIDER_H

/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2016 Lukas Karas

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
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <osmscoutclientqt/ClientQtImportExport.h>

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Online tile provider object. See OnlineTileProviderModel and Settings.
 */
class OSMSCOUT_CLIENT_QT_API OnlineTileProvider: public QObject
{
  Q_OBJECT

public:
  OnlineTileProvider() = default;

  OnlineTileProvider(const OnlineTileProvider &o):
    QObject(o.parent()),
    valid(o.valid), id(o.id), name(o.name), servers(o.servers),
          maximumZoomLevel(o.maximumZoomLevel), copyright(o.copyright){};

  OnlineTileProvider(const QString &id, const QString &name, const QStringList &servers, int maximumZoomLevel,
          QString copyright):
    valid(true), id(id), name(name), servers(servers), maximumZoomLevel(maximumZoomLevel),
    copyright(copyright){};

  ~OnlineTileProvider() override = default;

  OnlineTileProvider& operator=(const OnlineTileProvider &o)
  {
    valid = o.valid;
    id = o.id;
    name = o.name;
    servers = o.servers;
    maximumZoomLevel = o.maximumZoomLevel;
    copyright = o.copyright;

    return *this;
  }

  QString getId() const {
    return id;
  }

  int getMaximumZoomLevel() const {
    return maximumZoomLevel;
  }

  QString getName() const {
    return name;
  }

  QStringList getServers() const {
    return servers;
  }

  bool isValid() const {
    return valid;
  }

  QString getCopyright() const
  {
    return copyright;
  }

  static OnlineTileProvider fromJson(QJsonValue obj);

private:
  bool valid{false};
  QString id;
  QString name;
  QStringList servers;
  int maximumZoomLevel{-1};
  QString copyright;
};

}

#endif /* OSMSCOUT_CLIENT_QT_ONLINETILEPROVIDER_H */
