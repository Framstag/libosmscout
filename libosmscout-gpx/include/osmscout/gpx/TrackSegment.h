#ifndef OSMSCOUT_GPX_TRACKSEGMENT_H
#define OSMSCOUT_GPX_TRACKSEGMENT_H

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


#include <osmscout/gpx/TrackPoint.h>

#include <osmscout/gpx/GPXImportExport.h>

#include <vector>

namespace osmscout {
namespace gpx {

class OSMSCOUT_GPX_API TrackSegment {
public:
  std::vector<TrackPoint> points;

  /**
   * Compute track length
   * @return
   */
  Distance GetLength() const;
};
}
}

#endif //OSMSCOUT_GPX_TRACKSEGMENT_H
