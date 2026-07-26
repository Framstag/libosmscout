## Context

JavaScout is a JavaFX desktop app (`JavaScout/`) that uses `libosmscout-client-java` for map rendering, location search, and routing. The Java client wraps a C++ client library (`libosmscout-client/`) via JNI. Currently there is no mechanism to store user-defined favorite locations.

The change adds favorite locations across three layers:
1. **C++** — new `FavoriteLocationService` with JSON file persistence
2. **Java JNI** — new native methods on `OSMScoutClient` delegating to the C++ service
3. **JavaFX UI** — management dialog + fav picker in search/routing

## Goals / Non-Goals

**Goals:**
- Persist grouped favorite locations to a human-readable JSON file
- Expose full CRUD via C++ API and Java JNI bindings
- Integrate fav selection into JavaScout's search overlay and route panel
- Provide a dedicated management dialog for groups and favs
- Keep data model extensible via attribute maps on groups and favs

**Non-Goals:**
- No cloud sync or multi-device support
- No import/export from OSM bookmarks or GPX
- No map pin visualization on the rendered map (future enhancement)
- No changes to the C++ core database format

## Decisions

### Decision 1: JSON over SQLite or custom binary
**Choice:** JSON file with `nlohmann/json` (already available via `libosmscout-extern` or vendored).

**Rationale:**
- Human-readable — users can edit manually
- No schema migrations needed — attribute maps handle extensibility
- Simple to implement — no SQLite dependency or binary format design
- JSON is already used elsewhere in the project (online tile providers, map providers)

**Alternatives considered:**
- SQLite — heavier dependency, schema migrations, overkill for simple key-value data
- Custom binary — not human-readable, needs format versioning, parser maintenance
- Protobuf — adds build complexity, not human-editable

### Decision 2: `FavoriteLocationService` as standalone class, not part of `Settings`
**Choice:** New `FavoriteLocationService` class with its own file I/O, separate from `Settings`.

**Rationale:**
- Fav locations are user data, not configuration — different lifecycle and semantics
- `Settings` is already large and tied to Qt `QSettings` in the Qt client path
- Standalone class is testable without mocking the entire settings infrastructure
- Clean separation of concerns

### Decision 3: Thread safety via `std::shared_mutex`
**Choice:** `std::shared_mutex` with shared lock for reads, exclusive lock for writes.

**Rationale:**
- Multiple UI threads may read favs concurrently (search overlay, route panel)
- Writes are infrequent (user action only) — exclusive lock is acceptable
- `std::shared_mutex` is C++17, available in our C++20 toolchain

### Decision 4: Java JNI methods on `OSMScoutClient`, not a separate class
**Choice:** Add native methods directly to `OSMScoutClient`, store `FavoriteLocationService` in `ClientData`.

**Rationale:**
- Follows existing pattern — all native methods live on `OSMScoutClient`
- `ClientData` already holds the C++ client state — natural place for the service
- Avoids adding another JNI registration class
- `OSMScoutClientBuilder::build()` already initializes all C++ resources

### Decision 5: Fav picker as tab in SearchOverlay, not separate dialog
**Choice:** Add a "Favorites" tab/toggle to the existing `SearchOverlay` panel.

**Rationale:**
- Reuses existing overlay layout, animation, and positioning
- Users already familiar with the search UI for location selection
- Route panel's `LocationPicker` callback already integrates with `SearchOverlay`
- Avoids yet another floating overlay on the map

### Decision 6: Management dialog as modal JavaFX `Stage`
**Choice:** Standalone modal dialog (`FavLocationDialog`) using JavaFX `Stage` with `Modality.APPLICATION_MODAL`.

**Rationale:**
- Management is a separate concern from map browsing — modal dialog is appropriate
- JavaFX `ListView` + `TableView` provide the group/fav hierarchy naturally
- Follows standard desktop UI patterns

### Decision 7: JSON serialization in C++, not Java
**Choice:** The C++ `FavoriteLocationService` handles all JSON serialization. Java passes/receives arrays of Java objects via JNI.

**Rationale:**
- Single source of truth for data format
- If other language bindings (Python, Swift) are added later, they reuse the C++ service
- JNI marshalling of simple data structures (strings, doubles, arrays) is straightforward

## Data Model

### C++ (`FavoriteLocationService.h`)

```cpp
struct FavLocation {
    std::string name;
    double lat;
    double lon;
    std::map<std::string, std::string> attributes;
};

struct FavLocationGroup {
    std::string name;
    std::vector<FavLocation> favorites;
    std::map<std::string, std::string> attributes;
};

class FavoriteLocationService {
public:
    explicit FavoriteLocationService(const std::string& filePath);

    // File I/O
    bool Load();
    bool Save();

    // Group CRUD
    std::vector<FavLocationGroup> GetGroups() const;
    bool AddGroup(const std::string& name);
    bool DeleteGroup(const std::string& name);

    // Fav CRUD
    std::vector<FavLocation> GetFavorites(const std::string& groupName) const;
    bool AddFavorite(const std::string& groupName, const FavLocation& fav);
    bool DeleteFavorite(const std::string& groupName, const std::string& favName);
    bool RenameFavorite(const std::string& groupName,
                        const std::string& oldName,
                        const std::string& newName);

private:
    mutable std::shared_mutex mutex_;
    std::string filePath_;
    std::map<std::string, FavLocationGroup> groups_;  // keyed by group name
};
```

### JSON Format (`favorites.json`)

