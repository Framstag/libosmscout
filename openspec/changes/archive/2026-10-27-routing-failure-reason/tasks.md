# Tasks

## 1. The reason on the result (spec: routing-failure-reporting)

- [x] 1.1 In `libosmscout/include/osmscout/routing/AbstractRoutingService.h` add `enum class RoutingError`
  (`None`, `NoStartNode`, `NoTargetNode`, `NoRouteFound`, `DataFailure`, `Aborted`) with the comment that
  names the cause rather than the remedy, and give `RoutingResult` the error member plus `GetError()` and
  `SetError()`. Verify: `Success()` is still `!route.IsEmpty()` and no existing caller changes.
- [x] 1.2 In `libosmscout/src/osmscout/routing/AbstractRoutingService.cpp` set the reason on every branch
  that returns without a route: the unattachable target and start, the abort checks, the two data-read
  failures, the "no route found" end, and the failure to convert the result to route data. Verify: no return
  path that carries no route is left at `None`.

## 2. The endpoint-aware resolution (spec: routing-failure-reporting)

- [x] 2.1 Add `enum class RouteEndpoint` (`Start`, `Target`, `Unspecified`) to
  `AbstractRoutingService.h`, documented as which half of the way the end searches, and that `Unspecified`
  is the behaviour of the resolutions that predate it.
- [x] 2.2 In `libosmscout/include/osmscout/routing/SimpleRoutingService.h` declare `IsEndpointAttachable`
  and the endpoint-carrying `GetClosestRoutableNode`; keep the existing signature and let it delegate with
  `RouteEndpoint::Unspecified`.
- [x] 2.3 In `libosmscout/src/osmscout/routing/SimpleRoutingService.cpp` implement `IsEndpointAttachable`
  (the attach index first, then the directions the profile permits for that end; `Unspecified` returns true)
  and make the way selection ask it once per way, taking the next way within the radius when it fails. Keep
  the index ranges in step with `GetStartForwardRouteNode` / `GetStartBackwardRouteNode` /
  `GetTargetForwardRouteNode` / `GetTargetBackwardRouteNode`, and say so in the comment.
- [x] 2.4 In `MultiDBRoutingService.h` / `.cpp` add the endpoint-carrying overload and pass the end to every
  loaded database; keep the existing signature delegating with `Unspecified`.

## 3. Verification

- [x] 3.1 Build `libosmscout` and the tests in the CMake build with explicit targets; verify no errors and
  no new warnings from the touched files.
- [x] 3.2 Build the same in the Meson build directory; verify it compiles.
- [x] 3.3 Rebuild and run `MultiDBRoutingTest` (it routes inside `Tests/data/testregion`) and verify it
  passes: the endpoint-aware resolution still selects a routable way and the calculation still produces a
  route.
- [x] 3.4 Run the other tests the change can reach (the routing and location tests that do not need a v27
  map database) and verify no regression. `OSMScoutClientBasemapConfigTest.testClearingBasemapLookupDirectoryUnloads`
  is a known intermittent flake (passes in isolation): document, do not fix, do not block.
- [x] 3.5 State in `verification.md` that the reference carries no unit test for the attachability decision
  and why (it needs a routing database with route nodes plus a routing profile; `MultiDBRoutingTest` covers
  the path end to end).
- [x] 3.6 Run `openspec validate "routing-failure-reason" --strict`.

## 4. Change hygiene

- [x] 4.1 Verify the diff touches only the six files in the proposal's Impact section.
- [x] 4.2 Verify no existing signature changed: `GetClosestRoutableNode(coord, radius)` and
  `GetClosestRoutableNode(coord, profile, radius)` still exist and delegate with `Unspecified`.
