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

#ifndef ONLINETILEPROVIDER_H
#define	ONLINETILEPROVIDER_H

#include <QObject>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 * 
 * Online tile provider object. See OnlineTileProviderModel and Settings.
 */
class OSMSCOUT_CLIENT_QT_API OnlineTileProvider: public QObject
{
  Q_OBJECT
  
public: 
  inline OnlineTileProvider(): valid(false){}; 
  
  inline OnlineTileProvider(const OnlineTileProvider &o):
    QObject(o.parent()),
    valid(o.valid), id(o.id), name(o.name), servers(o.servers), 
          maximumZoomLevel(o.maximumZoomLevel), copyright(o.copyright){};
  
  inline OnlineTileProvider(QString id, QString name, QStringList servers, int maximumZoomLevel, 
          QString copyright):
    valid(true), id(id), name(name), servers(servers), maximumZoomLevel(maximumZoomLevel), 
    copyright(copyright){};
    
  virtual inline ~OnlineTileProvider(){};

  inline void operator=(const OnlineTileProvider &o)
  {
    valid = o.valid;
    id = o.id;
    name = o.name;
    servers = o.servers;
    maximumZoomLevel = o.maximumZoomLevel;
    copyright = o.copyright;
  }

  inline QString getId() const {
    return id;
  }

  inline int getMaximumZoomLevel() const {
    return maximumZoomLevel;
  }

  inline QString getName() const {
    return name;
  }

  inline QStringList getServers() const {
    return servers;
  }

  inline bool isValid() const {
    return valid;
  }

  QString getCopyright() const
  {
    return copyright;
  }

  static OnlineTileProvider fromJson(QJsonValue obj);

private: 
  bool valid;
  QString id;
  QString name;
  QStringList servers;
  int maximumZoomLevel;
  QString copyright;
};

#endif	/* ONLINETILEPROVIDER_H */

