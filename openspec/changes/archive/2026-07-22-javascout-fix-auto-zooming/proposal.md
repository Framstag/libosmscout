# JavaScout fix auto-zooming

## What Changes

Auto-zoom by speed works in principle but had several problems during GPX track playback:

1. **Initial/tunnel speed unknown**: `lastSpeedKmH` starts at `-1.0`. The SpeedAgent also reports `-1.0` when the position state is `EstimateInTunnel` (e.g. after a GPS gap in a tunnel). `computeSpeedZoom(-1.0)` returned mag 18 — the maximum zoom. When the first real speed arrived after the tunnel, the zoom jumped from 18 to the speed-appropriate level in one frame.

2. **No zoom smoothing**: Every position update snapped directly to the target magnification. With GPS gaps (tunnels, signal loss), the speed could change abruptly, causing jarring 4+ level zoom jumps.

3. **SpeedAgent bogus speeds after GPS gap**: The SpeedAgent's FIFO accumulated a segment across a large time gap (e.g. 103s tunnel). After the segment was popped, subsequent sub-1s GPS updates advanced `lastPosition` without computing speed. When a >=1s gap finally arrived, the position delta covered multiple skipped fixes, producing spuriously high speeds (e.g. 392 km/h).

4. **No turn-aware zoom**: The zoom only responded to speed, not to upcoming route instructions. Drivers couldn't see turn details before a manoeuvre.

5. **SPEED_ZOOM_TABLE too wide**: The range 19→12 (7 levels) caused excessive zoom changes. Low speeds were too zoomed in, high speeds too zoomed out.

6. **Noisy icon path log**: `std::cerr` printed the icon path on every render.

Fixes:
- Skip auto-zoom when `lastSpeedKmH < 0` (unknown speed) — keeps current zoom level
- Smooth zoom: move at most 1 magnification level per position update toward the target
- SpeedAgent: reset FIFO when GPS gap > 10 seconds (tunnel dropout)
- SpeedAgent: cap reported speed at 200 km/h, report `-1.0` for bogus values
- Java-side spike rejection: track `lastGoodSpeedKmH`, reject speeds > 150 km/h
- Turn-aware zoom: boost zoom when approaching a turn (< 600m), hold until 600m past
- Narrowed SPEED_ZOOM_TABLE range to 17→13 (4 levels)
- `currentSmoothMag` initialized to 15.0 instead of DEFAULT_MAGNIFICATION (5)
- Removed `std::cerr` icon path log
- Added `[AutoZoom]` debug logging for initial zoom, turn phase, and convergence

## Capabilities

### New Capabilities

- `javascout-fix-auto-zooming`: Auto-zoom no longer jumps to max zoom on unknown speed; zoom changes are smoothed to at most 1 level per update; SpeedAgent rejects bogus speeds after GPS gaps; turn-aware zoom boosts magnification before manoeuvres

### Modified Capabilities

- `javascout-auto-zoom`: Position-estimate handler guards auto-zoom with `lastSpeedKmH >= 0` check, applies 1-level-per-update smoothing, and filters speed spikes > 150 km/h
- `javascout-map-follow`: Follow mode gains turn-aware zoom (600m pre/post turn window)
- `osmscout::SpeedAgent`: FIFO reset on GPS gap > 10s, speed capped at 200 km/h

## Impact

- `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java` — auto-zoom block in `onPositionEstimate`, new fields (`lastGoodSpeedKmH`, `currentSmoothMag`, `turnZoomActive`, `nextTurnDistanceM`), SPEED_ZOOM_TABLE adjusted
- `libosmscout/src/osmscout/navigation/SpeedAgent.cpp` — FIFO reset on gap > 10s, speed sanity cap at 200 km/h
- `libosmscout-client-java/src/OSMScoutClient.cpp` — removed noisy `std::cerr` icon path log
