# Tasks

## 1. Speed-to-magnification mapping

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 1.1 Define `SpeedZoomLevel` record and `SPEED_ZOOM_TABLE` with breakpoints for stationary→walking→cycling→city→suburban→highway→fast (spec: javascout-auto-zoom, 1 SP)
- [x] 1.2 Implement `computeSpeedZoom(double speedKmH)` with linear interpolation between breakpoints (spec: javascout-auto-zoom, 1 SP)
- [x] 1.3 Implement `findBand(double speedKmH)` returning the table index for the current speed (spec: javascout-auto-zoom, 1 SP)

## 2. Auto-zoom in position handler

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`*

- [x] 2.1 Store last speed from `onCurrentSpeed` callback in `lastSpeedKmH` field (spec: javascout-auto-zoom, 1 SP)
- [x] 2.2 Add auto-zoom state fields: `autoZoomEnabled`, `autoZoomSuspended`, `lastAutoZoomBand` (spec: javascout-auto-zoom, 1 SP)
- [x] 2.3 In `onPositionEstimate` follow-mode block: compute target mag from last speed, check suspension/band-change, apply zoom if not suspended (spec: javascout-auto-zoom, 2 SP)
- [x] 2.4 Wire manual zoom to suspend auto-zoom: add listener in `MapInteractionHandler` or zoom controls that sets `autoZoomSuspended = true` and captures `lastAutoZoomBand` (spec: javascout-auto-zoom, 2 SP)

## 3. Auto-zoom toggle

*All code in `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java` and `RoutePanel.java`*

- [x] 3.1 Add auto-zoom toggle to follow-mode button (cycle: follow → follow+auto-zoom → off) or as a separate toggle in route panel navigation status area (spec: javascout-auto-zoom, 2 SP)
- [x] 3.2 Wire toggle to `autoZoomEnabled` field; update button/toggle visual state (spec: javascout-auto-zoom, 1 SP)

## 4. Testing

- [x] 4.1 Manual test: start navigation with GPX track at 1x speed, verify zoom adjusts as speed changes (spec: javascout-auto-zoom, 1 SP)
- [x] 4.2 Manual test: manually zoom while auto-zoom is active, verify zoom stays at manual level until speed crosses a threshold (spec: javascout-auto-zoom, 1 SP)
- [x] 4.3 Manual test: disable auto-zoom toggle, verify zoom stays fixed regardless of speed (spec: javascout-auto-zoom, 1 SP)
- [x] 4.4 Manual test: switch vehicle to bicycle, verify zoom levels are still appropriate (spec: javascout-auto-zoom, 1 SP)