```json
{
  "groups": {
    "Work": {
      "name": "Work",
      "attributes": {},
      "favorites": [
        {
          "name": "Office",
          "lat": 51.1657,
          "lon": 10.4515,
          "attributes": {}
        }
      ]
    },
    "Home": {
      "name": "Home",
      "attributes": {},
      "favorites": []
    }
  }
}
```

### Java

```java
// New classes in com.framstag.libosmscout.client
public class FavoriteLocation {
    public String name;
    public double lat;
    public double lon;
    public Map<String, String> attributes;
}

public class FavoriteLocationGroup {
    public String name;
    public List<FavoriteLocation> favorites;
    public Map<String, String> attributes;
}

// New methods on OSMScoutClient
public native boolean loadFavoriteLocations(String filePath);
public native boolean saveFavoriteLocations(String filePath, FavoriteLocationGroup[] groups);
public native FavoriteLocationGroup[] getFavoriteGroups();
public native boolean addGroup(String name);
public native boolean deleteGroup(String name);
public native boolean addFavorite(String groupName, String favName, double lat, double lon);
public native boolean deleteFavorite(String groupName, String favName);
public native boolean renameFavorite(String groupName, String oldName, String newName);
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ JavaScout (JavaFX)                                           │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │ SearchOverlay │  │ RoutePanel   │  │ FavLocationDialog │  │
│  │ (fav tab)     │  │ (fav picker) │  │ (modal)           │  │
│  └──────┬───────┘  └──────┬───────┘  └─────────┬─────────┘  │
│         │                 │                    │            │
│         └─────────────────┴────────────────────┘            │
│                              │                              │
│                     ┌────────▼────────┐                     │
│                     │  MainController  │                     │
│                     │  (owns client)  │                     │
│                     └────────┬────────┘                     │
│                              │                              │
│                     ┌────────▼────────┐                     │
│                     │ OSMScoutClient  │                     │
│                     │ (native methods)│                     │
│                     └────────┬────────┘                     │
├──────────────────────────────┼──────────────────────────────┤
│ JNI Layer                    │                              │
│                     ┌────────▼────────┐                     │
│                     │ OSMScoutClient  │                     │
│                     │ .cpp (JNI impl) │                     │
│                     └────────┬────────┘                     │
├──────────────────────────────┼──────────────────────────────┤
│ C++ Library                  │                              │
│                     ┌────────▼────────┐                     │
│                     │FavoriteLocation │                     │
│                     │   Service       │                     │
│                     │ (JSON file I/O) │                     │
│                     └────────┬────────┘                     │
│                              │                              │
│                     ┌────────▼────────┐                     │
│                     │  favorites.json │                     │
│                     │  (~/.config/    │                     │
│                     │   javascout/)   │                     │
│                     └─────────────────┘                     │
└─────────────────────────────────────────────────────────────┘
```

## Sequence: Fav added via management dialog

```
User          FavLocationDialog     MainController    OSMScoutClient    C++ Service      File
  │                  │                    │                 │                │             │
  │ open dialog      │                    │                 │                │             │
  │─────────────────>│                    │                 │                │             │
  │                  │ getFavoriteGroups()│                 │                │             │
  │                  │────────────────────────────────────>│                │             │
  │                  │                    │                 │ GetGroups()    │             │
  │                  │                    │                 │───────────────>│             │
  │                  │                    │                 │  groups[]      │             │
  │                  │                    │                 │<───────────────│             │
  │                  │<────────────────────────────────────│                │             │
  │                  │                    │                 │                │             │
  │ add fav "Cafe"   │                    │                 │                │             │
  │─────────────────>│                    │                 │                │             │
  │                  │ addFavorite()      │                 │                │             │
  │                  │────────────────────────────────────>│                │             │
  │                  │                    │                 │ AddFavorite()  │             │
  │                  │                    │                 │───────────────>│             │
  │                  │                    │                 │  true          │             │
  │                  │                    │                 │<───────────────│             │
  │                  │<────────────────────────────────────│                │             │
  │                  │                    │                 │                │             │
  │ close dialog     │                    │                 │                │             │
  │─────────────────>│                    │                 │                │             │
  │                  │ saveFavoriteLocations()             │                │             │
  │                  │────────────────────────────────────>│                │             │
  │                  │                    │                 │ Save()         │  write      │
  │                  │                    │                 │───────────────>│────────────>│
  │                  │                    │                 │  true          │  ok         │
  │                  │                    │                 │<───────────────│<────────────│
  │                  │<────────────────────────────────────│                │             │
```

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| JSON file corruption on crash during write | Write to temp file first, then atomic rename. `nlohmann/json` has no streaming writer — use `std::ofstream` + `dump(2)` to temp, then `std::filesystem::rename`. |
| Large fav list slows startup | JSON parse of a few hundred favs is sub-millisecond. Not a concern. |
| JNI marshalling overhead for large fav lists | Fav lists are user-scale (tens to low hundreds). Array-based JNI calls are fine. |
| Thread safety bug in JNI bridge | All Java native methods acquire shared/exclusive lock on the C++ side. Java-side caching is avoided — always read from C++ state. |
| `nlohmann/json` not yet in project | Add as header-only dependency in `libosmscout-extern` or vendor directly. It's a single header. |
| Two build systems (CMake + Meson) | Both must be updated for new source files. Follow existing patterns in `libosmscout-client/CMakeLists.txt` and `libosmscout-client/src/meson.build`. |

## Open Questions

All planning questions are resolved:

- Favorite locations are loaded **lazily** on first use of any fav feature.
- The management dialog is reachable from a "Favorites" menu item **and** a star icon next to the search button.
- JSON serialization uses `nlohmann/json` (see Decision 1 and Decision 7).
