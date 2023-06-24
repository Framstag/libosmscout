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

#include <osmscout/Area.h>
#include <osmscout/GeoCoord.h>
#include <osmscout/Point.h>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/PolygonCenter.h>
#include <osmscout/log/Logger.h>

#include <queue>
#include <vector>

namespace osmscout {

// get squared distance from a point to a segment
double getSegDistSq(const GeoCoord& p,
                    const Point& a,
                    const Point& b) {
  auto x = a.GetLon();
  auto y = a.GetLat();
  auto dx = b.GetLon() - x;
  auto dy = b.GetLat() - y;

  if (dx != 0 || dy != 0) {

    auto t = ((p.GetLon() - x) * dx + (p.GetLat() - y) * dy) / (dx * dx + dy * dy);

    if (t > 1) {
      x = b.GetLon();
      y = b.GetLat();
    } else if (t > 0) {
      x += dx * t;
      y += dy * t;
    }
  }

  dx = p.GetLon() - x;
  dy = p.GetLat() - y;

  return dx * dx + dy * dy;
}

using Ring = std::vector<Point>;
using Polygon = std::vector<const Ring*>;

// signed distance from point to polygon outline (negative if point is outside)
auto pointToPolygonDist(const GeoCoord& point, const Polygon& polygon) {
  bool inside = false;
  auto minDistSq = std::numeric_limits<double>::infinity();

  for (const auto& ring : polygon) {
    for (std::size_t i = 0, len = ring->size(), j = len - 1; i < len; j = i++) {
      const auto& a = (*ring)[i];
      const auto& b = (*ring)[j];

      if ((a.GetLat() > point.GetLat()) != (b.GetLat() > point.GetLat()) &&
          (point.GetLon() < (b.GetLon() - a.GetLon()) * (point.GetLat() - a.GetLat()) / (b.GetLat() - a.GetLat()) + a.GetLon())) {
        inside = !inside;
      }

      minDistSq = std::min(minDistSq, getSegDistSq(point, a, b));
    }
  }

  return (inside ? 1 : -1) * std::sqrt(minDistSq);
}

struct Cell {
  Cell(const GeoCoord& c_, double h_, const Polygon& polygon)
      : c(c_),
        h(h_),
        d(pointToPolygonDist(c, polygon)),
        max(d + h * std::sqrt(2))
  {}

  GeoCoord c; // cell center
  double h; // half the cell size
  double d; // distance from cell center to polygon
  double max; // max distance to polygon within a cell
};

// get polygon centroid (of first top-level outer ring)
Cell getCentroidCell(const Polygon& polygon) {
  assert(!polygon.empty());
  double area = 0;
  GeoCoord c { 0, 0 };
  const auto &ring = (*polygon[0]);

  for (std::size_t i = 0, len = ring.size(), j = len - 1; i < len; j = i++) {
    const Point& a = ring[i];
    const Point& b = ring[j];
    auto f = a.GetLon() * b.GetLat() - b.GetLon() * a.GetLat();
    c.Set(
    c.GetLat() + (a.GetLat() + b.GetLat()) * f,
    c.GetLon() + (a.GetLon() + b.GetLon()) * f);
    area += f * 3;
  }

  assert(!ring.empty());
  return Cell(area == 0 ? ring[0].GetCoord() : GeoCoord(c.GetLat() / area, c.GetLon() / area),
              0,
              polygon);
}

GeoCoord PolygonCenter(const Polygon &polygon, const GeoBox &envelope, double precision)
{
  const GeoCoord size{envelope.GetHeight(), envelope.GetWidth()};

  const double cellSize = std::min(size.GetLat(), size.GetLon());
  double h = cellSize / 2;

  // a priority queue of cells in order of their "potential" (max distance to polygon)
  auto compareMax = [] (const Cell& a, const Cell& b) {
    return a.max < b.max;
  };
  using Queue = std::priority_queue<Cell, std::vector<Cell>, decltype(compareMax)>;
  Queue cellQueue(compareMax);

  if (cellSize == 0) {
    return envelope.GetMinCoord();
  }

  // cover polygon with initial cells
  for (double lon = envelope.GetMinLon(); lon < envelope.GetMaxLon(); lon += cellSize) {
    for (double lat = envelope.GetMinLat(); lat < envelope.GetMaxLat(); lat += cellSize) {
      cellQueue.push(Cell({lat + h, lon + h}, h, polygon));
    }
  }

  // take centroid as the first best guess
  auto bestCell = getCentroidCell(polygon);

  // second guess: bounding box centroid
  Cell bboxCell(envelope.GetCenter(), 0, polygon);
  if (bboxCell.d > bestCell.d) {
    bestCell = bboxCell;
  }

  auto numProbes = cellQueue.size();
  while (!cellQueue.empty()) {
    // pick the most promising cell from the queue
    auto cell = cellQueue.top();
    cellQueue.pop();

    // update the best cell if we found a better one
    if (cell.d > bestCell.d) {
      bestCell = cell;
      log.Debug() << "found best " << ::round(1e4 * cell.d) / 1e4 << " after " << numProbes << " probes";
    }

    // do not drill down further if there's no chance of a better solution
    if (cell.max - bestCell.d <= precision) {
      continue;
    }

    // split the cell into four cells
    h = cell.h / 2;
    cellQueue.push(Cell({cell.c.GetLat() - h, cell.c.GetLon() - h}, h, polygon));
    cellQueue.push(Cell({cell.c.GetLat() - h, cell.c.GetLon() + h}, h, polygon));
    cellQueue.push(Cell({cell.c.GetLat() + h, cell.c.GetLon() - h}, h, polygon));
    cellQueue.push(Cell({cell.c.GetLat() + h, cell.c.GetLon() + h}, h, polygon));
    numProbes += 4;
  }

  log.Debug() << "num probes: " << numProbes;
  log.Debug() << "best distance: " << bestCell.d;

  return bestCell.c;
}

GeoCoord PolygonCenter(const Area &area, double precision)
{
  Polygon polygon;
  area.VisitRings([&](size_t, const Area::Ring &ring, const TypeInfoRef&) -> bool {
    polygon.push_back(&ring.nodes);
    return false; // visit just top-level outer rings
  });
  return PolygonCenter(polygon, area.GetBoundingBox(), precision);
}

GeoCoord PolygonCenter(const std::vector<Point>& ring, double precision)
{
  Polygon polygon;
  polygon.push_back(&ring);
  GeoBox bbox=osmscout::GetBoundingBox(ring);
  return PolygonCenter(polygon, bbox, precision);
}
}
