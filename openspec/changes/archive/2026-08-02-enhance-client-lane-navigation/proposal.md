## Why

libosmscout core already computes lane-level guidance (lane count, per-lane turn arrows, suggested lane) via `LaneAgent` and exposes it through the navigation engine. The Qt client (`NavigationModel`) already surfaces this data via QML properties. However, the Java client JNI bridge and JavaScout UI only pass a single turn string — the full per-lane turn vector is lost, and there is no visual lane guidance display. Users navigating with JavaScout cannot see which lane to use at complex junctions.

## What Changes

- **Java client JNI bridge**: Extend `NavigationListener.onLaneUpdate` to include the full per-lane turn array (`LaneTurn[]`). Add a `LaneTurn` Java enum matching the C++ `osmscout::LaneTurn`.
- **JavaScout UI**: Add a visual lane guidance overlay showing lane arrows with the suggested lane highlighted, rendered in the next-turn display area.

## Capabilities

### New Capabilities
- `javascout-lane-guidance`: Visual lane guidance display in JavaScout showing per-lane turn arrows and highlighting the suggested lane(s) for the next manoeuvre.

### Modified Capabilities
- `javascout-navigation`: The `NavigationListener.onLaneUpdate` callback signature changes to include a `LaneTurn[] turns` parameter. Existing JavaScout code must be updated.
- `turn-by-turn-instructions`: The next-turn display area gains a lane guidance sub-component.

## Impact

- **libosmscout-client-java** (`libosmscout-client-java/`):
  - Add `LaneTurn.java` enum in `com.framstag.libosmscout.client`
  - Change `NavigationListener.onLaneUpdate` signature to add `LaneTurn[] turns`
  - Update JNI dispatch in `OSMScoutClient.cpp` to build and pass the `turns` array
  - Update `NavigationListener` default method for backward compatibility

- **JavaScout** (`JavaScout/`):
  - `MainController.java`: Update `onLaneUpdate` call to pass `turns`
  - `RoutePanel.java`: Add lane guidance rendering (lane arrow graphics, suggested lane highlight) in the next-turn display area
  - Potentially new FXML or layout elements for the lane display

- **Build**: No new dependencies. Meson and Maven builds may need minor updates for new Java source files.
