/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2022 Lukas Karas

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

#include <osmscoutclientqt/SunriseSunset.h>

#include <QDebug>

namespace osmscout {

SunriseSunset::SunriseSunset()
{
  updateTimer.setSingleShot(true);

  connect(&updateTimer, &QTimer::timeout,
          this, &SunriseSunset::tick);
}

void SunriseSunset::tick()
{
  if (updateDate!=QDate::currentDate()) {
    update();
  } else {
    emit updated();
  }
}

void SunriseSunset::update()
{
  using namespace std::chrono;
  Timestamp now=system_clock::now();
  milliseconds nowMillis = duration_cast<milliseconds>(now.time_since_epoch());
  QDateTime nowQt=QDateTime::fromMSecsSinceEpoch(nowMillis.count());

  updatePosition=position;
  updateDate=nowQt.date();
  if (!std::isnan(position.GetLat()) &&
      !std::isnan(position.GetLon())) {
    sunriseSunset=GetSunriseSunset(position, now);
  } else {
    sunriseSunset=std::nullopt;
  }

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0) /* For compatibility with QT 5.6 */
  QDateTime midnight=QDateTime(nowQt.date().addDays(1));
#else
  QDateTime midnight=nowQt.date().addDays(1).startOfDay();
#endif

  milliseconds nextTick=milliseconds(midnight.toMSecsSinceEpoch());
  if (sunriseSunset.has_value()) {
    milliseconds sunrise=duration_cast<milliseconds>(std::get<0>(sunriseSunset.value()).time_since_epoch());
    milliseconds sunset=duration_cast<milliseconds>(std::get<1>(sunriseSunset.value()).time_since_epoch());
    if (sunrise > nowMillis && sunrise < nextTick){
      nextTick=sunrise + milliseconds(1);
    }
    if (sunset > nowMillis && sunset < nextTick){
      nextTick=sunset + milliseconds(1);
    }
  }
  updateTimer.setInterval(nextTick.count() - nowMillis.count());
  updateTimer.start();

  qDebug() << "Sunrise on" << QString::fromStdString(position.GetDisplayText())
           << "at" << updateDate << ":" << getSunrise() << ", sunset:" << getSunset()
           << ", next tick: " << QDateTime::fromMSecsSinceEpoch(nextTick.count());

  emit updated();
}

void SunriseSunset::update(const GeoCoord &newPosition)
{
  if (position==newPosition) {
    return;
  }
  position=newPosition;
  if (!std::isnan(updatePosition.GetLat()) &&
      !std::isnan(updatePosition.GetLon()) &&
      !std::isnan(position.GetLat()) &&
      !std::isnan(position.GetLon()) &&
      updatePosition.GetDistance(position) < Kilometers(50.0)) {
    return;
  }

  update();
}

void SunriseSunset::setLatitude(double lat)
{
  update(GeoCoord(lat, position.GetLon()));
}

void SunriseSunset::setLongitude(double lon)
{
  update(GeoCoord(position.GetLat(), lon));
}

bool SunriseSunset::isDay() const
{
  if (!sunriseSunset.has_value()) {
    return false;
  }
  using namespace std::chrono;
  auto now=system_clock::now();
  return std::get<0>(sunriseSunset.value()) < now &&
         now < std::get<1>(sunriseSunset.value());
}


}
