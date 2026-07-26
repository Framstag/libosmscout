# Proposal: JavaScout GPX Track Import and Render

## Why

JavaScout currently supports routing and favorites but has no way to replay a recorded or planned journey. Adding GPX track import and rendering creates a realistic testbed for live-routing features without needing a live GPS receiver, and gives users a way to visualize recorded tracks alongside the map.

## What Changes

- Add a JNI method to `OSMScoutClient` that imports a GPX file and returns its track points.
- Extend JavaScout's map renderer to draw imported track points as a polyline overlay.
- Replace the standalone favorites button with a top-left menu button; move favorites management into the menu.
- Add a "Import GPX Track…" menu item that opens a file chooser and renders the selected track.
- Register a new synthetic map type `_track` so the Cairo renderer can style the imported track.
- Update JavaScout's build configuration to link against `libosmscout-gpx` and its XML dependency.

No breaking changes to public Java client APIs. Existing route/favorite rendering behavior remains unchanged.

## Capabilities

### New Capabilities

- `javascout-gpx-track-import`: JavaScout can read track points from a GPX file through the Java client JNI bridge.
- `javascout-track-rendering`: JavaScout renders imported GPX track points as a styled polyline overlay on the Cairo map.
- `javascout-track-type`: A synthetic `_track` type is registered at runtime and styled via the standard stylesheet pipeline.
- `javascout-main-menu`: JavaScout has a top-left menu grouping favorites and GPX track import actions.

### Modified Capabilities

- `custom-poi-types`: Extend the existing custom POI registration mechanism to also register a `_track` synthetic type in `MainController`.
- `map-rendering`: Extend the JNI render path to accept optional track geometry arrays, similar to the existing route/favorite marker paths.

## Impact

- `libosmscout-client-java`: new native method, GPX library dependency, additional render parameter.
- `JavaScout`: new UI flow, renderer state, and stylesheet type registration.
- `stylesheets/include/route.oss`: new `_track` WAY style.
- Build files (CMake and Meson) for `libosmscout-client-java` and `JavaScout` need to declare the GPX dependency.
