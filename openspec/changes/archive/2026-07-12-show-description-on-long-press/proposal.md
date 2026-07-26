## What Changes

Add a long-press interaction on the map in **JavaScout** (JavaFX demo app) that finds the closest relevant OSM object at the pressed location, retrieves a structured description via `DescriptionService`, and displays it in a dynamic overlay dialog.

The change spans three layers:
- **`libosmscout-client-java`** (JNI C++ → Java) — new native methods for object lookup by coordinate and `DescriptionService` access
- **`JavaScout/`** (JavaFX app) — long-press detection, description overlay dialog, wiring

## Capabilities

### New Capabilities

- `long-press-description`: On mouse long press in the JavaScout map view, resolve the geo coordinate to the closest relevant map object (node, way, or area), call `DescriptionService::GetDescription()` on it, and render the result in a JavaFX overlay dialog. The dialog content dynamically adapts to the section/subsection/label/value structure of `ObjectDescription`. On small screens (width < 600px) the dialog is fullscreen; on desktop it appears as a centered overlay.

### Modified Capabilities

- *(none — no existing capability has spec-level behavior changes)*

## Impact

### Affected components

| Component | Impact |
|-----------|--------|
| `libosmscout-client-java/java/.../client/OSMScoutClient.java` | **Modify** — add `getDescription(lat, lon)` native method returning `ObjectDescription` |
| `libosmscout-client-java/java/.../client/ObjectDescription.java` | **Create** — new Java model class mirroring C++ `ObjectDescription` / `DescriptionEntry` (sectionKey, subsectionKey, index, labelKey, value) |
| `libosmscout-client-java/src/OSMScoutClient.cpp` | **Modify** — implement JNI `getDescription()`: resolve closest object via `DBThread`/`Database`, call `DescriptionService::GetDescription()`, marshal result to Java |
| `libosmscout-client-java/CMakeLists.txt` | **Modify** — link `osmscout` library for `DescriptionService` (verify already linked) |
| `libosmscout-client-java/meson.build` | **Modify** — same link check |
| `JavaScout/src/main/java/.../MapInteractionHandler.java` | **Modify** — add long-press detection: `MOUSE_PRESSED` starts a timer, `MOUSE_RELEASED`/`MOUSE_DRAGGED` cancels it; on timer fire, invoke description lookup |
| `JavaScout/src/main/java/.../DescriptionOverlay.java` | **Create** — new JavaFX overlay dialog (extends `StackPane` like `SearchOverlay`) that renders `ObjectDescription` entries dynamically: sections as headers, subsections as sub-headers, label/value pairs as rows. Fullscreen on small screens. |
| `JavaScout/src/main/java/.../MainController.java` | **Modify** — wire long-press callback from `MapInteractionHandler` to open `DescriptionOverlay` |
| `JavaScout/src/main/resources/.../style.css` | **Modify** — add CSS classes for description overlay |
| `JavaScout/pom.xml` | **Modify** — if new Java files need no extra deps, no change needed |

### Architecture sketch

```
mouse long press (hold 500ms)
  → MapInteractionHandler fires callback with (lat, lon)
  → MainController calls client.getDescription(lat, lon)
    → JNI: query objects in small bbox around (lat, lon)
    → JNI: rank by (has data, visible, proximity)
    → JNI: DescriptionService::GetDescription(best object)
    → JNI: marshal ObjectDescription entries to Java
  → DescriptionOverlay renders sections dynamically
```

### `ObjectDescription` data model (Java)

```java
public class DescriptionEntry {
    String sectionKey;
    String subsectionKey;   // may be empty
    boolean hasIndex;
    int index;
    String labelKey;
    String value;
}

public class ObjectDescription {
    List<DescriptionEntry> entries;
}
```

### Dialog layout structure

```
┌─────────────────────────────────┐
│  Object Description     [✕]     │  ← title bar
├─────────────────────────────────┤
│  General                         │  ← section header
│    Type: restaurant              │  ← label: value
│    Name: "Mario's"              │
│  Location                        │  ← section header
│    Address: Main St 12           │
│    Postal Code: 12345            │
│  Contact                         │
│    Phone: +1-555-1234            │
│    Website: example.com          │
│  ...                             │
└─────────────────────────────────┘
```

### Dependencies

- `libosmscout` `DescriptionService` (already part of core library, linked via `osmscout` target)
- `libosmscout-client-java` JNI layer (existing)
- JavaFX (existing dependency of JavaScout)

### Design decisions

- **Object selection**: not strictly closest by distance — pick the most reasonable visible object. Query objects in a small bounding box around the click point, then rank by: (1) has `DescriptionService` data to show, (2) visible on map at current zoom, (3) proximity. Prefer nodes over ways over areas for equal distance. This matches user intent: they long-pressed something they see and want info about.
- **Long-press threshold**: configurable via `Config` property `longPressTimeoutMs` (default 500ms).
- **Scrolling**: `DescriptionOverlay` uses a `ScrollPane` so long descriptions are scrollable.
