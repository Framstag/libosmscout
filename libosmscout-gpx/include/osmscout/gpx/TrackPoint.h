#ifndef OSMSCOUT_GPX_TRACKPOINT_H
#define OSMSCOUT_GPX_TRACKPOINT_H

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

#include <osmscout/GeoCoord.h>
#include <osmscout/util/String.h>

#include "GPXImportExport.h"

#include <chrono>
#include <optional>

namespace osmscout {
namespace gpx {

class OSMSCOUT_GPX_API TrackPoint {
public:

  inline explicit TrackPoint(const GeoCoord coord) :
      coord(coord) {
  }

  osmscout::GeoCoord coord;
  std::optional<double> elevation; // meters above sea
  std::optional<Timestamp> time;
  std::optional<double> course; // alias magvar - degrees, 0.0 <= value < 360.0

  /**
   * Dilution of precision (horizontal, vertical, position)
   * don't provide information about approximate inaccuracy in meters!
   * https://en.wikipedia.org/wiki/Dilution_of_precision_(navigation)
   *
   * See https://gis.stackexchange.com/a/97779 :
   * "HDOP is calculated mathematically from the positions of the satellites
   * and at the same time, at the same place HDOP is the same for all receivers."
   * Inaccuracy is computed from signal strength, used satelites
   * and receiver accuracy...
   *
   * But majority of high-level location apis (Qt for example)
   * provides estimated inaccuracy in meters and hide raw dilution values.
   * Sadly, GPX format don't provide elements to store computed
   * inaccuracy in meters, so majority of software store accuracy
   * to dilution tags.
   *
   * So keep in mind, that you can't be sure what is meaning
   * of these values :-(
   * ...I think that it is ok to assume that hdop/vdop represents
   * accuracy in meters.
   */
  std::optional<double> hdop;
  std::optional<double> vdop;
  std::optional<double> pdop;
};
}
}

#endif //OSMSCOUT_GPX_TRACKPOINT_H
