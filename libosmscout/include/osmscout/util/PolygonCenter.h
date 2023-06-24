#ifndef OSMSCOUT_POLYGONCENTER_H
#define OSMSCOUT_POLYGONCENTER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2022 Lukas Karas
  Copyright (C) 2016 Mapbox

  ISC License (compatible with GNU LGPL)

  Permission to use, copy, modify, and/or distribute this software for any purpose
  with or without fee is hereby granted, provided that the above copyright notice
  and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD TO
  THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
  OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
  ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
  SOFTWARE.
 */

#include <osmscout/lib/CoreImportExport.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/Point.h>
#include <osmscout/Area.h>

#include <vector>

namespace osmscout {

/**
 * \ingroup Util
 *
 * A fast algorithm for finding polygon pole of inaccessibility, the most distant internal point from the polygon
 * outline (not to be confused with centroid).
 * Useful for optimal placement of a text label on a polygon.
 *
 * Based on PolyLabel algorithm from Mapbox
 *  - https://blog.mapbox.com/a-new-algorithm-for-finding-a-visual-center-of-a-polygon-7c77e6492fbc
 *  - https://github.com/mapbox/polylabel
 */
extern OSMSCOUT_API GeoCoord PolygonCenter(const Area& area, double precision = 1);

extern OSMSCOUT_API GeoCoord PolygonCenter(const std::vector<Point>& polygon, double precision = 1);

}
#endif //OSMSCOUT_POLYGONCENTER_H
