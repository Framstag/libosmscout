## ADDED Requirements

### Requirement: Route calculation via JNI (async with progress)

`OSMScoutClient` SHALL expose native methods for async route calculation:
- `calculateRouteAsync(double startLat, double startLon, double destLat, double destLon, RouteCallback callback)` — starts routing on background thread
- `cancelRoute()` — cancels an in-progress route calculation

The JNI implementation SHALL use `MultiDBRoutingService::GetClosestRoutableNode()` to resolve start/target to routable nodes, `CalculateRoute()` with a `Breaker` and `RoutingProgress` to compute the path, and `TransformRouteDataToPoints()` to extract route geometry. Progress SHALL be reported via JNI callback to Java.

#### Scenario: Calculate route between two valid coordinates
- **WHEN** `calculateRouteAsync()` is called with two valid coordinates within the routing graph
- **THEN** `onSuccess` callback is invoked with a non-empty `RouteEntry[]` containing route geometry coords and metadata

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

### Requirement: RouteEntry Java data class

A new Java class `RouteEntry` SHALL be added to `libosmscout-client-java/java/com/framstag/libosmscout/client/` with public fields: `double[] latitudes`, `double[] longitudes`, `double distance` (meters), `double duration` (seconds).

#### Scenario: RouteEntry holds route geometry
- **WHEN** a route is calculated
- **THEN** `latitudes` and `longitudes` arrays contain the route waypoints in order from start to destination

#### Scenario: RouteEntry holds route metadata
- **WHEN** a route is calculated
- **THEN** `distance` contains the total route length in meters and `duration` contains estimated travel time in seconds

### Requirement: RouteCallback Java interface

A new Java interface `RouteCallback` SHALL be added with methods: `onProgress(int percent)`, `onSuccess(RouteEntry route)`, `onError(String message)`, `onCancel()`. This SHALL be passed to `calculateRouteAsync()` and invoked from JNI.

#### Scenario: Callback methods invoked from JNI
- **WHEN** route calculation progresses, completes, fails, or is cancelled
- **THEN** the corresponding callback method is called on the Java thread
