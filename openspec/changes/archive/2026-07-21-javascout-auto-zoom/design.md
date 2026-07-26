# JavaScout auto-zoom by speed design

## Context

`MainController` receives position estimates from the navigation engine in `onPositionEstimate`. When follow mode is active, it calls `renderer.requestRenderPreserveRoute(position.lat, position.lon, renderer.getMagnification())` — the magnification is whatever the user last set or the default.

The `NavigationPosition` object includes `accuracy` but not speed. Speed is delivered separately via `onCurrentSpeed(double speedKmH)`. The last known speed is already stored in `RoutePanel` for display.

The `MapRenderer` accepts a magnification parameter (0 = world, higher = more zoomed in). Typical navigation magnifications range from 12 (city street) to 16 (building level) to 18 (pedestrian detail).

## Goals / Non-Goals

**Goals:**
- Adjust map magnification based on current speed during follow mode
- Smooth transitions (no jarring zoom jumps)
- User manual zoom temporarily overrides auto-zoom
- Configurable speed→magnification mapping
- Works with all vehicle types (car, bicycle, pedestrian)

**Non-Goals:**
- Animated zoom transitions (instant snap is acceptable for v1)
- Speed-dependent map rotation (handled separately by driving-direction-up mode)
- Per-vehicle profile customization in v1 (single mapping for all vehicles)

## Decisions

### Decision 1: Speed-to-magnification mapping table

**Chosen:** A lookup table with linear interpolation between breakpoints.

```java
private static final SpeedZoomLevel[] SPEED_ZOOM_TABLE = {
    new SpeedZoomLevel(0,   18),   // stationary / walking → max zoom
    new SpeedZoomLevel(6,   17),   // slow jog
    new SpeedZoomLevel(15,  16),   // cycling / slow city traffic
    new SpeedZoomLevel(30,  15),   // city driving
    new SpeedZoomLevel(60,  14),   // suburban / secondary roads
    new SpeedZoomLevel(90,  13),   // highway
    new SpeedZoomLevel(130, 12),   // very fast
};
```

Magnification is linearly interpolated between breakpoints. Below the first entry, clamp to the first entry's mag. Above the last, clamp to the last entry's mag.

**Rationale:** Simple, predictable, easy to tune. A formula (e.g. `mag = 18 - log2(1 + speed/5)`) is harder to reason about and adjust. The table makes the mapping explicit and tunable without code changes.

**Alternatives considered:**
- Logarithmic formula: harder to tune, less intuitive
- Fixed zoom per speed band (no interpolation): causes visible jumps at boundaries

### Decision 2: Manual zoom override with threshold reset

**Chosen:** When the user manually zooms during follow mode, auto-zoom is suspended. The auto-zoom target is still computed on each position update, but not applied. When the speed crosses a threshold boundary (enters a different table row), auto-zoom re-engages and snaps to the computed magnification.

```java
private boolean autoZoomSuspended = false;
private int lastAutoZoomBand = -1;  // index into SPEED_ZOOM_TABLE

// In position handler:
if (followMode && autoZoomEnabled) {
    int targetMag = computeSpeedZoom(currentSpeedKmH);
    int currentBand = findBand(currentSpeedKmH);
    if (autoZoomSuspended) {
        // Re-engage if speed crossed a threshold boundary
        if (currentBand != lastAutoZoomBand) {
            autoZoomSuspended = false;
        }
    }
    if (!autoZoomSuspended) {
        renderer.requestRenderPreserveRoute(lat, lon, targetMag, angle);
        lastAutoZoomBand = currentBand;
    }
}
```

**Rationale:** The user might zoom in to see a specific intersection. Auto-zoom snapping back immediately would be frustrating. But if their speed changes significantly (e.g. highway exit → city street), the old zoom is no longer appropriate and auto-zoom should re-engage.

**Alternatives considered:**
- Permanent override until manual re-enable: user would have to re-enable auto-zoom after every manual zoom, annoying
- Time-based decay: re-engage after N seconds of no manual zoom — arbitrary, unpredictable

### Decision 3: Auto-zoom enabled by default, toggle in route panel

**Chosen:** Auto-zoom is on by default when follow mode is active. A small toggle button (or the existing follow button cycling through "follow" → "follow + auto-zoom" → "off") controls it.

**Rationale:** Auto-zoom is the expected behavior for navigation. Making it opt-in means most users won't discover it. A toggle lets power users disable it.

**Alternatives considered:**
- Always on during follow mode: no way to disable, some users prefer manual zoom
- Config file only: not discoverable

### Decision 4: Speed source is the last `onCurrentSpeed` value

**Chosen:** `MainController` stores the last speed from `onCurrentSpeed(double speedKmH)` and uses it in the position-estimate handler. If speed is unknown (negative), auto-zoom uses the previous known speed or falls back to a default magnification.

**Rationale:** Speed and position arrive on separate callbacks. The position handler fires more frequently. Using the last known speed is simple and correct — speed doesn't change that fast.

**Alternatives considered:**
- Compute speed from position deltas: noisy, duplicates engine work
- Pass speed through `NavigationPosition`: would require JNI changes

## Data Flow

```
NavigationEngine
    │
    ├── onCurrentSpeed(speedKmH)  ──▶ store lastSpeed
    │
    └── onPositionEstimate(pos) ──▶ if followMode && autoZoomEnabled:
                                        targetMag = lookup(lastSpeed)
                                        if !autoZoomSuspended || bandChanged:
                                            renderer.requestRender(..., targetMag)
```

## State Machine

```
                    ┌──────────────┐
                    │ Follow mode  │
                    │ Auto-zoom on │
                    └──────┬───────┘
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
         Speed change  Manual zoom  Speed crosses
         → update mag  → suspend    band boundary
                        auto-zoom   → re-engage
              │            │            │
              └────────────┼────────────┘
                           ▼
                    ┌──────────────┐
                    │  Render at   │
                    │  new mag     │
                    └──────────────┘
```

## Types

### MainController additions

```java
private static final class SpeedZoomLevel {
    final double speedKmH;
    final int magnification;
    SpeedZoomLevel(double speedKmH, int magnification) { ... }
}

private static final SpeedZoomLevel[] SPEED_ZOOM_TABLE = { ... };
private boolean autoZoomEnabled = true;
private boolean autoZoomSuspended = false;
private int lastAutoZoomBand = -1;
private double lastSpeedKmH = -1.0;
```

### New methods

```java
// In MainController
private int computeSpeedZoom(double speedKmH) { ... }
private int findBand(double speedKmH) { ... }

// In onCurrentSpeed handler — store speed
// In onPositionEstimate handler — apply auto-zoom
```

## Risks / Trade-offs

- **[Zoom oscillation]** If speed hovers at a boundary, the mag could flip back and forth. → Mitigated by interpolation (smooth transition) and band-change detection (only re-engages when crossing a threshold, not on every update).
- **[GPS speed noise]** Noisy speed readings cause jittery zoom. → The engine's `SpeedAgent` already smooths speed; we use the engine's reported speed, not raw GPS deltas.
- **[Manual zoom during high-speed turn]** User zooms in to see a turn, then speed drops (they're turning), band changes, auto-zoom re-engages and zooms out. → Acceptable trade-off. The band-change heuristic works for sustained speed changes, not momentary dips.
- **[Pedestrian use]** Walking speed (~5 km/h) maps to mag 17-18, which is very zoomed in. → Correct for pedestrian navigation where you need to see individual buildings and paths.
