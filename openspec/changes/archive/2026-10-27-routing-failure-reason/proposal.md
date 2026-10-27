# Proposal

## Why

A route calculation fails for several unrelated reasons, and a caller of this library can neither report
nor act on the reason because they all look alike: `RoutingResult::Success()` is `!route.IsEmpty()`, and an
empty result says nothing about why. A caller that shows a message has to guess — "no route found" is a
different thing to tell a user than "the start position is not on the road network", and a cancellation is
not a failure at all.

The same resolution has a second gap. A position is attached to a routable way by looking for a route node
on that way, and the two ends of a route search **opposite halves** of the way: the start searches forward
from the attach index, the target backward from it (a position can therefore attach as a start and not as a
target on a direction-restricted way such as a one-way street or a slip road). The resolution that picks the
closest routable way did not know which end it resolved for, so it could select a way that the end cannot
attach to — and the whole calculation then failed, although a way a little further away could have carried
the position.

## What Changes

- `RoutingResult` SHALL carry a `RoutingError` naming why it carries no route: `None`, `NoStartNode`,
  `NoTargetNode`, `NoRouteFound`, `DataFailure` or `Aborted`, with `GetError()` / `SetError()`.
  `Success()` SHALL keep meaning "the route is not empty", so a caller that only asks whether the
  calculation worked keeps its behaviour.
- The routing service SHALL resolve the closest routable node **for a route end**, named by a new
  `RouteEndpoint` (`Start`, `Target`, `Unspecified`): a way the position cannot be attached to for that end
  SHALL be skipped in favour of the next one within the radius instead of failing the calculation.
- `Unspecified` SHALL apply no end-specific restriction and SHALL select exactly what the resolution
  selected before the parameter existed, so the existing signature keeps its behaviour.
- The multi-database resolution SHALL ask every loaded database for an end-attachable way and take the
  closest candidate that can carry the position for that end, so the database whose nearest way is unusable
  does not fail the calculation.
- Every failure branch of the calculation SHALL set the reason: an unattachable end, an exhausted search, a
  breaker abort, and a failure to read the routing data or to convert the result.

## Capabilities

### New Capabilities

- `routing-failure-reporting`: the reason a route calculation carries no route, and the resolution of a
  routable node for a named route end.

### Modified Capabilities

<!-- None. The existing resolution signatures keep their behaviour; the new one is an overload. -->

## Impact

Affected files and modules (`libosmscout/`):

- `libosmscout/include/osmscout/routing/AbstractRoutingService.h` — `RouteEndpoint`, `RoutingError`, the
  error on `RoutingResult` and its accessors.
- `libosmscout/src/osmscout/routing/AbstractRoutingService.cpp` — the reason on every failure branch.
- `libosmscout/include/osmscout/routing/SimpleRoutingService.h` — `IsEndpointAttachable` and the
  endpoint-carrying `GetClosestRoutableNode`.
- `libosmscout/src/osmscout/routing/SimpleRoutingService.cpp` — the attachability decision and the
  way selection that skips a way the end cannot attach to.
- `libosmscout/include/osmscout/routing/MultiDBRoutingService.h` — the endpoint-carrying
  `GetClosestRoutableNode`.
- `libosmscout/src/osmscout/routing/MultiDBRoutingService.cpp` — the endpoint passed to every loaded
  database.

No new dependency. No file format or type configuration change, so no `FileFormatVersion.md` version bump
applies. No existing signature changes — the endpoint-carrying resolution is an overload and the
pre-existing ones delegate to it with `RouteEndpoint::Unspecified`.

Verified by `MultiDBRoutingTest` (which routes inside the repository's `Tests/data/testregion`) and by the
build of both build systems. The reference has no unit test for the attachability decision itself: it needs
a routing database with route nodes and a routing profile, which only the routing test data provides; the
end-to-end test exercises the path the decision sits on.
