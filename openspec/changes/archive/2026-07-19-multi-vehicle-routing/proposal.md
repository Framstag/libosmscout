## Why

JavaScout navigation currently supports car routing only. Users navigating by bicycle or on foot get no route guidance. Adding multi-vehicle routing with a selectable profile unlocks the app for cyclists and pedestrians — two large OSM user groups — with minimal engine changes since `MultiDBRoutingService` already supports different vehicles.

## What Changes

- **libosmscout-client**: Expose `Vehicle` enum and `RoutingProfile` parameter struct in the public API. Thread profile through `MultiDBRoutingService` route calculation.
- **libosmscout-client-java**: New `RoutingProfile` Java class with vehicle type and avoid flags. Overloaded `calculateRouteAsync` accepting profile. Profile passed through JNI to C++ routing service. `startNavigation` accepts vehicle type for the navigation engine.
- **JavaScout**: Vehicle selector in `RoutePanel` (car/bicycle/pedestrian). Optional avoid-tolls and avoid-ferries checkboxes. Profile wired through route calculation and navigation session start.

## Capabilities

### New Capabilities

- `routing-profile`: Routing profile configuration — vehicle type (car, bicycle, pedestrian), avoid flags (toll roads, ferries, unpaved roads). Used by both route calculation and live navigation.

### Modified Capabilities

- `route-calculation`: `calculateRouteAsync` gains an overload accepting `RoutingProfile`. JNI bridge passes profile to `MultiDBRoutingService`. Existing no-profile overload defaults to car.
- `route-input`: Route panel adds vehicle selector and optional avoid-* checkboxes. Profile state managed alongside start/destination.
- `turn-by-turn-instructions`: Navigation engine initialized with vehicle type from profile. Instruction text and icons adapt to vehicle (e.g., "Turn left" vs "Dismount and walk" for pedestrian).
- `javascout-navigation`: `startNavigation` accepts vehicle type. Navigation session uses profile-appropriate agents and speed assumptions.

## Impact

- **libosmscout-client**: New public types (`Vehicle`, `RoutingProfile`). No breaking changes — existing car-only API unchanged.
- **libosmscout-client-java**: New `RoutingProfile` class. New overload of `calculateRouteAsync`. `startNavigation` gains vehicle parameter. Backward compatible via default car profile.
- **JavaScout**: `RoutePanel` UI extended with vehicle selector. `MainController` passes profile through route calculation and navigation start.
- **Tests**: `OSMScoutClientNavigationLiveTest` extended with bike and pedestrian test cases.
