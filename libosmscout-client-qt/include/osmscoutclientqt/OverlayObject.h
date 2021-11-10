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
#include <osmscoutclientqt/ClientQtImportExport.h>
#include <osmscoutclientqt/LocationEntry.h>

#include <optional>

namespace osmscout {

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
  Q_PROPERTY(qint8 layer READ getLayer WRITE setLayer)
  Q_PROPERTY(QString name READ getName WRITE setName)
  Q_PROPERTY(QString color READ getColor WRITE setColor)
  Q_PROPERTY(LocationEntry *boundingBox READ getBBoxAsLocation NOTIFY bboxChanged)

protected:
  QString                             typeName;
  std::vector<osmscout::Point>        nodes;
  mutable std::vector<SegmentGeoBox>  segmentsBoxes;
  mutable osmscout::GeoBox            box;
  int8_t                              layer{std::numeric_limits<int8_t>::max()};
  QString                             name;
  QString                             color;
  mutable QMutex                      lock;
  std::optional<osmscout::Color>      colorValue;

public slots:
  void clear();
  void addPoint(double lat, double lon);

signals:
  void bboxChanged();

public:
  OverlayObject(QObject *parent=Q_NULLPTR);

  explicit OverlayObject(const std::vector<osmscout::Point> &nodes,
                         QString typeName="_route",
                         QObject *parent=Q_NULLPTR);

  OverlayObject(const OverlayObject &o);

  ~OverlayObject() override;

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

  inline QString getTypeName() const
  {
    QMutexLocker locker(&lock);
    return typeName;
  }

  inline void setTypeName(const QString &name){
    QMutexLocker locker(&lock);
    typeName=name;
  }

  inline size_t getSize(){
    QMutexLocker locker(&lock);
    return nodes.size();
  }

  inline qint8 getLayer() const
  {
    QMutexLocker locker(&lock);
    return layer;
  }

  inline void setLayer(qint8 l)
  {
    QMutexLocker locker(&lock);
    layer = l;
  }

  inline QString getName() const
  {
    QMutexLocker locker(&lock);
    return name;
  }

  inline void setName(const QString &n)
  {
    QMutexLocker locker(&lock);
    name = n;
  }

  inline QString getColor() const
  {
    QMutexLocker locker(&lock);
    return color;
  }

  void setColor(const QString &c);
  void setColorValue(Color &c);

  LocationEntry* getBBoxAsLocation() const;
  osmscout::GeoBox boundingBox() const;
  std::vector<osmscout::GeoCoord> getCoords() const;
  std::vector<osmscout::Point> getPoints() const;

protected:
  void setupFeatures(const osmscout::TypeInfoRef &type,
                     osmscout::FeatureValueBuffer &features) const;

  // internal, lock have to be acquired
  osmscout::GeoBox boundingBoxInternal() const;

  // internal, lock have to be acquired
  std::vector<SegmentGeoBox> segments() const;
};


class OSMSCOUT_CLIENT_QT_API OverlayArea : public OverlayObject
{
Q_OBJECT

public:
  OverlayArea(QObject *parent=Q_NULLPTR);

  explicit OverlayArea(const std::vector<osmscout::Point> &nodes,
                       QString typeName="_route",
                       QObject *parent=Q_NULLPTR);

  ~OverlayArea() override;

  osmscout::RefType getObjectType() const override{
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

  explicit OverlayWay(const std::vector<osmscout::Point> &nodes,
                      QString typeName="_route",
                      QObject *parent=Q_NULLPTR);

  ~OverlayWay() override;

  osmscout::RefType getObjectType() const override{
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

  explicit OverlayNode(const std::vector<osmscout::Point> &nodes,
                       QString typeName="_route",
                       QObject *parent=Q_NULLPTR);

  ~OverlayNode() override;

  osmscout::RefType getObjectType() const override{
    return osmscout::RefType::refNode;
  }

  bool toNode(osmscout::NodeRef &node,
              const osmscout::TypeConfig &typeConfig) const;
};


using OverlayObjectRef = std::shared_ptr<OverlayObject>;

}

#endif /* OSMSCOUT_CLIENT_QT_OVERLAYOBJECT_H */
