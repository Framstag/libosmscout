## 1. Java Client Library — Data Classes

- [x] 1.1 Create `RouteEntry.java` in `libosmscout-client-java/java/com/framstag/libosmscout/client/` with fields: `double[] latitudes`, `double[] longitudes`, `double distance`, `double duration` (spec: `route-calculation`)
- [x] 1.2 Create `RouteCallback.java` interface in same package with methods: `onProgress(int percent)`, `onSuccess(RouteEntry route)`, `onError(String message)`, `onCancel()` (spec: `route-calculation`)
- [x] 1.3 Add new Java files to `libosmscout-client-java/java/meson.build` (jar + native_headers) (spec: `route-calculation`)

## 2. JNI — Route Calculation

- [x] 2.1 Add `calculateRouteAsync()` native method to `OSMScoutClient.java` — takes start lat/lon, dest lat/lon, and `RouteCallback` object (spec: `route-calculation`)
- [x] 2.2 Add `cancelRoute()` native method to `OSMScoutClient.java` (spec: `route-calculation`)
- [x] 2.3 Implement `calculateRouteAsync()` in `OSMScoutClient.cpp` — spawn background thread, call `GetClosestRoutableNode()` for start/target, `CalculateRoute()` with `Breaker` + `RoutingProgress`, `TransformRouteDataToPoints()` for geometry (spec: `route-calculation`, design: §1, §5)
- [x] 2.4 Implement `cancelRoute()` in `OSMScoutClient.cpp` — set `Breaker` flag (spec: `route-calculation`, design: §5)
- [x] 2.5 Implement JNI progress callback — invoke `RouteCallback.onProgress()` from `RoutingProgress` (spec: `route-calculation`, design: §5)
- [x] 2.6 Implement JNI success/error/cancel callbacks — marshal `RouteEntry` from C++ route data, invoke `RouteCallback.onSuccess()`/`onError()`/`onCancel()` (spec: `route-calculation`, design: §5)

## 3. JNI — Route Visualization via Cairo Pipeline

- [x] 3.1 Modify `render()` JNI signature to accept optional route overlay data: `render(width, height, lat, lon, mag, routeLats, routeLons)` (spec: `route-visualization`, design: §2)
- [x] 3.2 In C++ `render()` implementation, create `osmscout::Way` from route coords, set type to `_route`, add to `MapData::poiWays` (spec: `route-visualization`, design: §2)
- [x] 3.3 Create start/end marker nodes with types `_route_start` / `_route_end`, add to `MapData::poiNodes` (spec: `route-visualization`, design: §2)
- [x] 3.4 Add `_route`, `_route_start`, `_route_end` type definitions to stylesheet (`.ost` file) with appropriate rendering styles (spec: `route-visualization`, design: §2)

## 4. JavaScout — Route Input Panel

- [x] 4.1 Create `RoutePanel.java` — route panel with start/destination search fields reusing `SearchOverlay` pattern, "Calculate Route" button (spec: `route-input`, design: §3, §6)
- [x] 4.2 Wire long-press on map to set start/destination coordinate when corresponding field is active (spec: `route-input`, design: §3)
- [x] 4.3 Disable "Calculate Route" button until both fields have valid coordinates (spec: `route-input`)
- [x] 4.4 Add `RoutePanel` to `MainController` and FXML layout (spec: `route-input`)

## 5. JavaScout — Route Calculation UI

- [x] 5.1 Create progress dialog with `ProgressIndicator` and cancel button (spec: `route-calculation`, design: §5)
- [x] 5.2 Wire "Calculate Route" button to call `OSMScoutClient.calculateRouteAsync()` and show progress dialog (spec: `route-calculation`, design: §5)
- [x] 5.3 Wire cancel button to call `OSMScoutClient.cancelRoute()` (spec: `route-calculation`, design: §5)
- [x] 5.4 On success: pass `RouteEntry` coords to render pipeline and trigger re-render (spec: `route-calculation`, `route-visualization`, design: §2, §5)
- [x] 5.5 On error: show error message dialog (spec: `route-calculation`)

## 6. Route Description (Turn-by-Turn)

- [x] 6.1 Add `String[] descriptions` field to `RouteEntry.java` (spec: `route-info-popup`)
- [x] 6.2 Add route description generation in C++ JNI: call `TransformRouteDataToRouteDescription()`, apply postprocessors, run `RouteDescriptionPostprocessor::GenerateDescription()` with `BeforeNode` callback collecting text lines with columnar distance/time format (spec: `route-info-popup`, design: §4)
- [x] 6.3 Add `String[] descriptions` marshalling in JNI success callback (spec: `route-info-popup`)
- [x] 6.4 Replace info popup with monospace `ListView` showing columnar turn-by-turn instructions below route parameter box in `RoutePanel.java` (spec: `route-info-popup`, design: §4)
- [x] 6.5 Remove Info toggle button, show distance/duration as columns in description (spec: `route-info-popup`)
