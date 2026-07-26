# JavaScout fix auto-zooming — Design

## Context

`MainController` receives position estimates from the navigation engine in `onPositionEstimate`. When follow mode is active, it calls `renderer.requestRenderPreserveRoute()` with the current position and magnification. The magnification was whatever the user last set or the default (5), which is far too zoomed out for navigation.

The `NavigationPosition` object includes `accuracy` but not speed. Speed is delivered separately via `onCurrentSpeed(double speedKmH)`. The last known speed is stored in `lastSpeedKmH`.

The `MapRenderer` accepts a magnification parameter. Typical navigation magnifications range from 12 (suburb) to 19 (block level).

## Goals / Non-Goals

**Goals:**
- Adjust map magnification based on current speed during follow mode
- Smooth transitions (no jarring zoom jumps)
- User manual zoom temporarily overrides auto-zoom
- Turn-aware zoom: boost magnification before manoeuvres
- Reject bogus speed values from SpeedAgent after GPS gaps (tunnels)
- Configurable speed→magnification mapping

**Non-Goals:**
- Animated zoom transitions (instant snap is acceptable)
- Speed-dependent map rotation (handled separately by driving-direction-up mode)
- Per-vehicle profile customization (single mapping for all vehicles)

## Decisions

### Decision 1: Speed-to-magnification mapping table

**Chosen:** A lookup table with linear interpolation between breakpoints. Magnification is a `double` (fractional zoom levels).

```java
private static final SpeedZoomLevel[] SPEED_ZOOM_TABLE = {
    new SpeedZoomLevel(0,   17.0),
    new SpeedZoomLevel(6,   16.0),
    new SpeedZoomLevel(15,  15.0),
    new SpeedZoomLevel(30,  14.0),
    new SpeedZoomLevel(60,  14.0),
    new SpeedZoomLevel(100, 14.0),
    new SpeedZoomLevel(140, 13.0),
    new SpeedZoomLevel(180, 13.0),
};
```

Magnification is linearly interpolated between breakpoints. Below the first entry, clamp to the first entry's mag. Above the last, clamp to the last entry's mag.

**Rationale:** Simple, predictable, easy to tune. Fractional `double` values allow smooth 0.5-level steps during convergence.

### Decision 2: Smooth zoom with 1-level-per-update stepping

**Chosen:** Each position update moves the current magnification at most 1 level toward the target. The current magnification is tracked as a `double` (`currentSmoothMag`) independent of the renderer's int value.

```java
double step = 1.0;
if (targetMag > mag) {
    mag = Math.min(mag + step, targetMag);
} else if (targetMag < mag) {
    mag = Math.max(mag - step, targetMag);
}
currentSmoothMag = mag;
```

**Rationale:** Prevents jarring jumps (e.g. 18→14 after tunnel). At ~1 fix/sec, a 4-level jump takes ~4 seconds. Fractional steps allow sub-level convergence.

### Decision 3: Manual zoom override with threshold reset

**Chosen:** When the user manually zooms during follow mode, auto-zoom is suspended. The auto-zoom target is still computed on each position update, but not applied. When the speed crosses a threshold boundary (enters a different table row), auto-zoom re-engages and snaps to the computed magnification.

```java
if (autoZoomSuspended) {
    if (currentBand != lastAutoZoomBand) {
        autoZoomSuspended = false;
    }
}
```

**Rationale:** The user might zoom in to see a specific intersection. Auto-zoom snapping back immediately would be frustrating. But if their speed changes significantly (e.g. highway exit → city street), the old zoom is no longer appropriate.

### Decision 4: Speed spike rejection

**Chosen:** Track `lastGoodSpeedKmH` — the last speed value ≤ 150 km/h. If `lastSpeedKmH` exceeds 150 km/h (e.g. 392 km/h after tunnel), use `lastGoodSpeedKmH` instead.

```java
if (lastSpeedKmH >= 0 && lastSpeedKmH <= 150.0) {
    speed = lastSpeedKmH;
    lastGoodSpeedKmH = lastSpeedKmH;
} else if (lastSpeedKmH >= 0) {
    speed = lastGoodSpeedKmH; // spike rejected
} else {
    speed = 20.0; // unknown
}
```

