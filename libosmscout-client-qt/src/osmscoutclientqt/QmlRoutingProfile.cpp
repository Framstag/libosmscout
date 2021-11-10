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

#include <osmscoutclientqt/QmlRoutingProfile.h>

#include <QDebug>

namespace osmscout {

QmlRoutingProfile::QmlRoutingProfile(QObject *parent):
  QObject(parent)
{
  setDefaults();
}

QmlRoutingProfile::QmlRoutingProfile(Vehicle vehicle):
  vehicle(static_cast<QmlVehicle>(vehicle))
{
  setDefaults();
}

QmlRoutingProfile::QmlRoutingProfile(const QmlRoutingProfile& other):
  QObject(other.parent())
{
   operator=(other);
}

QmlRoutingProfile& QmlRoutingProfile::operator=(const QmlRoutingProfile& other)
{
  vehicle=other.vehicle;
  maxSpeed=other.maxSpeed;
  speedTable=other.speedTable;
  costLimitDistance=other.costLimitDistance;
  costLimitFactor=other.costLimitFactor;
  applyJunctionPenalty=other.applyJunctionPenalty;
  penaltySameType=other.penaltySameType;
  penaltyDifferentType=other.penaltyDifferentType;
  maxPenalty=other.maxPenalty;

  emit update();
  return *this;
}

Vehicle QmlRoutingProfile::getVehicle() const
{
  return static_cast<Vehicle>(vehicle);
}

QmlRoutingProfile::QmlVehicle QmlRoutingProfile::getQmlVehicle() const
{
  return vehicle;
}

void QmlRoutingProfile::setVehicle(QmlVehicle vehicle)
{
  if (this->vehicle==vehicle){
    return;
  }
  this->vehicle = vehicle;
  setDefaults();
}

double QmlRoutingProfile::getMaxSpeed() const
{
  return maxSpeed;
}

void QmlRoutingProfile::setMaxSpeed(double d)
{
  if (maxSpeed==d){
    return;
  }
  maxSpeed=d;
  emit update();
}

QVariantMap QmlRoutingProfile::getSpeedTable() const
{
  QVariantMap table;
  for (auto const &entry: speedTable) {
    const auto &speedArr=entry.second.speed;
    QString type=QString::fromStdString(entry.first);
    if (!std::isnan(speedArr[0]) &&
        std::all_of(speedArr.begin() + 1, speedArr.end(), [](double d){ return std::isnan(d); })) {
      table[type]=speedArr[0];
    } else {
      int grade=1;
      for (double speed: speedArr){
        if (!std::isnan(speed)){
          table[type + QString(":%1").arg(grade)]=speed;
        }
        grade++;
      }
    }
  }
  return table;
}

void QmlRoutingProfile::setSpeedTable(const QVariantMap &table)
{
  speedTable.clear();
  for (auto entry = table.begin(); entry != table.end(); entry++){
    Grade grade=SolidGrade;
    QString type=entry.key();
    bool ok=true;
    if (type.contains(':')){
      QStringList arr=type.split(':');
      if (arr.size()!=2){
        qWarning() << "Cannot parse type:" << type;
        continue;
      }

      int i=arr[1].toInt(&ok);
      if (!ok || i<1 || i>5){
        qWarning() << "Cannot parse grade:" << arr[1];
        continue;
      }
      grade=static_cast<Grade>(i);
      type=arr[0];
    }
    double speed=entry.value().toDouble(&ok);
    if (!ok || std::isnan(speed) || speed<0) {
      qWarning() << "Cannot parse speed:" << entry.value();
      continue;
    }
    speedTable[type.toStdString()][grade]=speed;
  }
}

bool QmlRoutingProfile::getJunctionPenalty() const
{
  return applyJunctionPenalty;
}

void QmlRoutingProfile::setJunctionPenalty(bool b)
{
  if (applyJunctionPenalty==b){
    return;
  }
  applyJunctionPenalty=b;
  emit update();
}

double QmlRoutingProfile::getPenaltySameType() const
{
  return penaltySameType.AsMeter();
}

void QmlRoutingProfile::setPenaltySameType(double d)
{
  if (penaltySameType==Meters(d)){
    return;
  }
  penaltySameType=Meters(d);
  emit update();
}

double QmlRoutingProfile::getPenaltyDifferentType() const
{
  return penaltyDifferentType.AsMeter();
}

void QmlRoutingProfile::setPenaltyDifferentType(double d)
{
  if (penaltyDifferentType==Meters(d)){
    return;
  }
  penaltyDifferentType=Meters(d);
  emit update();
}

double QmlRoutingProfile::getMaxPenalty() const
{
  return maxPenalty.count();
}

void QmlRoutingProfile::setMaxPenalty(double d)
{
  if (maxPenalty.count()==d){
    return;
  }
  maxPenalty=std::chrono::seconds((int64_t)d);
  emit update();
}

void QmlRoutingProfile::setDefaults()
{
  speedTable.clear();

  if (vehicle==FootVehicle) {
    maxSpeed=5;
    applyJunctionPenalty=false;
    maxPenalty=std::chrono::seconds::zero();
    penaltySameType=Meters(0);
    penaltyDifferentType=Meters(0);
    costLimitDistance=Kilometers(10);
    costLimitFactor=5.0;

    speedTable["highway_track"][SolidGrade]=maxSpeed*0.9;
    speedTable["highway_track"][MostlySoftGrade]=maxSpeed*0.75;
    speedTable["highway_path"][SolidGrade]=maxSpeed*0.9;
    speedTable["highway_path"][MostlySoftGrade]=maxSpeed*0.75;
    speedTable["highway_steps"][SolidGrade]=1;

  } else if (vehicle==BicycleVehicle) {
    maxSpeed=20;
    applyJunctionPenalty=true;
    maxPenalty=std::chrono::seconds(10);
    penaltySameType=Meters(40);
    penaltyDifferentType=Meters(250);
    costLimitDistance=Kilometers(20);
    costLimitFactor=7.5;

    speedTable["highway_trunk"][SolidGrade]=maxSpeed;
    speedTable["highway_trunk_link"][SolidGrade]=maxSpeed;
    speedTable["highway_primary"][SolidGrade]=maxSpeed;
    speedTable["highway_primary_link"][SolidGrade]=maxSpeed;
    speedTable["highway_secondary"][SolidGrade]=maxSpeed;
    speedTable["highway_secondary_link"][SolidGrade]=maxSpeed;
    speedTable["highway_tertiary"][SolidGrade]=maxSpeed;
    speedTable["highway_tertiary_link"][SolidGrade]=maxSpeed;
    speedTable["highway_unclassified"][SolidGrade]=maxSpeed;
    speedTable["highway_road"][SolidGrade]=maxSpeed;
    speedTable["highway_residential"][SolidGrade]=maxSpeed;
    speedTable["highway_living_street"][SolidGrade]=maxSpeed;
    speedTable["highway_service"][SolidGrade]=maxSpeed;

    speedTable["highway_track"][SolidGrade]=maxSpeed;
    speedTable["highway_track"][GravelGrade]=15;
    speedTable["highway_track"][UnpavedGrade]=12;
    speedTable["highway_track"][MostlySoftGrade]=10;
    speedTable["highway_track"][SoftGrade]=8;

    speedTable["highway_path"][SolidGrade]=12;
    speedTable["highway_path"][GravelGrade]=10;
    speedTable["highway_path"][UnpavedGrade]=9;
    speedTable["highway_path"][MostlySoftGrade]=8;
    speedTable["highway_path"][SoftGrade]=7;

    speedTable["highway_cycleway"][SolidGrade]=maxSpeed;
    speedTable["highway_roundabout"][SolidGrade]=10;

    speedTable["highway_footway"][SolidGrade]=5;

  } else { // vehicle==CarVehicle
    maxSpeed=160;
    applyJunctionPenalty=true;
    maxPenalty=std::chrono::seconds(10);
    penaltySameType=Meters(40);
    penaltyDifferentType=Meters(250);
    costLimitDistance=Kilometers(20);
    costLimitFactor=7.5;

    speedTable["highway_motorway"][SolidGrade]=110.0;
    speedTable["highway_motorway_trunk"][SolidGrade]=100.0;
    speedTable["highway_motorway_primary"][SolidGrade]=70.0;
    speedTable["highway_motorway_link"][SolidGrade]=60.0;
    speedTable["highway_motorway_junction"][SolidGrade]=60.0;
    speedTable["highway_trunk"][SolidGrade]=100.0;
    speedTable["highway_trunk_link"][SolidGrade]=60.0;
    speedTable["highway_primary"][SolidGrade]=70.0;
    speedTable["highway_primary_link"][SolidGrade]=60.0;
    speedTable["highway_secondary"][SolidGrade]=60.0;
    speedTable["highway_secondary_link"][SolidGrade]=50.0;
    speedTable["highway_tertiary"][SolidGrade]=55.0;
    speedTable["highway_tertiary_link"][SolidGrade]=55.0;
    speedTable["highway_unclassified"][SolidGrade]=50.0;
    speedTable["highway_road"][SolidGrade]=50.0;
    speedTable["highway_residential"][SolidGrade]=20.0;
    speedTable["highway_roundabout"][SolidGrade]=40.0;
    speedTable["highway_living_street"][SolidGrade]=10.0;
    speedTable["highway_service"][SolidGrade]=30.0;
  }
  emit update();
}

RoutingProfileRef QmlRoutingProfile::MakeInstance(TypeConfigRef typeConfig) const
{
  osmscout::FastestPathRoutingProfileRef routingProfile =
      std::make_shared<osmscout::FastestPathRoutingProfile>(typeConfig);

  routingProfile->SetVehicle(static_cast<Vehicle>(vehicle));
  routingProfile->SetVehicleMaxSpeed(maxSpeed);
  routingProfile->SetJunctionPenalty(applyJunctionPenalty);
  routingProfile->SetCostLimitDistance(costLimitDistance);
  routingProfile->SetCostLimitFactor(costLimitFactor);
  routingProfile->SetPenaltySameType(penaltySameType);
  routingProfile->SetPenaltyDifferentType(penaltyDifferentType);
  routingProfile->SetMaxPenalty(maxPenalty);

  for (const auto &type : typeConfig->GetTypes()) {
    if (!type->GetIgnore() &&
        type->CanRoute(static_cast<Vehicle>(vehicle))) {

      if (auto typeSpeed=speedTable.find(type->GetName());
          typeSpeed==speedTable.end()){
        routingProfile->AddType(type,maxSpeed);
      } else {
        routingProfile->AddType(type,typeSpeed->second);
      }
    }
  }

  return routingProfile;
}

}
