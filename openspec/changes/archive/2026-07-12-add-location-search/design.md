## ctx

JavaScout (JavaFX) has `OSMScoutClient` with JNI bindings for open DB, render, close — no location search API exists.

User wants a map overlay search button in JavaScout that expands into a search field + result list. clientjavajar needs new location search API.

## Goals / Non-Goals

**Goals:**
- Map overlay search button on JavaScout
- On press: button expands to search text field + empty result list
- Result list: full-width on small screens, popup on large screens
- Sorted results shown on search return
- Click result → map navigates to location
- Cancel button → back to map overview
- New location search API in `libosmscout-client-java` (clientjavajar)
- JavaScout uses new API for search

**Non-Goals:**
- No changes to core `LocationService` C++ API (already exists)
- No routing integration (existing `SearchDialog` handles routing separately)
- No voice search, no geocoding reverse lookup

## Decisions

### 1. Overlay search button pattern (JavaScout only)

Floating search button over the map. On press, button expands to reveal a text field and result list below. Cancel button collapses back to icon.

**JavaScout (JavaFX):**
- New `SearchOverlay` control in JavaScout UI
- Calls new `OSMScoutClient.searchLocations()` API
- Same responsive pattern via JavaFX

### 2. Java client API design

New method on `OSMScoutClient`:

```java
public native LocationEntry[] searchLocations(String query, int limit);
```

`LocationEntry` mirrors the QML `LocationEntry` type:

```java
package com.framstag.libosmscout.client;

public class LocationEntry {
    public String label;
    public String type;       // "coordinate" or "object"
    public String objectType; // e.g. "place_town", "address", "highway_bus_stop"
    public double lat;
    public double lon;
    public String[] region;   // admin region hierarchy
}
```

JNI implementation in `libosmscout-client-java/src/`:
- Calls `LocationService::SearchForLocationByString()` on the C++ side
- Converts `LocationSearchResult::Entry` list to Java `LocationEntry[]`
- Uses existing `DatabaseRef` from the client

### 3. Sorting

Results sorted by relevance rank (same logic as existing QML `locationRank()`):
- Type priority (country > state > town > address > POI)
- Distance from search center (closer = higher)
- String match quality (exact > prefix > partial)

Sorting happens on the Java side after receiving results from native.

### 4. Map navigation on result click

JavaScout already has `MapRenderer` pan-to-coord. Reuse existing.

## Risks / Trade-offs

- **JNI complexity**: New native method needs careful memory management for string arrays and object arrays returned to Java
- **Search center**: JavaScout currently has no "search center" concept — search will use map center at time of search
- **Threading**: Java search API runs on calling thread. UI should call on background thread to avoid blocking
