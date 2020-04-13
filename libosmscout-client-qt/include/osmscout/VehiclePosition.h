#ifndef OSMSCOUT_CLIENT_QT_VEHICLEPOSITION_H
#define OSMSCOUT_CLIENT_QT_VEHICLEPOSITION_H

/*
 OSMScout - a Qt backend for libosmscout and libosmscout-map
 Copyright (C) 2019 Lukas Karas

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

#include <osmscout/ClientQtImportExport.h>

#include <osmscout/navigation/PositionAgent.h>
#include <osmscout/util/Bearing.h>

#include <QObject>

#include <memory>
#include <optional>

namespace osmscout {

/**
 * \ingroup QtAPI
 *
 * Object aggregating estimated data about vehicle during navigation.
 */
class OSMSCOUT_CLIENT_QT_API VehiclePosition: public QObject
{
  Q_OBJECT

  Q_PROPERTY(double   lat       READ getLat             CONSTANT)
  Q_PROPERTY(double   lon       READ getLon             CONSTANT)
  Q_PROPERTY(double   bearing   READ getBearingRadians  CONSTANT)

public:
  inline explicit VehiclePosition(QObject *parent = 0) :
    QObject(parent)
  {}

  inline VehiclePosition(const Vehicle &vehicle,
                         const PositionAgent::PositionState &state,
                         const GeoCoord &coord,
                         const std::optional<Bearing> &bearing,
                         const std::optional<GeoCoord> &nextStepCoord,
                         QObject *parent = 0):
      QObject(parent), vehicle(vehicle), state(state), coord(coord), bearing(bearing), nextStepCoord(nextStepCoord)
  {}

  inline VehiclePosition & operator=(const VehiclePosition &o)
  {
    vehicle=o.vehicle;
    state=o.state;
    coord=o.coord;
    bearing=o.bearing;
    nextStepCoord=o.nextStepCoord;
    return *this;
  }

  inline double getLat() const
  {
    return coord.GetLat();
  }

  inline double getLon() const
  {
    return coord.GetLon();
  }

  inline GeoCoord getCoord() const
  {
    return coord;
  }

  inline std::optional<Bearing> getBearing() const
  {
    return bearing;
  }

  inline double getBearingRadians() const
  {
    return bearing ? bearing->AsRadians() : 0;
  }

  inline std::optional<GeoCoord> getNextStepCoord() const
  {
    return nextStepCoord;
  }

  inline PositionAgent::PositionState getState() const
  {
    return state;
  }

private:
  Vehicle vehicle;
  PositionAgent::PositionState state;
  GeoCoord coord;
  std::optional<Bearing> bearing;
  std::optional<GeoCoord> nextStepCoord;
};

}

#endif //OSMSCOUT_CLIENT_QT_VEHICLEPOSITION_H
