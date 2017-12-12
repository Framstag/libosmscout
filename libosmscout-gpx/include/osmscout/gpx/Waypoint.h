#ifndef OSMSCOUT_GPX_WAYPOINT_H
#define OSMSCOUT_GPX_WAYPOINT_H

/*
  This source is part of the libosmscout-gpx library
  Copyright (C) 2017 Lukas Karas

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

#include <osmscout/gpx/Optional.h>
#include <osmscout/gpx/TrackPoint.h>

#include <osmscout/GeoCoord.h>

#include <osmscout/private/GPXImportExport.h>

#include <string>

namespace osmscout {
namespace gpx {

class OSMSCOUT_GPX_API Waypoint {
public:
  Waypoint(GeoCoord coord) :
      coord(coord) {
  }

  Optional <std::string> name;

  osmscout::GeoCoord coord;
  Optional<double> elevation; // meters above sea
  Optional<Timestamp> time;
  Optional<double> course; // degrees, 0.0 <= value < 360.0
  Optional<double> hdop; // meters
  Optional<double> vdop; // meters
  Optional<double> pdop; // meters
};
}
}

#endif //OSMSCOUT_GPX_WAYPOINT_H
