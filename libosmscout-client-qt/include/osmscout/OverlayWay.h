#ifndef OSMSCOUT_CLIENT_QT_OVERLAYWAY_H
#define OSMSCOUT_CLIENT_QT_OVERLAYWAY_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map

 Copyright (C) 2017  Lukáš Karas

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
#include <QMutex>
#include <QMutexLocker>

#include <osmscout/Way.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 *
 * Qt abstraction for various objects on map,
 * used for search and routing
 */
class OSMSCOUT_CLIENT_QT_API OverlayWay : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString type READ getTypeName WRITE setTypeName)
  Q_PROPERTY(int size READ getSize)

private:
  QString                       typeName;
  std::vector<osmscout::Point>  nodes;
  osmscout::GeoBox              box;
  mutable QMutex                lock;

public slots:
  void clear();
  void addPoint(double lat, double lon);

public:
  OverlayWay(QObject *parent=Q_NULLPTR);

  OverlayWay(const std::vector<osmscout::Point> &nodes,
             QString typeName="_route",
             QObject *parent=Q_NULLPTR);

  
  virtual ~OverlayWay();

  void set(const OverlayWay &other);
  
  inline QString getTypeName() const
  {
    QMutexLocker locker(&lock);
    return typeName;
  }

  inline void setTypeName(QString name){
    QMutexLocker locker(&lock);
    typeName=name;
  }

  inline int getSize(){
    return nodes.size();
  }

  bool toWay(osmscout::WayRef &way,
             const osmscout::TypeConfig &typeConfig) const;

  osmscout::GeoBox boundingBox();
};

typedef std::shared_ptr<OverlayWay> OverlayWayRef;

#endif /* OSMSCOUT_CLIENT_QT_OVERLAYWAY_H */
