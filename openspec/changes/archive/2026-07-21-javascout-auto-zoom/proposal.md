# JavaScout auto-zoom by speed

## Why

JavaScout has live navigation with follow mode: the map re-centers on the vehicle position with every GPS update. But the zoom level stays fixed at whatever the user last set. This creates a poor experience at both ends of the speed spectrum:

- **At highway speed (100 km/h)**: zoomed in at street level, the map scrolls frantically and you can't see the next turn or junction until you're on top of it.
- **Walking or cycling**: zoomed out too far, streets and paths are tiny, the route is a thin line, and you can't read street names.

Auto-zoom adjusts the magnification based on current speed so the visible area matches what the driver/cyclist/pedestrian needs at that moment. It's a small change with outsized impact on navigation usability.

## What Changes

- Add speed-to-magnification mapping logic in `MainController`'s position-estimate handler
- Add configurable speed thresholds and zoom levels
- Allow user manual zoom to temporarily override auto-zoom (reset on next speed threshold crossing)
- Add auto-zoom toggle to follow-mode button or settings
- No changes to `libosmscout-client-java` JNI layer or C++ code
- No changes to build files

## Capabilities

### New Capabilities

- `javascout-auto-zoom`: Map magnification adjusts automatically based on navigation speed during follow mode

### Modified Capabilities

- `javascout-map-follow`: Follow mode gains speed-aware zoom adjustment
- `javascout-navigation`: Position estimate handler triggers zoom recalculation

## Impact

- `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java` — add speed-to-mag mapping, auto-zoom logic in position handler, manual-override tracking
- `JavaScout/src/main/java/com/framstag/libosmscout/RoutePanel.java` — optional auto-zoom toggle in navigation status area
- No changes to `libosmscout-client-java`, `MapRenderer.java`, or build files
