# Tasks: JavaScout GPX Track Import and Render

## 1. GPX import Java client API

- [x] 1.1 Add `TrackPoint` Java data class with `lat`, `lon`, `timestamp` (spec: `javascout-gpx-track-import`) — 2 SP
- [x] 1.2 Add `TrackPoint[] importGpxTrack(String filePath)` native method declaration to `OSMScoutClient` (spec: `javascout-gpx-track-import`) — 1 SP
- [x] 1.3 Implement JNI `Java_com_framstag_libosmscout_client_OSMScoutClient_importGpxTrack` calling `osmscout::gpx::ImportGpx` and building Java array (spec: `javascout-gpx-track-import`) — 3 SP
- [x] 1.4 Guard import method when `OSMSCOUT_BUILD_GPX` is off (return empty array + warning) (spec: `javascout-gpx-track-import`) — 1 SP

## 2. Build configuration for GPX dependency

- [x] 2.1 Add `libosmscout-gpx` and libxml2 dependencies to `libosmscout-client-java` Meson build (spec: `javascout-gpx-track-import`) — 2 SP
- [x] 2.2 Add equivalent dependency declarations to Meson build for `libosmscout-client-java` (spec: `javascout-gpx-track-import`) — 2 SP
- [x] 2.3 Ensure `JavaScout` Maven/CMake setup inherits the GPX dependency (spec: `javascout-gpx-track-import`) — 2 SP

## 3. Track rendering data path

- [x] 3.1 Add `trackLats`/`trackLons` fields and `setTrackPoints`/`clearTrack` methods to `MapRenderer` (spec: `javascout-track-rendering`) — 2 SP
- [x] 3.2 Extend JNI render method signature to accept track arrays (spec: `map-rendering`) — 2 SP
- [x] 3.3 In C++ render method, build synthetic `_track` WAY and add it to `MapData::poiWays` (spec: `map-rendering`) — 3 SP
- [x] 3.4 Ensure track is preserved across pan, zoom, and resize re-renders (spec: `javascout-track-rendering`) — 1 SP

## 4. Synthetic track type registration and styling

- [x] 4.1 Register `_track` via `OSMScoutClientBuilder.withCustomPoiType("_track")` in `MainController` (spec: `custom-poi-types`) — 1 SP
- [x] 4.2 Add `_track` WAY style to `stylesheets/include/route.oss` (spec: `javascout-track-type`) — 2 SP
- [x] 4.3 Verify `_track` renders distinguishable from `_route` (spec: `javascout-track-type`) — 1 SP

## 5. JavaScout top-left menu and track import UI

- [x] 5.1 Replace standalone favorites button with a top-left menu button (spec: `javascout-main-menu`) — 2 SP
- [x] 5.2 Add menu items "Favorites" and "Import GPX Track…" (spec: `javascout-main-menu`) — 2 SP
- [x] 5.3 Wire "Favorites" menu item to open existing `FavLocationDialog` (spec: `javascout-main-menu`) — 1 SP
- [x] 5.4 Implement JavaFX `FileChooser` with `.gpx` filter and path handling (spec: `javascout-track-rendering`) — 2 SP
- [x] 5.5 Wire import result to `MapRenderer.setTrackPoints` and trigger re-render (spec: `javascout-track-rendering`) — 2 SP
- [x] 5.6 Handle import errors/cancellation gracefully and dismiss menu on Escape/outside click (spec: `javascout-main-menu`) — 2 SP

## 6. Verification

- [x] 6.1 Build and run JavaScout with a sample GPX file; confirm track renders (spec: all) — 2 SP
- [x] 6.2 Build with `OSMSCOUT_BUILD_GPX=OFF`; confirm graceful degradation (spec: `javascout-gpx-track-import`) — 1 SP
- [x] 6.3 Add or update automated tests if feasible (JNI unit test or Java test with mocked client) (spec: all) — 3 SP
