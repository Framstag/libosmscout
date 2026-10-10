/*
  This source is part of the libosmscout-map library
  Copyright (C) 2017  Fanny Monori

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscoutmapopengl/Triangulate.h>

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include <osmscout/log/Logger.h>
#include <osmscout/util/ScopeGuard.h>

#include <poly2tri/poly2tri.h>

namespace osmscout {

namespace {
  /**
   * Distance below which the cross product of three points counts as zero, i.e. as the three points
   * being collinear so that the middle one is no corner of the ring.
   */
  constexpr double CornerTolerance=1e-12;

  /**
   * Removes the points that carry no corner of the ring before it is handed to the triangulator: a
   * point that repeats its predecessor, a closing point that repeats the first point (both make a
   * zero-length edge, which the vendored triangulator asserts on and dereferences once the assert is
   * compiled out), and a point whose two neighbours and itself are collinear. Returns false when
   * fewer than three distinct corners remain, which the callers report as a rejection.
   *
   * The removal does not change the area the ring covers, so the triangles of an accepted polygon are
   * the ones it was triangulated into before.
   */
  bool NormalizeRing(const std::vector<Vertex2D>& ring,
                     std::vector<Vertex2D>& normalized)
  {
    normalized.clear();

    for (const auto& point : ring) {
      if (!normalized.empty() && normalized.back()==point) {
        continue;
      }

      normalized.push_back(point);
    }

    while (normalized.size()>1 && normalized.front()==normalized.back()) {
      normalized.pop_back();
    }

    bool removed=true;

    while (removed && normalized.size()>=3) {
      removed=false;

      for (size_t i=0; i<normalized.size(); i++) {
        const Vertex2D& previous=normalized[(i+normalized.size()-1)%normalized.size()];
        const Vertex2D& current=normalized[i];
        const Vertex2D& next=normalized[(i+1)%normalized.size()];

        double cross=(current.GetX()-previous.GetX())*(next.GetY()-previous.GetY())-
                     (current.GetY()-previous.GetY())*(next.GetX()-previous.GetX());

        if (std::fabs(cross)<=CornerTolerance) {
          normalized.erase(normalized.begin()+static_cast<std::ptrdiff_t>(i));
          removed=true;
          break;
        }
      }
    }

    return normalized.size()>=3;
  }

  std::vector<Vertex2D> ToRing(const std::vector<Point>& points)
  {
    std::vector<Vertex2D> ring;

    ring.reserve(points.size());

    for (const auto& point : points) {
      ring.emplace_back(point.GetLon(),point.GetLat());
    }

    return ring;
  }

  std::vector<p2t::Point *> ToPolyline(const std::vector<Vertex2D>& ring)
  {
    std::vector<p2t::Point *> polyline;

    polyline.reserve(ring.size());

    for (const auto& point : ring) {
      polyline.push_back(new p2t::Point(point.GetX(),point.GetY()));
    }

    return polyline;
  }

  void DeletePolyline(std::vector<p2t::Point *>& polyline)
  {
    for (auto point : polyline) {
      delete point;
    }

    polyline.clear();
  }

  std::vector<GLfloat> TrianglesOf(p2t::CDT& cdt)
  {
    std::vector<GLfloat> result;

    for (const auto triangle : cdt.GetTriangles()) {
      p2t::Point a=*triangle->GetPoint(0);
      p2t::Point b=*triangle->GetPoint(1);
      p2t::Point c=*triangle->GetPoint(2);

      result.emplace_back(a.x);
      result.emplace_back(a.y);
      result.emplace_back(b.x);
      result.emplace_back(b.y);
      result.emplace_back(c.x);
      result.emplace_back(c.y);
    }

    return result;
  }
} // namespace

  std::vector<GLfloat> osmscout::Triangulate::TriangulatePolygon(std::vector<osmscout::Vertex2D> points) {
    std::vector<Vertex2D> ring;

    if (!NormalizeRing(points,ring)) {
      log.Warn() << "Skipping a polygon of " << points.size() << " points: fewer than three distinct corners remain";
      return {};
    }

    std::vector<p2t::Point *> polyline=ToPolyline(ring);
    ScopeGuard polylineDeleter([&polyline]() noexcept {
      DeletePolyline(polyline);
    });

    try {
      p2t::CDT cdt(polyline);

      cdt.Triangulate();

      return TrianglesOf(cdt);
    } catch (const std::runtime_error& e) {
      log.Warn() << "Skipping a polygon of " << points.size() << " points: triangulation failed: " << e.what();
      return {};
    }
  }

  std::vector<GLfloat> osmscout::Triangulate::TriangulatePolygon(std::vector<osmscout::Point> points) {
    return TriangulatePolygon(ToRing(points));
  }

  std::vector<GLfloat> osmscout::Triangulate::TriangulatePolygon(std::vector<osmscout::GeoCoord> points) {
    std::vector<Vertex2D> ring;

    ring.reserve(points.size());

    for (const auto& coord : points) {
      ring.emplace_back(coord.GetLon(),coord.GetLat());
    }

    return TriangulatePolygon(ring);
  }

  std::vector<GLfloat> osmscout::Triangulate::TriangulateWithHoles(std::vector<std::vector<osmscout::Point>> points) {
    if (points.empty()) {
      log.Warn() << "Skipping a polygon with holes: no ring given";
      return {};
    }

    std::vector<Vertex2D> outer;

    if (!NormalizeRing(ToRing(points[0]),outer)) {
      log.Warn() << "Skipping a polygon with holes of " << points.size() << " rings: the outer ring has fewer than three distinct corners";
      return {};
    }

    std::vector<p2t::Point *> polyline=ToPolyline(outer);
    ScopeGuard polylineDeleter([&polyline]() noexcept {
      DeletePolyline(polyline);
    });

    std::vector<std::vector<p2t::Point *>> holes;
    ScopeGuard holesDeleter([&holes]() noexcept {
      for (auto& hole : holes) {
        DeletePolyline(hole);
      }
    });

    for (size_t i=1; i<points.size(); i++) {
      std::vector<Vertex2D> hole;

      if (!NormalizeRing(ToRing(points[i]),hole)) {
        log.Warn() << "Skipping a hole of " << points[i].size() << " points: fewer than three distinct corners remain";
        continue;
      }

      holes.push_back(ToPolyline(hole));
    }

    try {
      p2t::CDT cdt(polyline);

      for (auto& hole : holes) {
        cdt.AddHole(hole);
      }

      cdt.Triangulate();

      return TrianglesOf(cdt);
    } catch (const std::runtime_error& e) {
      log.Warn() << "Skipping a polygon with holes of " << points.size() << " rings: triangulation failed: " << e.what();
      return {};
    }
  }
}
