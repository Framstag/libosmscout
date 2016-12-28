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

#ifndef MAPPROVIDER_H
#define	MAPPROVIDER_H

#include <QObject>
#include <QString>
#include <QUrl>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API MapProvider: public QObject
{
  Q_OBJECT

private:
  bool valid;
  QString uri;
  QString listUri;
  QString name;
  
public:
  inline MapProvider():valid(false) {}
  
  inline MapProvider(const MapProvider &o):
    QObject(o.parent()),
    valid(o.valid), uri(o.uri), listUri(o.listUri), name(o.name){};  
  
  inline MapProvider(QString name, QString uri, QString listUri): 
    valid(true), uri(uri), listUri(listUri), name(name) {}
  
  virtual inline ~MapProvider() {}
  
  inline void operator=(const MapProvider &o)
  {
    valid = o.valid;
    uri = o.uri;
    listUri = o.listUri;
    name = o.name;
  }  
  
  inline QString getName() const
  {
    return name;
  }
  
  inline QString getUri() const
  {
    return uri;
  }
  
  inline QUrl getListUri(int fromVersion, int toVersion, QString locale="en") const
  {
    return listUri.arg(fromVersion).arg(toVersion).arg(locale);
  }
  
  inline bool isValid() const
  {
    return valid;
  }
  
  static MapProvider fromJson(QJsonValue obj);
};

Q_DECLARE_METATYPE(MapProvider)
  
#endif
