/*
 * route_length.h — pure helper for the length a calculated route publishes
 * (change "client-java-route-length").
 *
 * The length a calculated route published used to be result.GetOverallDistance():
 * the start/target air-line estimate the router computes for its cost limit and
 * its progress denominator, not a length of the route. Against the polyline the
 * same call draws it read 0.748x on a ~70 km route, 0.795x on a 20 km one and
 * 0.552x on a 1.5 km one — the caller showed a length that neither matched the
 * geometry nor the step list beside it.
 *
 * The published length is therefore decided from the route itself: the
 * description's own total (the cumulative distance at its last node, i.e. the
 * sum of its steps) when it produced one, else the great-circle length of the
 * geometry this same call publishes, else zero when neither produced a usable
 * length. The air-line estimate is deliberately not an input.
 *
 * Kept dependency-free on purpose (no libosmscout includes) so it can be
 * unit-tested on the host; the geometry length itself is measured by the caller
 * with the library's ellipsoidal distance.
 */
#ifndef NAVIVEYLIN_ROUTE_LENGTH_H
#define NAVIVEYLIN_ROUTE_LENGTH_H

namespace naviveylin {

// Decides the length a calculated route publishes, in metres. Returns the
// description's own total when it is positive, else the length of the published
// geometry when it is positive, else zero.
inline double RouteLengthMeters(double descriptionTotalMeters,
                                double geometryLengthMeters)
{
  if (descriptionTotalMeters > 0.0) {
    return descriptionTotalMeters;
  }
  if (geometryLengthMeters > 0.0) {
    return geometryLengthMeters;
  }
  return 0.0;
}

} // namespace naviveylin

#endif // NAVIVEYLIN_ROUTE_LENGTH_H
