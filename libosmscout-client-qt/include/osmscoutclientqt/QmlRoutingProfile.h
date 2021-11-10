#ifndef OSMSCOUT_CLIENT_QT_QMLROUTINGPROFILE_H
#define OSMSCOUT_CLIENT_QT_QMLROUTINGPROFILE_H

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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscout/OSMScoutTypes.h>
#include <osmscout/util/Distance.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/TypeConfig.h>

#include <QObject>
#include <QVariant>

#include <cmath>

namespace osmscout {

/**
 * Routing profile
 *
 * \ingroup QtAPI
 */
class OSMSCOUT_CLIENT_QT_API QmlRoutingProfile: public QObject {
  Q_OBJECT
  Q_PROPERTY(QmlVehicle        vehicle    READ getQmlVehicle  WRITE setVehicle    NOTIFY update)
  Q_PROPERTY(double            maxSpeed   READ getMaxSpeed    WRITE setMaxSpeed   NOTIFY update)
  Q_PROPERTY(QVariantMap       speedTable READ getSpeedTable  WRITE setSpeedTable NOTIFY update)

  Q_PROPERTY(bool applyJunctionPenalty READ getJunctionPenalty WRITE setJunctionPenalty NOTIFY update)
  // meters
  Q_PROPERTY(double penaltySameType READ getPenaltySameType WRITE setPenaltySameType NOTIFY update)
  // meters
  Q_PROPERTY(double penaltyDifferentType READ getPenaltyDifferentType WRITE setPenaltyDifferentType NOTIFY update)
  // seconds
  Q_PROPERTY(double maxPenalty READ getMaxPenalty WRITE setMaxPenalty NOTIFY update)

public:
  // enums fields exported to qml have to start with uppercase...
  enum QmlVehicle: uint8_t
  {
    FootVehicle     = Vehicle::vehicleFoot,
    BicycleVehicle  = Vehicle::vehicleBicycle,
    CarVehicle      = Vehicle::vehicleCar
  };

  Q_ENUM(QmlVehicle);

signals:
  void update();

public:
  Q_INVOKABLE explicit QmlRoutingProfile(QObject *parent = nullptr);
  explicit QmlRoutingProfile(Vehicle vehicle);
  ~QmlRoutingProfile() override = default;

  QmlRoutingProfile(const QmlRoutingProfile& other);
  QmlRoutingProfile& operator=(const QmlRoutingProfile& other);

  Vehicle getVehicle() const;
  QmlVehicle getQmlVehicle() const;
  void setVehicle(QmlVehicle vehicle);

  double getMaxSpeed() const;
  void setMaxSpeed(double);

  QVariantMap getSpeedTable() const;
  void setSpeedTable(const QVariantMap &);

  bool getJunctionPenalty() const;
  void setJunctionPenalty(bool);

  double getPenaltySameType() const;
  void setPenaltySameType(double);

  double getPenaltyDifferentType() const;
  void setPenaltyDifferentType(double);

  double getMaxPenalty() const;
  void setMaxPenalty(double);

  RoutingProfileRef MakeInstance(TypeConfigRef typeConfig) const;

private:
  void setDefaults(); //!< setup defaults for current vehicle

private:
  QmlVehicle vehicle=QmlVehicle::CarVehicle;
  double maxSpeed=160;
  std::map<std::string,SpeedVariant> speedTable;
  bool applyJunctionPenalty=true;
  Distance costLimitDistance=Kilometers(20);
  double costLimitFactor=7.5;
  osmscout::Distance penaltySameType=Meters(160);
  osmscout::Distance penaltyDifferentType=Meters(250);
  std::chrono::seconds maxPenalty=std::chrono::seconds(10);
};

using QmlRoutingProfileRef = std::shared_ptr<QmlRoutingProfile>;

}

Q_DECLARE_METATYPE(osmscout::QmlRoutingProfileRef)

#endif //OSMSCOUT_CLIENT_QT_QMLROUTINGPROFILE_H
