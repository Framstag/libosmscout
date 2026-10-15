/*
 * route_failure_kind.h — the failure kind the JNI bridge hands to Java for a
 * route calculation that did not produce a route.
 *
 * A failed calculation used to reach Java as one English sentence, so a caller
 * could neither pick wording for the user's language nor decide whether the
 * failure is something the user can act on. The kind is the machine-readable
 * half of that report: it names the cause, and the caller selects the display
 * text for it from its own resources (openspec change
 * "distinguish-routing-failures", spec `routing-failure-reporting`).
 *
 * **Why there is no "route too long" kind.** The router reports one error for a
 * search that exhausted its alternatives, and a caller cannot tell from it why:
 * an empty open list means the connection is longer than the profile's cost
 * limit allows *or* that the endpoints' parts of the network do not join. A
 * counter of the extensions the cost limit discarded does not separate them —
 * measured 2026-10-13 on `nordrhein-westfalen-27-20260820-0826`, a sweep of 416
 * points across Dortmund's old town produced 24 failures that had all discarded
 * extensions at the cost limit, on trips whose air-line distance was at most
 * 1.2 km. "The distance is longer than this vehicle profile allows" would have
 * been false for every one of them, so that kind was removed rather than
 * guessed at: they report `NoRouteFound`.
 *
 * Pure and dependency-free on purpose (no libosmscout include, no JNI), so the
 * decision is host-testable on its own. The translation from the router's own
 * types happens in OSMScoutClient.cpp, which is the only place that knows both.
 *
 * The kinds mirror the `KIND_*` constants of
 * `com.framstag.libosmscout.client.RouteCallback` — **keep the two in step**,
 * the number is what crosses the boundary.
 */
#ifndef NAVIVEYLIN_ROUTE_FAILURE_KIND_H
#define NAVIVEYLIN_ROUTE_FAILURE_KIND_H

namespace naviveylin {

// Mirrors RouteCallback.KIND_* — the value crosses JNI as an int.
inline constexpr int kRouteFailureUnknown=0;
inline constexpr int kRouteFailureRoutingDataUnavailable=1;
inline constexpr int kRouteFailureStartUnroutable=2;
inline constexpr int kRouteFailureTargetUnroutable=3;
inline constexpr int kRouteFailureNoRouteFound=4;
inline constexpr int kRouteFailureRoutingDataFailure=5;
inline constexpr int kRouteFailureEngineFailure=6;

/**
 * Why a route calculation produced no route, in the terms the route callback
 * understands. This is the bridge's own vocabulary, not the router's: the
 * router's `RoutingError` is translated into it where both types are known.
 */
enum class RouteFailureCause
{
  /** The cause is not known (a failure outside the router's own report). */
  Unknown,
  /** The start position could not be attached to the road network. */
  NoStartNode,
  /** The target position could not be attached to the road network. */
  NoTargetNode,
  /** The search ran and found no connection. */
  NoRouteFound,
  /** The routing data could not be read. */
  DataFailure,
  /** No routing data could be used at all (none loaded, or none could be opened). */
  RoutingDataUnavailable,
  /** The native routing work failed outside the router. */
  EngineFailure
};

/**
 * The kind to report for a failure.
 *
 * @param cause why the calculation produced no route
 * @return one of the `kRouteFailure*` kinds
 */
inline int RouteFailureKind(RouteFailureCause cause)
{
  switch (cause) {
    case RouteFailureCause::NoStartNode:
      return kRouteFailureStartUnroutable;
    case RouteFailureCause::NoTargetNode:
      return kRouteFailureTargetUnroutable;
    case RouteFailureCause::NoRouteFound:
      return kRouteFailureNoRouteFound;
    case RouteFailureCause::DataFailure:
      return kRouteFailureRoutingDataFailure;
    case RouteFailureCause::RoutingDataUnavailable:
      return kRouteFailureRoutingDataUnavailable;
    case RouteFailureCause::EngineFailure:
      return kRouteFailureEngineFailure;
    case RouteFailureCause::Unknown:
      break;
  }

  return kRouteFailureUnknown;
}

} // namespace naviveylin

#endif // NAVIVEYLIN_ROUTE_FAILURE_KIND_H
