# route-calculation

JNI bridge to libosmscout routing engine for async route calculation between two coordinates.

## Purpose

Expose `MultiDBRoutingService` routing to Java via JNI. Provide async calculation with progress reporting, cancellation, and structured route results.

## Requirements

### Requirement: Route calculation via JNI (async with progress)

`OSMScoutClient` SHALL expose native methods for async route calculation:
- `calculateRouteAsync(double startLat, double startLon, double destLat, double destLon, RouteCallback callback)` — starts routing on background thread with default car profile
- `calculateRouteAsync(double startLat, double startLon, double destLat, double destLon, RoutingProfile profile, RouteCallback callback)` — starts routing with specified vehicle profile
- `cancelRoute()` — cancels an in-progress route calculation

The JNI implementation SHALL use `MultiDBRoutingService::GetClosestRoutableNode()` to resolve start/target to routable nodes, `CalculateRoute()` with a `Breaker` and `RoutingProgress` to compute the path, and `TransformRouteDataToPoints()` to extract route geometry. The `RoutingProfile` SHALL select the appropriate speed map and routing profile. Progress SHALL be reported via JNI callback to Java.

#### Scenario: Calculate route between two valid coordinates
- **WHEN** `calculateRouteAsync()` is called with two valid coordinates within the routing graph
- **THEN** `onSuccess` callback is invoked with a non-empty `RouteEntry[]` containing route geometry coords and metadata

#### Scenario: Calculate bicycle route
- **WHEN** `calculateRouteAsync()` is called with two valid coordinates and `RoutingProfile(Vehicle.BICYCLE)`
- **THEN** the route is calculated using bicycle-compatible ways (cycleways, paths, residential streets)
- **AND** the estimated duration reflects 15 km/h average speed

#### Scenario: Calculate pedestrian route
- **WHEN** `calculateRouteAsync()` is called with two valid coordinates and `RoutingProfile(Vehicle.PEDESTRIAN)`
- **THEN** the route is calculated using pedestrian-compatible ways (footways, paths, pedestrian zones)
- **AND** the estimated duration reflects 5 km/h average speed

#### Scenario: Progress reported during calculation
- **WHEN** route calculation is in progress
- **THEN** `onProgress` callback is invoked periodically with a percentage value (0-100)

#### Scenario: Cancel route calculation
- **WHEN** user clicks cancel button during route calculation
- **THEN** `cancelRoute()` is called, setting the `Breaker`
- **AND** `onCancel` callback is invoked
- **AND** the routing algorithm stops

#### Scenario: Calculate route with unreachable destination
- **WHEN** `calculateRouteAsync()` is called with coordinates where no route exists
- **THEN** `onError` callback is invoked with an error message

#### Scenario: Calculate route with invalid coordinates
- **WHEN** `calculateRouteAsync()` is called with invalid coordinates
- **THEN** `onError` callback is invoked immediately

#### Scenario: Avoid tolls affects route
- **WHEN** `calculateRouteAsync()` is called with `RoutingProfile(Vehicle.CAR, true, false, false)`
- **THEN** the calculated route avoids toll roads where possible
- **AND** if no toll-free route exists, the shortest toll road route is returned with a note

### Requirement: RouteEntry Java data class

A new Java class `RouteEntry` SHALL be added to `libosmscout-client-java/java/com/framstag/libosmscout/client/` with public fields: `double[] latitudes`, `double[] longitudes`, `double distance` (meters), `double duration` (seconds), `String[] descriptions` (turn-by-turn description lines).

#### Scenario: RouteEntry holds route geometry
- **WHEN** a route is calculated
- **THEN** `latitudes` and `longitudes` arrays contain the route waypoints in order from start to destination

#### Scenario: RouteEntry holds route metadata
- **WHEN** a route is calculated
- **THEN** `distance` contains the total route length in meters and `duration` contains estimated travel time in seconds

#### Scenario: RouteEntry holds turn-by-turn description
- **WHEN** a route is calculated
- **THEN** `descriptions` array contains columnar text lines with distance, time, and turn instructions

### Requirement: RouteCallback Java interface

