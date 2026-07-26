# JavaScout show current road info design

## Context

The navigation engine's `PositionAgent` produces a `PositionMessage` on each GPS update. This message contains a `position.routeNode` iterator into the `RouteDescription`, which carries typed descriptions for the current route segment:

- `NameDescription` — road name (e.g. "Ruhrschnellweg") and reference (e.g. "A40")
- `TypeNameDescription` — road type (e.g. "motorway", "primary", "residential")

The `PositionMessage` has a `GetRouteDescription<T>()` template method that extracts a description from the current route node.

Currently, `DispatchPositionEstimate` in `OSMScoutClient.cpp` only extracts `state`, `lat`, `lon`, and `bearing` from the message. The road info is discarded.

## Goals / Non-Goals

**Goals:**
- Expose current road name, ref, and type from the navigation engine to Java
- Display road info in a separate overlay above the next-turn instructions
- Visually distinguish road info from turn instructions (smaller font, muted color)
- Each field shown only if available (name, ref, type are independent)

**Non-Goals:**
- No changes to the route calculation or navigation engine behavior
- No changes to the `NavigationPosition` class (backward compatible)
- No changes to the `RoutePanel` or other UI components

## Decisions

### Decision 1: Road info from DescriptionService, not from route description

**Chosen:** Call `client.getDescription(lat, lon)` on a background thread to look up road info at the current geo coordinate. Parse the returned `ObjectDescription` entries for the "General" section, extracting "Ref", "Type", and "Name" label values.

**Rationale:** The user wants road info derived from the current GPS position (the marker), not from the routing instructions. The `DescriptionService` queries the database for objects at the coordinate and returns structured entries. This works regardless of whether the vehicle is on-route or off-route.

The `GeneralDescriptionProcessor` produces entries with:
- `sectionKey="General"`, `labelKey="Type"` → road type (e.g. "motorway")
- `sectionKey="General"`, `labelKey="Name"` → road name (e.g. "Ruhrschnellweg")
- `sectionKey="General"`, `labelKey="Ref"` → road reference (e.g. "A40")

**Alternatives considered:**
- Extract from `PositionMessage::position.way` in C++ JNI — requires feature value readers, more complex JNI
- Extract from `RouteDescription::NameDescription` — only works when on-route, user explicitly rejected this
- New native method `getRoadInfoAt()` — would duplicate `getDescription` logic

### Decision 2: Throttled background lookup

**Chosen:** Throttle the `getDescription` call to at most once every 2 seconds, and skip if the position hasn't moved significantly (~50m). Run on a background thread to avoid blocking the UI or navigation thread.

**Rationale:** `getDescription` is a database query (LoadAreasInRadius, LoadWaysInRadius, LoadNodesInRadius). Calling it on every position update (every 1-2 seconds during track playback) would be wasteful. The road doesn't change every few meters — 2 seconds and 50m granularity is sufficient.

**Alternatives considered:**
- Call on every position update — too expensive
- Extract from C++ `PositionMessage::position.way` — avoids DB query but requires JNI changes

### Decision 3: Overlay above next-turn, visually distinct

**Chosen:** A new `VBox` overlay positioned at `TOP_LEFT` with `StackPane.setMargin` of zero (same as next-turn overlay). The road info box is added to the map panel *before* the next-turn box so it appears above it (JavaFX stack order). Styling uses smaller font and muted gray text to distinguish from the blue turn icon and bold distance text of the next-turn overlay.

**Rationale:** The road info is supplementary — useful for awareness but not as critical as the next turn. Placing it above the turn instructions in the same corner creates a natural reading order: current road first, then upcoming turn.

## Data Flow

```
NavigationEngine
    │
    └── PositionMessage
        ├── position.coord        → onPositionEstimate (existing)
        └── position.state        → onPositionEstimate (existing)
                                    ↓
                            updateRoadInfoFromPosition(lat, lon)
                              (throttled: ≤1/2s, ≥50m)
                                    ↓
                            client.getDescription(lat, lon)
                              [background thread, DB query]
                                    ↓
                            Parse "General" section entries
                              Ref → ref, Type → typeName, Name → name
                                    ↓
                            updateCurrentRoadOverlay(CurrentRoadInfo)
                                    ↓
                            Show/hide overlay in top-left
```

## UI Layout

```
┌──────────────────────────────────────┐
│ ┌────────────────────┐               │
│ │ A40 motorway       │  ← road info (new, muted gray)
│ │ Ruhrschnellweg     │               │
│ └────────────────────┘               │
│ ┌────────────────────┐               │
│ │ ➡ 200 m → Hauptstr │  ← next turn (existing, blue icon)
│ │    then 500 m → B1 │               │
│ └────────────────────┘               │
│                                      │
│              🟦 (follow button)      │
└──────────────────────────────────────┘
```

## Types

### CurrentRoadInfo (Java)

```java
public class CurrentRoadInfo {
    public final String ref;      // e.g. "A40", "B1", or ""
    public final String typeName; // e.g. "motorway", "primary", or ""
    public final String name;     // e.g. "Ruhrschnellweg", or ""

    public boolean hasInfo();
    public String toDisplayString(); // "[ref] [typeName] [name]"
}
```

### NavigationListener addition

```java
default void onCurrentRoadInfo(CurrentRoadInfo info) {}
```

## Risks / Trade-offs

- **[Off-route]** When the vehicle is off-route, `getDescription` still returns the road at the current coordinate (the nearest way). → The overlay shows whatever road is at the snapped position, which is correct behavior.
- **[No road name]** Many OSM ways have no name tag (e.g. unnamed residential streets). → The overlay shows only ref and/or type if available. If nothing is available, the overlay is hidden.
- **[DB query latency]** `getDescription` queries the database (LoadAreasInRadius, LoadWaysInRadius, LoadNodesInRadius) which may take 10-100ms. → Mitigated by 2-second throttle and 50m distance threshold. The query runs on a background thread, not the UI or navigation thread.
- **[Stale road info]** After a road change, the overlay may show the old road for up to 2 seconds. → Acceptable trade-off. The driver doesn't need sub-second road name updates.
