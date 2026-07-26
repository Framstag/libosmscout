# JavaScout automatic rerouting design

## Context

`MainController` creates a `NavigationListener` that handles all engine callbacks. The `onRerouteRequest` handler is currently a TODO:

```java
@Override
public void onRerouteRequest(double lat, double lon, double bearing,
                             double destLat, double destLon) {
    Platform.runLater(() -> {
        // TODO: automatically recalculate route from current
        // position to destination
    });
}
```

The engine fires this when `PositionAgent` detects the vehicle is off the planned route. The callback provides:
- `lat`, `lon` — current estimated position
- `bearing` — current heading (or NaN if unknown)
- `destLat`, `destLon` — original destination coordinates

Everything needed to recalculate is already available. The route calculation API (`calculateRouteAsync`), route overlay update, and navigation restart all exist.

## Goals / Non-Goals

**Goals:**
- Recalculate route when off-route is detected
- Prevent reroute loops with a cooldown
- Show visual feedback during rerouting
- Handle failure gracefully (keep old route, show error)

**Non-Goals:**
- Alternative route suggestions (only one reroute calculated)
- Avoid areas / road closures (routing profile unchanged)
- Partial reroute / keep remaining route segments (full recalculation from current position)
- Real GPS provider integration (separate concern)

## Decisions

### Decision 1: Full recalculation from current position

**Chosen:** Call `calculateRouteAsync(currentLat, currentLon → destLat, destLon)` with the same `RoutingProfile` as the original route.

**Rationale:** The engine already provides current position and destination. The routing service handles finding the nearest routable node. This is the simplest correct approach.

**Alternatives considered:**
- Partial reroute keeping remaining waypoints — more complex, no API for it in `MultiDBRoutingService`
- Pre-computed alternative routes — not supported by the routing engine

### Decision 2: Reroute cooldown

**Chosen:** 15-second cooldown after each reroute attempt. During cooldown, `onRerouteRequest` is ignored.

**Rationale:** Without a cooldown, a single off-route event could trigger cascading reroutes if the new route also doesn't match the vehicle's actual path (e.g. wrong turn, GPS noise). 15 seconds gives the driver time to follow the new route before another reroute fires.

**Edge case:** If the vehicle is still off-route after the cooldown expires, the engine will fire another `onRerouteRequest` and a fresh reroute is attempted.

### Decision 3: Visual feedback via RoutePanel

**Chosen:** `RoutePanel` shows a "Rerouting..." label with a subtle animation during recalculation. On success, the label disappears and the new route is displayed. On failure, a brief "Reroute failed" message is shown.

**Rationale:** The user needs to know the system is responding. A spinner in the route panel is unobtrusive but visible.

### Decision 4: Keep old route until new one succeeds

**Chosen:** The old route overlay and navigation session remain active until the new route is calculated and navigation restarts.

**Rationale:** If reroute fails, the user still has the original route as a reference. If we cleared it, they'd have no guidance at all.

### Decision 5: Generation counter guards stale JNI callbacks

**Chosen:** Each `NavigationListener` captures `rerouteGeneration` at creation. When reroute starts, the counter increments. Every callback checks `isStale()` — if generation doesn't match, it bails immediately.

**Rationale:** The C++ `NavigationController` runs on a background thread and queues `Platform.runLater()` callbacks via JNI. When the old controller is stopped and a new one starts, stale callbacks from the old thread may still be in the JavaFX event queue. Without a generation guard, these stale callbacks overwrite the new route's instruction data in the UI.

**Alternatives considered:**
- Flush pending `Platform.runLater()` calls — no public JavaFX API for this
- Clear next-turn overlay before starting new nav — still racy, stale callback could execute after

### Decision 6: Track player pauses during reroute, not stops

**Chosen:** During reroute, the track player is paused (not stopped) and its controller reference is swapped to the new `NavigationController` after reroute succeeds. Playback resumes from the next unplayed point.

**Rationale:** Stopping the track player during reroute kills the GPS fix source. Without GPS fixes, the new `NavigationController` never emits position estimates, so the marker stays gone. Pausing preserves the playback position (`emittedIndex`), and swapping the controller lets the same track feed the new navigation engine.

**Implementation:** Added `TrackPlayer.setController(NavigationController)` method. The `controller` field is no longer `final`.

## Data Flow

```
GPS fix → NavigationEngine
              ↓
         PositionAgent detects OffRoute
              ↓
         RerouteRequestMessage
              ↓  JNI
         NavigationListener.onRerouteRequest(lat, lon, bearing, destLat, destLon)
              │
              ├── cooldown active? → return (ignore)
              │
              └── cooldown expired?
                      │
                      ▼
              ┌─────────────────────┐
              │ 1. Set reroute flag  │
              │ 2. Show "Rerouting…" │
              │ 3. Cancel old nav    │
              │    (keep route       │
              │     overlay visible) │
              └────────┬────────────┘
                       │
                       ▼
              ┌─────────────────────┐
              │ 4. calculateRoute   │
              │    Async(current,   │
              │     dest, profile)  │
              └────────┬────────────┘
                       │
              ┌────────┴────────────┐
              ▼                     ▼
         onSuccess              onError
              │                     │
              ▼                     ▼
      ┌──────────────┐    ┌────────────────┐
      │ 5a. Update    │    │ 5b. Show error │
      │     route     │    │     message     │
      │     overlay   │    │     (3s toast)  │
      │ 6a. Start new │    │ 6b. Keep old   │
      │     nav       │    │     route + nav │
      │ 7a. Clear     │    └────────────────┘
      │     reroute   │
      │     flag      │
      └──────────────┘
```

## State Machine

```
                    ┌──────────┐
                    │ On Route │
                    └────┬─────┘
                         │ OffRoute detected
                         ▼
                    ┌──────────┐
              ┌────▶│ Rerouting│◀────────────┐
              │     │ (cooldown│             │
              │     │  active) │             │
              │     └────┬─────┘             │
              │          │                   │
              │     ┌────┴─────┐             │
              │     │  Route   │             │
              │     │Calculated│             │
              │     └────┬─────┘             │
              │          │                   │
              │     ┌────┴─────┐    still    │
              │     │ On Route │  off-route  │
              │     └────┬─────┘  after 15s  │
              │          │                   │
              │          └───────────────────┘
              │
              └── cooldown expired, still off-route
                  → reroute again
```

## Types

### MainController additions

```java
// New fields
private static final long REROUTE_COOLDOWN_MS = 15_000;
private long lastRerouteTime = 0;
private boolean rerouting = false;
private double routeDestLat;
private double routeDestLon;
```

### RoutePanel additions

```java
// New method
void setRerouteStatus(boolean active, boolean failed);
// Shows/hides "Rerouting..." label or "Reroute failed" toast
```

## Risks / Trade-offs

- **[Reroute loop]** If the new route also doesn't match the vehicle's path (e.g. vehicle is on a parallel road), the engine will fire another off-route immediately. → Mitigated by 15s cooldown.
- **[Race condition]** Reroute calculation runs on a background thread. If the user manually stops navigation during rerouting, the callback may fire after stop. → Check `navigationController != null` and `rerouting` flag in success handler.
- **[GPS noise]** A single noisy GPS fix could trigger a false off-route. → The engine's `PositionAgent` already has hysteresis for this; we trust its judgment.
- **[No profile change]** Reroute uses the same vehicle profile. If the user went off-route because the road type is impassable for their vehicle, rerouting won't help. → Acceptable for v1; profile switching is a future enhancement.