A new Java interface `RouteCallback` SHALL be added with methods: `onProgress(int percent)`, `onSuccess(RouteEntry route)`, `onError(String message)`, `onCancel()`. This SHALL be passed to `calculateRouteAsync()` and invoked from JNI.

#### Scenario: Callback methods invoked from JNI
- **WHEN** route calculation progresses, completes, fails, or is cancelled
- **THEN** the corresponding callback method is called on the Java thread

### Requirement: Progress reports to Java are throttled and moved-driven

While an asynchronous route calculation runs, the bridge SHALL hand a progress value to Java only when the
reported percentage has changed and at most once per documented rate-limit interval. The first report of a
calculation SHALL be handed over immediately. The reported percentage SHALL be non-decreasing for one
calculation and SHALL be capped below completion, completion being delivered as the calculation's result.

#### Scenario: An unchanged percentage is not reported again

- **GIVEN** a running route calculation whose progress has not moved since the last report
- **WHEN** the routing engine reports progress for further edges
- **THEN** no further progress callback SHALL be made to Java

#### Scenario: A report inside the rate-limit interval is dropped

- **GIVEN** a progress report that was handed to Java
- **WHEN** the percentage changes again inside the rate-limit interval
- **THEN** that change SHALL NOT be handed to Java
- **AND** a later change after the interval SHALL be handed to Java

#### Scenario: The first report is not delayed

- **GIVEN** a route calculation that has just started reporting progress
- **WHEN** the first percentage becomes available
- **THEN** it SHALL be handed to Java without waiting for the rate-limit interval

#### Scenario: Progress never reports completion

- **GIVEN** a running route calculation whose progress approaches the whole route
- **WHEN** progress is reported
- **THEN** the reported percentage SHALL NOT be the completed value
- **AND** completion SHALL be delivered as the calculation's result

#### Scenario: Progress does not go backwards

- **GIVEN** a running route calculation
- **WHEN** successive progress reports are handed to Java
- **THEN** each reported percentage SHALL be at least the previously reported one

#### Scenario: A long calculation still reports progress

- **GIVEN** a route long enough that its calculation takes longer than the rate-limit interval
- **WHEN** the calculation runs
- **THEN** Java SHALL receive more than one progress report
- **AND** the reports SHALL describe a growing percentage

### Requirement: The published route length is a length of that route

A calculated route's published length SHALL be a length of that route, SHALL NOT be the start/target air-line
estimate used for the cost limit and the progress denominator, and SHALL be derived from the description's
own total (the cumulative distance at its last node) when the description produced one, else from the
great-circle length of the published route geometry. The estimated duration SHALL be derived from the
published length.

#### Scenario: The description's total is the published length

- **GIVEN** a route whose description produced an own total
- **WHEN** the route is delivered to Java
- **THEN** its published length SHALL equal the cumulative distance at the description's last node
- **AND** the description's steps SHALL cover that total (with `client-java-route-instruction-metrics`,
  the per-step leg distances sum to it exactly)

#### Scenario: The published geometry provides the length when the description does not

- **GIVEN** a route whose description produced no length but which publishes more than one geometry point
- **WHEN** the route is delivered to Java
- **THEN** its published length SHALL be the great-circle length of the published geometry
- **AND** it SHALL be greater than zero

#### Scenario: The air-line estimate is never a published length

- **GIVEN** a route whose start and target lie far apart with a long detour between them
- **WHEN** the route is delivered to Java
- **THEN** its published length SHALL be at least the air-line distance between its start and target
- **AND** it SHALL NOT equal the router's under-counted estimate

#### Scenario: The duration follows the published length

- **GIVEN** a route whose length is decided as above
- **WHEN** the route is delivered to Java
- **THEN** its estimated duration SHALL be derived from that published length
- **AND** SHALL NOT be derived from the air-line estimate

#### Scenario: A route without a usable length reports none

- **GIVEN** a route whose description produced no length and which publishes at most one geometry point
- **WHEN** the route is delivered to Java
- **THEN** its published length SHALL be zero
- **AND** the call SHALL NOT fault
