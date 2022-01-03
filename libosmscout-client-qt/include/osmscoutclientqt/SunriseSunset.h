#ifndef OSMSCOUT_CLIENT_QT_SUNRISESUNSET_H
#define OSMSCOUT_CLIENT_QT_SUNRISESUNSET_H

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

#include <osmscoutclientqt/ClientQtImportExport.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/util/SunriseSunset.h>

#include <QObject>
#include <QDateTime>
#include <QTimer>

#include <cmath>

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * QML Component for computing today's sunrise / sunset time for specific place on Earth.
 */
class OSMSCOUT_CLIENT_QT_API SunriseSunset : public QObject {
  Q_OBJECT
  Q_PROPERTY(double latitude READ getLatitude WRITE setLatitude)
  Q_PROPERTY(double longitude READ getLongitude WRITE setLongitude)
  Q_PROPERTY(bool ready READ isReady NOTIFY updated)
  Q_PROPERTY(bool day READ isDay NOTIFY updated)
  Q_PROPERTY(QDateTime sunrise READ getSunrise NOTIFY updated)
  Q_PROPERTY(QDateTime sunset READ getSunset NOTIFY updated)

signals:
  void updated();

public slots:
  void tick();

public:
  SunriseSunset();
  SunriseSunset(const SunriseSunset &) = delete;
  SunriseSunset(SunriseSunset &&) = delete;

  ~SunriseSunset() override = default;

  SunriseSunset& operator=(const SunriseSunset&) = delete;
  SunriseSunset& operator=(SunriseSunset&&) = delete;

  double getLatitude() const
  {
    return position.GetLat();
  }

  void setLatitude(double lat);

  double getLongitude() const
  {
    return position.GetLon();
  }

  void setLongitude(double lon);

  bool isReady() const
  {
    return !std::isnan(updatePosition.GetLat()) && !std::isnan(updatePosition.GetLon());
  }

  QDateTime getSunrise() const
  {
    return getValue<0>();
  }

  QDateTime getSunset() const
  {
    return getValue<1>();
  }

  bool isDay() const;

private:
  void update();
  void update(const GeoCoord &newPosition);

  template <size_t i>
  QDateTime getValue() const
  {
    if (!sunriseSunset.has_value()) {
      return QDateTime();
    }
    using namespace std::chrono;
    Timestamp ts=std::get<i>(sunriseSunset.value());
    milliseconds millisSinceEpoch = duration_cast<milliseconds>(ts.time_since_epoch());
    return QDateTime::fromMSecsSinceEpoch(millisSinceEpoch.count());
  }

private:
  GeoCoord position{NAN, NAN};
  GeoCoord updatePosition{NAN, NAN};
  QDate updateDate;
  SunriseSunsetRes sunriseSunset=std::nullopt;
  QTimer updateTimer;
};

}

#endif //OSMSCOUT_CLIENT_QT_SUNRISESUNSET_H