**Rationale:** The SpeedAgent can produce bogus speeds after GPS gaps. Rather than capping (which still gives wrong zoom), use the last plausible speed.

### Decision 5: Turn-aware zoom with distance-based hold

**Chosen:** When approaching a turn (< 600m), boost the target magnification. Hold the boost until 600m past the turn (distance-based, not timeout).

```java
if (nextTurnDistanceM < 600) {
    turnZoomActive = true;
} else if (nextTurnDistanceM > 1200) {
    turnZoomActive = false;
}
if (turnZoomActive) {
    if (nextTurnDistanceM < 300) {
        targetMag = Math.max(targetMag, 16.0);
    } else {
        targetMag = Math.max(targetMag, 15.0);
    }
}
```

**Rationale:** The driver needs to see turn details before a manoeuvre. A timeout-based hold would be inconsistent at different speeds. Distance-based hold (600m pre + 600m post = 1200m window) adapts naturally to speed.

### Decision 6: SpeedAgent FIFO reset on GPS gap

**Chosen:** In `SpeedAgent::Process()`, if the time gap between consecutive GPS updates exceeds 10 seconds, clear the FIFO before adding the new segment.

```cpp
auto gap = gpsUpdateMsg->timestamp - lastPosition.time;
if (gap > seconds(10)) {
    segmentFifo.clear();
}
```

**Rationale:** A large gap means GPS signal was lost (tunnel, dropout). The position jump across the gap should not contribute to speed calculation. Clearing the FIFO ensures only post-reacquisition segments are used.

### Decision 7: SpeedAgent sanity cap

**Chosen:** If computed speed exceeds 200 km/h, report `-1.0` (unknown) instead.

```cpp
if (speed > 200.0) {
    speed = -1.0;
}
```

**Rationale:** Defense-in-depth. Even with FIFO reset, edge cases could produce bogus speeds. Reporting unknown lets the Java side fall back to `lastGoodSpeedKmH`.

## Data Flow

```
NavigationEngine
    │
    ├── onCurrentSpeed(speedKmH)  ──▶ store lastSpeedKmH
    │                                  if <= 150: update lastGoodSpeedKmH
    │
    └── onPositionEstimate(pos) ──▶ if followMode && autoZoomEnabled:
                                        speed = filterSpike(lastSpeedKmH)
                                        targetMag = computeSpeedZoom(speed)
                                        if turnZoomActive: boost targetMag
                                        if !autoZoomSuspended:
                                            mag = smoothStep(mag, targetMag)
                                        renderer.requestRender(..., mag)

    └── onNextRouteInstruction(instr) ──▶ store nextTurnDistanceM

SpeedAgent
    │
    └── Process(GPSUpdateMessage) ──▶ if gap > 10s: clear FIFO
                                         compute speed from FIFO
                                         if speed > 200: report -1.0
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
         Speed change  Manual zoom  Turn approach
         → smooth step → suspend    → boost mag
              │            │            │
              └────────────┼────────────┘
                           ▼
                    ┌──────────────┐
                    │  Render at  │
                    │  new mag     │
                    └──────────────┘
```

## Types

### MainController additions

```java
private record SpeedZoomLevel(double speedKmH, double magnification) {}
private static final SpeedZoomLevel[] SPEED_ZOOM_TABLE = { ... };
private boolean autoZoomEnabled = true;
private boolean autoZoomSuspended = false;
private int lastAutoZoomBand = -1;
private double lastSpeedKmH = -1.0;
private double lastGoodSpeedKmH = 20.0;
private double currentSmoothMag = 15.0;
private double nextTurnDistanceM = Double.POSITIVE_INFINITY;
private boolean turnZoomActive = false;
```

### New methods

```java
private static double computeSpeedZoom(double speedKmH) { ... }
private static int findBand(double speedKmH) { ... }
```

## Risks / Trade-offs

- **[Speed spike rejection threshold]** 150 km/h threshold might reject legitimate high-speed driving. → Acceptable for this use case (car not driving at 140+ km/h). Can be tuned.
- **[Turn zoom window]** 600m pre/post might be too short at very high speeds or too long in dense city driving. → Single config works for initial version.
- **[Smoothing lag]** 1 level/sec means a 4-level jump takes 4 seconds. During rapid speed changes, the zoom lags behind. → Acceptable trade-off for smoothness.
