/*
 * route_start.h — pure helpers for the position a calculated route publishes its
 * start at (NaviVeylin change "publish-requested-route-start"; tracker issue #82,
 * TODO §130).
 *
 * The router uses the caller's start coordinate only to find a routable node and
 * then discards it: ResolveRoutePosition snaps it, TransformRouteDataToPoints
 * publishes the snapped node's geometry, and the description's first node is that
 * same node. A start that lies off the network — a station, a POI, a coordinate
 * pasted into the search field, a GPS fix at the edge of the map data — therefore
 * published the polyline, the published total and the first step tens to hundreds
 * of metres from where the user asked to depart.
 *
 * The rule here: the published route begins at the requested start. The approach
 * from it to the snapped node becomes the polyline's first segment, it is added to
 * every later description node's cumulative distance (so the per-step distances,
 * the description total and the step list keep agreeing), and it is added to the
 * published total. A requested start within the snap tolerance is not prepended, so
 * an ordinary route — a reroute from the current position, an object start that
 * resolves to a node — publishes the routed geometry unchanged.
 *
 * The approach is a great-circle length on a sphere; the drawn polyline's length is
 * measured by the caller with the library's ellipsoidal distance. The two differ by
 * far less than the tolerance's own margin, which is why the tolerance is a
 * decision bound rather than a length: the metres this header returns are the
 * metres the published numbers use, and the segment drawn between the same two
 * positions is the same length within rounding.
 *
 * Kept dependency-free on purpose (no libosmscout includes) so it can be
 * unit-tested on the host without a database or JNI — the shape of route_length.h
 * beside it.
 */
#ifndef NAVIVEYLIN_ROUTE_START_H
#define NAVIVEYLIN_ROUTE_START_H

#include <cmath>
#include <cstddef>
#include <numbers>

namespace naviveylin {

// The snap tolerance, in metres. A requested start at or below this distance from
// the snapped node is treated as being on the network: nothing is prepended, no
// zero-length first segment is published. One metre is below the resolution of the
// coordinates the router snaps and far below what a drawn route can show.
inline constexpr double kRouteStartSnapToleranceMeters = 1.0;

// Mean Earth radius, in metres — the sphere RouteStartApproachMeters measures on.
inline constexpr double kRouteStartEarthRadiusMeters = 6371000.0;

// The great-circle length between two positions, in metres. A position that is not
// finite yields 0: an unusable coordinate must not make an approach appear, because
// the prepend decision rests on this number.
inline double RouteStartApproachMeters(double fromLat, double fromLon,
                                       double toLat, double toLon)
{
  if (!std::isfinite(fromLat) || !std::isfinite(fromLon) ||
      !std::isfinite(toLat) || !std::isfinite(toLon)) {
    return 0.0;
  }

  const double lat1 = fromLat * std::numbers::pi / 180.0;
  const double lat2 = toLat * std::numbers::pi / 180.0;
  const double deltaLat = (toLat - fromLat) * std::numbers::pi / 180.0;
  const double deltaLon = (toLon - fromLon) * std::numbers::pi / 180.0;

  const double sinLat = std::sin(deltaLat / 2.0);
  const double sinLon = std::sin(deltaLon / 2.0);

  double haversine = sinLat * sinLat + std::cos(lat1) * std::cos(lat2) * sinLon * sinLon;

  // Rounding can push the sum a hair past 1 on a long antipodal distance, where the
  // square root of a negative number would be NaN.
  if (haversine > 1.0) {
    haversine = 1.0;
  }

  return 2.0 * kRouteStartEarthRadiusMeters * std::asin(std::sqrt(haversine));
}

// Whether the approach from the requested start to the snapped node is published as
// the route's first segment. Strictly greater than the tolerance: a start exactly at
// the tolerance is on the network and publishes the routed vertex (spec
// `osmscout-jni` — "A start on the network is published unchanged").
inline bool ShouldPrependRouteStart(double approachMeters)
{
  return approachMeters > kRouteStartSnapToleranceMeters;
}

// A description node's cumulative distance after the approach has been added.
//
// The start node (index 0) keeps the distance it has — zero for a description that
// begins at the route's start — because that value is what makes step 0 read as the
// route's start and own no leg. Every later node carries the approach, so the first
// leg's delta, the description total and the per-step sums all include it.
inline double RouteStartNodeDistanceMeters(std::size_t nodeIndex,
                                           double nodeDistanceMeters,
                                           double approachMeters)
{
  if (nodeIndex == 0) {
    return nodeDistanceMeters;
  }

  return nodeDistanceMeters + approachMeters;
}

} // namespace naviveylin

#endif // NAVIVEYLIN_ROUTE_START_H
