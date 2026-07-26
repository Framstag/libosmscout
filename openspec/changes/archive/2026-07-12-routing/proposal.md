## What Changes

Add interactive routing to JavaScout (JavaFX app): user inputs start + destination, engine calculates route, route renders on map with optional info popup. This is the first half of full routing — covers UI input, route computation, and visual feedback. Excludes turn-by-turn navigation, voice guidance, and route editing.

## Capabilities

### New Capabilities

- `route-input`: UI for entering start and destination. Reuses existing search overlay pattern — user types location name or picks on map. Start/destination fields in a route panel.
- `route-calculation`: JNI bridge to libosmscout routing engine. New native methods on `OSMScoutClient` for calculating route between two `GeoCoord`s. Returns route geometry (list of coords) + metadata (distance, estimated time).
- `route-visualization`: Render computed route on map canvas overlay — colored polyline with start/end markers. Updates on recalculation.
- `route-info-popup`: Optional popup showing route summary (distance, duration). Toggle on/off.
- `route-description`: Turn-by-turn route description shown below the route parameter box. Generated via `RouteDescriptionPostprocessor` (matching `Demos/Routing.cpp`). Includes distance, duration, street names, turn directions, and waypoint list. Replaces the separate Info button — info is part of the description.

### Modified Capabilities

*(None — no existing spec-level behavior changes.)*

## Impact

- **libosmscout-client-java/java/**: New `RouteEntry` Java class (route result). New native methods on `OSMScoutClient`: `calculateRoute(lat1,lon1,lat2,lon2)` returning `RouteEntry[]`.
- **libosmscout-client-java/src/**: New C++ JNI implementation calling `MultiDBRoutingService::CalculateRoute()` and `TransformRouteDataToPoints()`. New Meson/CMake build entries.
- **JavaScout/**: New `RouteOverlay.java` (canvas overlay for route polyline), `RoutePanel.java` (start/dest input panel reusing search overlay pattern, route description list), update `MainController.java` to wire routing UI.
- **libosmscout-routing**: Already exists — no changes needed, just JNI integration.
