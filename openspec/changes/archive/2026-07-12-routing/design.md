## Context

JavaScout is a JavaFX desktop app that renders OSM maps via a JNI bridge (`libosmscout-client-java`). Current JNI surface: `openDatabase`, `render`, `searchLocations`, `getDescription`. No routing exposed.

libosmscout C++ routing stack is mature — `MultiDBRoutingService` with `CalculateRoute()`, `GetClosestRoutableNode()`, `TransformRouteDataToPoints()`. Need JNI bridge to expose this to Java.

Existing `SearchOverlay` provides location search with text input + result list + map navigation. Reusable pattern for route start/destination input.

## Goals / Non-Goals

**Goals:**
- JNI methods on `OSMScoutClient` for route calculation between two `GeoCoord`s
- Java `RouteEntry` class for route result (list of coords + metadata + turn-by-turn descriptions)
- Route panel in JavaScout: start/destination fields reusing search overlay
- Route polyline rendered via Cairo pipeline (matching OSMScout2 overlay pattern)
- Turn-by-turn route description with columnar display (distance, time, instruction)
- Async route calculation with progress dialog and cancel button

**Non-Goals:**
- Turn-by-turn navigation or voice guidance
- Route editing (drag waypoints, alternative routes)
- Multi-waypoint routing (via points)
- Route persistence or history
- Elevation profile

## Decisions

### 1. JNI routing API — `calculateRoute(lat1, lon1, lat2, lon2)` returning `RouteEntry[]`

**Why**: Simplest API matching the first-half scope. Two coords in, route geometry + metadata out. `RouteEntry` mirrors `LocationEntry` pattern — flat data class with public fields, marshalled in JNI.

**Alternatives considered**:
- Expose full `MultiDBRoutingService` object graph to Java — too complex, leaks C++ internals
- Return raw `double[]` interleaved coords — no metadata, fragile

**How**: JNI implementation calls `GetClosestRoutableNode()` for start/target, then `CalculateRoute()`, then `TransformRouteDataToPoints()` to get `GeoCoord` list. Distance extracted from route data.

### 2. Route visualization — Cairo pipeline overlay (matching OSMScout2 pattern)

**Why**: libosmscout `MapPainterCairo` already supports rendering overlay objects via `MapData::poiWays`. OSMScout2 uses `OverlayWay` → `toWay()` → `MapData::poiWays` → rendered by `MapPainter` using `_route` type style. JavaScout should follow same pattern instead of drawing a separate JavaFX canvas overlay.

**How**:
- Modify JNI `render()` to accept optional route overlay data: `render(width, height, lat, lon, mag, routeLats, routeLons)`
- In C++ JNI implementation, create an `osmscout::Way` from route coords, set its type to `_route` (matching OSMScout2 convention), and add to `MapData::poiWays`
- The Cairo painter renders it using the style defined for `_route` type in the stylesheet
- Start/end markers rendered as small `poiNodes` with distinct types (`_route_start`, `_route_end`)
- No separate JavaFX canvas overlay needed — route is part of the rendered pixel buffer

**Alternatives considered**:
- JavaFX `GraphicsContext` overlay on same canvas — simpler but inconsistent with libosmscout rendering architecture, breaks if render backend changes
- Separate `Canvas` layer on top — complicates event handling, double the rendering work

### 3. Route input — reuse `SearchOverlay` pattern

**Why**: `SearchOverlay` already provides location search with text input, result list, debounce, and map navigation. Route panel needs two such fields (start + destination) with same behavior.

**How**: `RoutePanel` contains two search fields. Each field uses `OSMScoutClient.searchLocations()` same as `SearchOverlay`. User types location name or picks on map via long-press. "Calculate" button triggers route.

**Alternatives considered**:
- Separate dialog — more disruptive UX
- Single text field with "from X to Y" parsing — fragile, no autocomplete

### 4. Route description — turn-by-turn via RouteDescriptionPostprocessor

**Why**: User needs to see turn-by-turn instructions, not just distance/duration. `Demos/Routing.cpp` uses `RouteDescriptionPostprocessor` with a callback to generate human-readable route descriptions. Same approach for JavaScout.

**How**:
- After route calculation, call `TransformRouteDataToRouteDescription()` to get `RouteDescription`
- Apply postprocessors: `DistanceAndTimePostprocessor`, `StartPostprocessor`, `TargetPostprocessor`, `WayNamePostprocessor`, `WayTypePostprocessor`, `CrossingWaysPostprocessor`, `DirectionPostprocessor`, `InstructionPostprocessor`, `MotorwayJunctionPostprocessor`, `DestinationPostprocessor`
- Call `RouteDescriptionPostprocessor::GenerateDescription()` with a callback that overrides `BeforeNode(const Node&)` to extract `GetDistance().AsMeter()/1000.0` and `GetTime()` from each route node
- Format output as columnar text: total distance (6 chars) | segment distance (5 chars) | total time (HH:MM) | segment time (+MM:SS) | instruction text
- Return description lines as `String[]` in `RouteEntry`
- JavaScout displays them in a monospace `ListView` below the route parameter box
- Info button dropped — distance/duration shown as columns in description

**Alternatives considered**:
- Qt `RouteDescriptionBuilder` + `RouteStep` — too Qt-specific
- Separate JNI call for description — more roundtrips, simpler to include in RouteEntry

### 5. Route calculation — async with progress + cancel

**Why**: Route calculation can take seconds for long distances. Blocking the UI thread or even the render thread would freeze the map. OSMScout2 uses async routing with progress callback and cancel via `Breaker`.

**How**:
- New JNI method `calculateRouteAsync(startLat, startLon, destLat, destLon, callback)` that runs routing on a background thread
- Progress reported via JNI callback to Java: `onProgress(percent)`, `onSuccess(RouteEntry[])`, `onError(String)`, `onCancel()`
- JavaScout shows a `ProgressIndicator` dialog with cancel button during calculation
- Cancel calls JNI `cancelRoute()` which sets a `Breaker` flag checked by the routing algorithm
- The `Breaker` pattern is already used by `MultiDBRoutingService::CalculateRoute()` — just expose it via JNI

**Alternatives considered**:
- Synchronous on DBThread — blocks render, bad UX
- Java `FutureTask` on thread pool — no progress reporting, no cancel

### 6. Route input — synchronous (no async needed)

**Why**: Location search via `searchLocations()` already runs on a background thread in `SearchOverlay` (Java `Task`). Route panel reuses same pattern. No additional async machinery needed for input.

## Risks / Trade-offs

- **[Risk] `GetClosestRoutableNode()` may return distant node if no routing graph near coord** → Show error message, let user pick closer point on map
- **[Risk] Route calculation is synchronous on DBThread** → Blocks render during calculation. Acceptable for first half; async with progress bar deferred to second half
- **[Risk] JNI marshalling of route coords creates garbage** → `RouteEntry[]` with flat `double[]` for coords minimizes object count
- **[Trade-off] Reusing `SearchOverlay` pattern means route panel depends on location index** → Location index must exist in database; acceptable for typical OSM imports
