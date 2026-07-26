## Context

JavaScout renders the map via JNI to the C++ Cairo backend. The `MercatorProjection` already supports an `angle` parameter (radians, 0 = north-up). OSMScout2 uses this via `vehicleAutoRotateMap` + `VehicleFollowHandler` to rotate the map to driving direction.

Current JavaScourt pipeline: `MapRenderer` → `OSMScoutClient.render()` → JNI → `OSMScoutClient.cpp` → `MercatorProjection.Set(center, 0.0, ...)`. Angle is hardcoded to 0.0.

Navigation position estimates carry a `bearing` field. The `NavigationListener.onPositionEstimate()` callback already receives this.

## Goals / Non-Goals

**Goals:**
- Pass map rotation angle through the full Java → JNI → C++ render pipeline
- Rotate map to driving direction when navigation is active and driving-direction-up mode is selected
- Add compass toggle button (north-up ↔ driving-direction-up)
- Compass icon rotates to show true north even when map is rotated
- Follow-mode + rotation: when user pans/zooms, rotation pauses; re-engages on next position estimate if follow-mode is on

**Non-Goals:**
- No gesture-based rotation (pinch rotate) — only programmatic rotation from bearing
- No changes to libosmscout core C++ libraries
- No changes to other map backends (Qt, AGG, Cairo, etc.)
- No changes to OSMScout2 or other apps

## Decisions

### 1. Add `angle` parameter to JNI render methods

**Decision:** Add `double angle` parameter to `render()` and `renderWithRouteAndPois()` Java native methods, and to the C++ JNI bridge.

**Rationale:** The `MercatorProjection::Set()` already accepts angle. The only missing piece is threading it through the JNI boundary. Adding a parameter is simpler than a separate setter call.

**Alternatives considered:**
- Separate `setAngle()` JNI call on the client → requires state management and thread safety on C++ side. More complex.
- Store angle in `ClientData` → adds mutable state, race conditions with async render.

### 2. MapRenderer tracks `currentAngle`

**Decision:** `MapRenderer` gets a `currentAngle` field (double, radians), default 0.0. `requestRender()` overloads accept optional angle. `doRender()` passes it to JNI.

**Rationale:** The renderer already tracks `currentLat/Lon/Mag`. Angle is a natural extension of view state.

### 3. Compass button in MainController

**Decision:** New compass button (SVGPath icon) overlaid on the map, positioned bottom-right above the follow button. Tapping toggles between `MapRotationMode.NORTH_UP` and `MapRotationMode.DRIVING_DIRECTION_UP`.

**Rationale:** Standard navigation app UX. The compass icon itself rotates to show north direction — this is a visual rotation of the icon only, not the map.

### 4. Rotation driven from NavigationListener.onPositionEstimate()

**Decision:** When `followMode && rotationMode == DRIVING_DIRECTION_UP`, each `onPositionEstimate()` call sets `renderer.setAngle(position.bearing)` and re-renders.

**Rationale:** The bearing from the PositionAgent is the vehicle's heading. This is the same approach OSMScout2 uses — the `VehicleFollowHandler` calls `rotateTo()` with the vehicle bearing.

### 5. Compass icon rendered in JavaFX (no SVG dependency)

**Decision:** Draw compass programmatically using JavaFX Canvas primitives (circle, triangle for north pointer). No new SVG icon files needed.

**Rationale:** Avoids adding icon files to the project. The compass is simple geometry. If a styled icon is wanted later, it can be swapped for an SVG.

### 6. projectToPixel() also needs angle

**Decision:** Add `angle` parameter to `projectToPixel()` JNI method. The `drawCurrentLocationMarker()` in MapRenderer uses this to project coordinates correctly when the map is rotated.

**Rationale:** Without this, the location marker would be drawn at wrong screen coordinates when the map is rotated.

## Risks / Trade-offs

- **[Bearing jitter]** Raw GPS bearing can be noisy → Mitigation: The PositionAgent already smooths bearing. If still jittery, add a low-pass filter in MainController before passing to renderer.
- **[Performance]** Re-rendering on every position estimate (1 Hz) with rotation change → Mitigation: The existing debounce (200ms) in MapRenderer already coalesces rapid requests. Rotation changes within the debounce window are batched.
- **[JNI API change]** Adding `angle` parameter to `render()` and `renderWithRouteAndPois()` breaks binary compatibility for any external JNI callers → Mitigation: No external callers exist. The JavaScout app is the sole consumer.
- **[Compass visual]** Programmatic compass may look basic → Mitigation: Acceptable for v1. Can be replaced with SVG later.
