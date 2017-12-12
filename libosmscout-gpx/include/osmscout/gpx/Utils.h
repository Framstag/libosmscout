#ifndef LIBOSMSCOUT_GPX_UTILS_H
#define LIBOSMSCOUT_GPX_UTILS_H
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

#include <osmscout/private/GPXImportExport.h>
#include <osmscout/gpx/TrackPoint.h>

#include <string>
#include <memory>
#include <vector>

namespace osmscout {
namespace gpx {

class OSMSCOUT_GPX_API ProcessCallback {
public:
  virtual ~ProcessCallback(){};

  /**
   * Callback for reporting import progress
   * @param p - progress in range 0.0 .. 1.0
   */
  virtual void Progress(double p);

  /**
   * Error while importing gpx
   * @param error
   */
  virtual void Error(std::string error);
};

typedef std::shared_ptr<ProcessCallback> ProcessCallbackRef;

/**
 * Filter out following points that are not distant more than minDistance.
 * For minDistance == 0 it just remove points with same coordinates.
 *
 * @param points
 * @param minDistance in meters
 */
extern OSMSCOUT_GPX_API void FilterNearPoints(std::vector<TrackPoint> &points, double minDistance=0);

/**
 * Filter out points with horizontal dilution (or position dilution if horizontal is not presented)
 * bigger than maDilution value
 *
 * @param points
 * @param maxDilution
 */
extern OSMSCOUT_GPX_API void FilterInaccuratePoints(std::vector<TrackPoint> &points,
                                                    double maxDilution=30);

}
}

#endif //LIBOSMSCOUT_GPX_UTILS_H
