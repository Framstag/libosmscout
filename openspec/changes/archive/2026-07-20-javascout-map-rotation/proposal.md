# JavaScout Map Rotation

## What Changes

JavaScout currently renders the map with north always up. When navigation is active, the map should rotate so the driving direction is always pointing up (forward-up orientation). A compass button lets the user toggle between "north-up" and "driving-direction-up" modes.

This matches standard navigation app behavior (Google Maps, Apple Maps, OSM Scout 2).

## Capabilities

### New Capabilities

- `map-rotation`: Rotate the map projection so the user's bearing aligns with screen-up when navigation is active and follow-mode is on. Rotation is applied via the existing MercatorProjection angle parameter (same mechanism as OSMScout2's `vehicleAutoRotateMap`). When the user pans/zooms manually, rotation pauses; it resumes when follow-mode re-engages.

- `compass-toggle`: A new compass icon button overlaid on the map. Tapping it switches between:
  - **North-up** (default, angle=0): map stays fixed with north at top
  - **Driving-direction-up** (bearing-aligned): map rotates so the vehicle's heading points up

  The compass icon itself rotates to show true north direction, giving the user a visual reference even when the map is rotated.

### Modified Capabilities

- `javascout-navigation` (from prior change): The existing follow-mode button and navigation position tracking now also drive map rotation. When follow-mode is active and `driving-direction-up` is selected, the map bearing updates on each position estimate.

## Impact

- **JavaScout/src/main/java/com/framstag/libosmscout/MapRenderer.java**: Add `currentAngle` field, pass angle to JNI render call, add `setAngle()` / `getAngle()` methods. The `requestRender` / `doRender` pipeline must forward the angle to the C++ projection.

- **JavaScout/src/main/java/com/framstag/libosmscout/MapInteractionHandler.java**: No change — rotation is driven programmatically, not by gesture. Gesture handlers remain north-up relative.

- **JavaScout/src/main/java/com/framstag/libosmscout/MainController.java**: Add compass button overlay. Wire navigation position bearing to map rotation when in driving-direction-up mode. Toggle between north-up and bearing-aligned on compass tap.

- **JavaScout/src/main/java/com/framstag/libosmscout/client/OSMScoutClient.java** (JNI): The `render()` and `renderWithRouteAndPois()` native methods need an `angle` parameter added. The C++ JNI bridge must pass it through to `MercatorProjection::Set()`.

- **libosmscout-client-java/**: JNI C++ glue code for the render methods must accept the new angle parameter.

- **Icons**: A compass SVG icon needs to be added to the icon set (or rendered programmatically in JavaFX).

- **No changes** to libosmscout core, libosmscout-map, or any other C++ library — the projection already supports arbitrary angles.
