# JavaScout show current road info

## Why

During live navigation, JavaScout shows the next turn instruction in the top-left overlay, but doesn't show what road the vehicle is currently on. The driver has no way to confirm they're on the correct road without zooming in and reading street labels on the map.

The navigation engine already knows the current road from the route description — it has the road name, reference (e.g. "A40"), and type (e.g. "motorway"). This data just isn't exposed to the Java layer or displayed in the UI.

Showing the current road info in a separate overlay above the turn instructions gives the driver continuous awareness of their current road without distracting from the next-turn guidance.

## What Changes

- Add `CurrentRoadInfo` Java class with `ref`, `typeName`, `name` fields
- Add `onCurrentRoadInfo(CurrentRoadInfo)` callback to `NavigationListener`
- Modify C++ JNI `DispatchPositionEstimate` to extract `NameDescription` and `TypeNameDescription` from the route node and dispatch them to Java
- Add a new overlay in `MainController` (top-left, above next-turn overlay) showing the current road info
- Update `meson.build` for `libosmscout-client-java` to include the new file

## Capabilities

### New Capabilities

- `javascout-current-road-info`: Display of current road name, reference, and type during live navigation

### Modified Capabilities

- `javascout-navigation`: Navigation listener gains `onCurrentRoadInfo` callback
- `javascout-map-follow`: Current road info overlay shown in top-left during navigation

## Impact

- `libosmscout-client-java/java/com/framstag/libosmscout/client/CurrentRoadInfo.java` — new data class
- `libosmscout-client-java/java/com/framstag/libosmscout/client/NavigationListener.java` — new `onCurrentRoadInfo` default method
- `libosmscout-client-java/src/OSMScoutClient.cpp` — road info extraction in `DispatchPositionEstimate`, new JNI method lookups
- `libosmscout-client-java/java/meson.build` — add `CurrentRoadInfo.java`
- `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java` — new overlay, `onCurrentRoadInfo` handler
