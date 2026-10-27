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

A new Java class `RouteEntry` SHALL be added to `libosmscout-client-java/java/com/framstag/libosmscout/client/` with public fields: `double[] latitudes`, `double[] longitudes`, `double distance` (meters), `double duration` (seconds), `String[] descriptions` (turn-by-turn description lines), and the per-instruction arrays `double[] instructionLats`, `double[] instructionLons`, `double[] instructionDistances` (meters) and `double[] instructionTimes` (seconds). It SHALL additionally carry the route's manoeuvres as structured steps in `RouteStep[] steps`, each step carrying the manoeuvre's data as published by the client library's route step model, and the route's database-to-map-directory mapping.

#### Scenario: RouteEntry holds route geometry
- **WHEN** a route is calculated
- **THEN** `latitudes` and `longitudes` arrays contain the route waypoints in order from start to destination

#### Scenario: RouteEntry holds route metadata
- **WHEN** a route is calculated
- **THEN** `distance` contains the total route length in meters and `duration` contains estimated travel time in seconds

#### Scenario: RouteEntry holds turn-by-turn description
- **WHEN** a route is calculated
- **THEN** `descriptions` array contains columnar text lines with distance, time, and turn instructions

#### Scenario: Per-instruction positions are index-aligned with the instruction lines
- **WHEN** a route is calculated with a description
- **THEN** `instructionLats` and `instructionLons` SHALL carry the position of each instruction line's manoeuvre
- **AND** they SHALL be index-aligned with the instruction lines of `descriptions`, the description's header not being an instruction and not counted

#### Scenario: Per-step leg values are the legs of the route
- **WHEN** a route is calculated with a description
- **THEN** `instructionDistances` SHALL carry the leg that ends at each instruction line's manoeuvre, measured from the instruction preceding it
- **AND** `instructionTimes` SHALL carry that leg's own estimated travel time, as the description reports it
- **AND** the entries of `instructionDistances` SHALL add up to `distance`
- **AND** the entries of `instructionTimes` SHALL NOT be required to add up to `duration`: the description's per-leg times follow the road speeds of the route, while `duration` stays the length-derived estimate of `client-java-route-length` at one average speed

#### Scenario: The arrays are dropped as a set when they cannot be aligned
- **WHEN** the per-instruction values cannot be aligned one-to-one with the instruction lines
- **THEN** all four arrays SHALL be null
- **AND** `descriptions`, `distance` and `duration` SHALL be reported as before

#### Scenario: A calculated route publishes structured steps

- **WHEN** a route is calculated with a description
- **THEN** `steps` SHALL carry one entry per described route node, in route order
- **AND** each entry SHALL carry that node's manoeuvre data without the client having to read `descriptions`
- **AND** the count of `steps` SHALL NOT depend on the count of description lines, so that a mismatch in one SHALL NOT discard the other

#### Scenario: The steps carry the route's database mapping

- **WHEN** a route is calculated from maps of more than one region
- **THEN** each step naming a database SHALL be resolvable to a map directory through the published mapping
- **AND** the mapping SHALL be available even when the description lines carry no information about it

#### Scenario: Existing description lines are unchanged

- **WHEN** a route is calculated after this change
- **THEN** `descriptions` SHALL carry the same lines as before
- **AND** `distance`, `duration` and the four per-instruction arrays SHALL carry the same values as before

### Requirement: A step exposes the manoeuvre facts the description holds

A route's published steps SHALL expose the manoeuvre facts that the route description carries for their nodes, including the motorway junction's name and exit reference, the destination sign text, the way type and the maximum speed, so that a client SHALL NOT need to parse description lines to obtain them.

#### Scenario: A motorway exit is readable as data

- **GIVEN** a calculated route that leaves a motorway at a junction that carries a reference and a name
- **WHEN** a client reads the route's steps
- **THEN** the step for that manoeuvre SHALL carry the exit reference and the junction name
- **AND** it SHALL carry the sign destinations of that junction

#### Scenario: A motorway manoeuvre appears among the steps

- **GIVEN** a calculated route that enters, changes and leaves a motorway
- **WHEN** a client reads the route's steps
- **THEN** the route SHALL publish a step for each of those manoeuvres
- **AND** the description lines SHALL keep their existing content

### Requirement: RouteCallback Java interface

The Java interface `RouteCallback` SHALL carry the failure kinds as integer constants — `KIND_UNKNOWN`,
`KIND_ROUTING_DATA_UNAVAILABLE`, `KIND_START_UNROUTABLE`, `KIND_TARGET_UNROUTABLE`, `KIND_NO_ROUTE_FOUND`,
`KIND_ROUTING_DATA_FAILURE`, `KIND_ENGINE_FAILURE` — and the methods `onProgress(int percent)`,
`onSuccess(RouteEntry route)`, `onError(String message)`, `onErrorDetail(int kind, String detail, double
nearestRoadDistanceMeters)` and `onCancel()`. `onErrorDetail` SHALL be a default method that delegates to
`onError(detail)`, and the kind numbering SHALL be identical on the Java and the native side, because the
integer is what crosses the boundary. It SHALL be passed to the route calculation and invoked from JNI.

#### Scenario: Callback methods invoked from JNI

- **WHEN** route calculation progresses, completes, fails, or is cancelled
- **THEN** the corresponding callback method is called on the Java thread

#### Scenario: A failure reports a kind a caller can act on

- **GIVEN** a calculation that fails because an end cannot be attached to the road network
- **WHEN** the failure reaches Java
- **THEN** `onErrorDetail` SHALL be called with `KIND_START_UNROUTABLE` or `KIND_TARGET_UNROUTABLE`
- **AND** the detail SHALL be an English description for a log, not display text

#### Scenario: An implementation written before the method existed keeps working

- **GIVEN** a `RouteCallback` implementation that overrides only `onError(String)`
- **WHEN** a calculation fails with a kind
- **THEN** the default `onErrorDetail` SHALL call `onError` with the detail
- **AND** no error SHALL be raised for the missing override

#### Scenario: An unknown kind is the unknown kind

- **GIVEN** a failure whose kind this interface does not define
- **WHEN** the kind reaches Java
- **THEN** it SHALL be reported as one of the interface's kinds or as `KIND_UNKNOWN`
- **AND** a caller that does not know it SHALL be able to treat it as `KIND_UNKNOWN`

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
