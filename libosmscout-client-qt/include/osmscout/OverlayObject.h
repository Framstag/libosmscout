#ifndef OSMSCOUT_CLIENT_QT_OVERLAYOBJECT_H
#define OSMSCOUT_CLIENT_QT_OVERLAYOBJECT_H

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
#include <osmscout/Area.h>
#include <osmscout/Node.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/private/ClientQtImportExport.h>

/**
 * \ingroup QtAPI
 *
 * Qt abstraction for various objects on map,
 * used for search and routing
 */
class OSMSCOUT_CLIENT_QT_API OverlayObject : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString type READ getTypeName WRITE setTypeName)
  Q_PROPERTY(int size READ getSize)
  Q_PROPERTY(QString objectType READ getObjectTypeStr)

protected:
  QString                       typeName;
  std::vector<osmscout::Point>  nodes;
  osmscout::GeoBox              box;
  mutable QMutex                lock;

public slots:
  void clear();
  void addPoint(double lat, double lon);

public:
  OverlayObject(QObject *parent=Q_NULLPTR);

  OverlayObject(const std::vector<osmscout::Point> &nodes,
                QString typeName="_route",
                QObject *parent=Q_NULLPTR);

  virtual ~OverlayObject();

  virtual osmscout::RefType getObjectType() const{
    return osmscout::RefType::refNone;
  }

  QString getObjectTypeStr() const {
    switch (getObjectType()){
      case osmscout::RefType::refArea:
        return "area";
      case osmscout::RefType::refWay:
        return "way";
      case osmscout::RefType::refNode:
        return "node";
      default:
        return "none";
    }
  }

  bool set(const OverlayObject &other);
  
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

  osmscout::GeoBox boundingBox();
};


class OSMSCOUT_CLIENT_QT_API OverlayArea : public OverlayObject
{
Q_OBJECT

public:
  OverlayArea(QObject *parent=Q_NULLPTR);

  OverlayArea(const std::vector<osmscout::Point> &nodes,
              QString typeName="_route",
              QObject *parent=Q_NULLPTR);

  virtual ~OverlayArea();

  virtual osmscout::RefType getObjectType() const{
    return osmscout::RefType::refArea;
  }

  bool toArea(osmscout::AreaRef &area,
              const osmscout::TypeConfig &typeConfig) const;
};

class OSMSCOUT_CLIENT_QT_API OverlayWay : public OverlayObject
{
Q_OBJECT

public:
  OverlayWay(QObject *parent=Q_NULLPTR);

  OverlayWay(const std::vector<osmscout::Point> &nodes,
             QString typeName="_route",
             QObject *parent=Q_NULLPTR);

  virtual ~OverlayWay();

  virtual osmscout::RefType getObjectType() const{
    return osmscout::RefType::refWay;
  }

  bool toWay(osmscout::WayRef &way,
             const osmscout::TypeConfig &typeConfig) const;
};

class OSMSCOUT_CLIENT_QT_API OverlayNode : public OverlayObject
{
Q_OBJECT

public:
  OverlayNode(QObject *parent=Q_NULLPTR);

  OverlayNode(const std::vector<osmscout::Point> &nodes,
              QString typeName="_route",
              QObject *parent=Q_NULLPTR);

  virtual ~OverlayNode();

  virtual osmscout::RefType getObjectType() const{
    return osmscout::RefType::refNode;
  }

  bool toNode(osmscout::NodeRef &node,
              const osmscout::TypeConfig &typeConfig) const;
};


typedef std::shared_ptr<OverlayObject> OverlayObjectRef;

#endif /* OSMSCOUT_CLIENT_QT_OVERLAYOBJECT_H */
