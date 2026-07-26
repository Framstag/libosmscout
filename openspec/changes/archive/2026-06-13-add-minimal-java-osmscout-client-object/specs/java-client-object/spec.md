## ADDED Requirements

> **Note**: `libosmscout-binding` (SWIG-generated) is a separate, independent library. This spec covers `libosmscout-client-java` only — no reuse of or dependency on `libosmscout-binding`.

The system SHALL provide a `OSMScoutClientBuilder` Java class with fluent `with*` setters (pure Java, no JNI) and a `native OSMScoutClient build()` method. The `build()` JNI creates and initialises the underlying C++ client objects (`DBThread`, `MapManager`, `Settings`) and returns an `OSMScoutClient` instance.

The `OSMScoutClient` class SHALL hold a `long nativeHandle` set by JNI, with methods `openDatabase(String)`, `close()`, and `isInitialized()`.

**Builder parameters (pure Java setters):**
- `withBasemapLookupDirectory(String)`
- `withIconDirectory(String)`
- `withMapLookupDirectories(String[])`
- `withPhysicalDpi(double)`
- `withUnits(String)` — `"metrics"` or `"imperial"`

**Return:** `OSMScoutClient` on success, `null` if already initialised.
**Parameters:**
- `basemapLookupDirectory` (String) — path to basemap directory
- `iconDirectory` (String) — path to icon directory
- `physicalDpi` (double) — screen DPI for map rendering
- `units` (String) — measurement system (`"metrics"` or `"imperial"`)
- `mapLookupDirectories` (String[]) — directories to search for map databases

**Return:** `true` on successful initialisation, `false` if already initialised.

#### Scenario: Successful build

- **WHEN** `new OSMScoutClientBuilder().withBasemapLookupDirectory(dir)...withUnits("metrics").build()` is called
- **THEN** the C++ `Settings`, `MapManager`, and `DBThread` objects SHALL be created
- **AND** `DBThread.Initialize()` SHALL be called
- **AND** the method returns a non-null `OSMScoutClient`

#### Scenario: Double build returns null
#### Scenario: Double init returns false

- **WHEN** `init()` is called a second time after a successful first call
- **THEN** the method returns `false` without creating new objects

### Requirement: Open database

The system SHALL provide a method `openDatabase(String path)` that registers a map database directory and triggers database loading via `MapManager` / `DBThread`.

**Parameter:** `path` — absolute filesystem path to a directory containing `.osmscout` map data.

**Return:** `true` if the path was registered, `false` on failure.

#### Scenario: Open valid database path

- **WHEN** `init()` has been called successfully
- **AND** `openDatabase("/data/maps/berlin")` is called with a valid map directory
- **THEN** the path SHALL be registered with `MapManager`
- **AND** `DBThread.OnDatabaseListChanged()` SHALL be triggered
- **AND** the method returns `true`

#### Scenario: Open before init returns false

- **WHEN** `openDatabase(path)` is called before `init()`
- **THEN** the method returns `false`

### Requirement: Close database

The system SHALL provide a `close()` method that releases all C++ resources (DBThread, MapManager, Settings) and resets the initialisation state.

**Return:** `true` if cleanup succeeded, `false` if not initialised.

#### Scenario: Successful close

- **WHEN** client is initialised and `close()` is called
- **THEN** the C++ static singleton pointers SHALL be reset
- **AND** `DBThread` resources SHALL be released
- **AND** the method returns `true`

#### Scenario: Close before init returns false

- **WHEN** `close()` is called before `init()`
- **THEN** the method returns `false`