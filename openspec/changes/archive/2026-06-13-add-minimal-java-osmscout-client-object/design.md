## Context

A new `libosmscout-client-java` library bridges Java/Kotlin clients to `libosmscout-client` C++ objects via JNI. Currently scaffolded with:
- `OSMScoutClient.java` — single `native boolean init()` method
- `OSMScoutClient.cpp` — empty JNI impl
- Meson build that produces `.so` + `.jar`

> **Note**: `libosmscout-binding` (SWIG-generated low-level DB bindings) is a separate, independent library. This change builds `libosmscout-client-java` from scratch with hand-written JNI — no reuse of or dependency on `libosmscout-binding`.

Reference implementation is `libosmscout-client-qt` which provides `OSMScoutQt` — a singleton wrapping `DBThread`, `MapManager`, `Settings` with a `OSMScoutQtBuilder` fluent builder. The Java client mirrors this pattern.

## Goals / Non-Goals

**Goals:**
- Java `OSMScoutClientBuilder` with fluent setters + `build()` that returns `OSMScoutClient`
- Java `OSMScoutClient` with `openDatabase(path)`, `close()`, `isInitialized()`
- JNI C++ impl that creates/manages `DBThread`, `MapManager`, `Settings` from `libosmscout-client`
- Meson build compiling and linking correctly
- Minimal surface: no map rendering, routing, or search yet

**Non-Goals:**
- Full Qt-equivalent feature set (routing, lookup modules, elevation, etc.)
- Android-specific packaging or SDK integration
- Thread management for background services (DBThread handles its own)
- Extending or modifying `libosmscout-binding` (separate SWIG project)

## Decisions

### 1. Builder Pattern (matching OSMScoutQtBuilder)

**Decision**: Provide a Java `OSMScoutClientBuilder` class with fluent `with*` setters (pure Java, no JNI) and a native `build()` method. Returns an `OSMScoutClient` instance (or null on failure). The C++ side stores objects in a global static pointer (singleton) like `OSMScoutQt`, but each Java builder `build()` call creates exactly one active instance.

```java
OSMScoutClient client = new OSMScoutClientBuilder()
    .withBasemapLookupDirectory(basemapDir)
    .withIconDirectory(iconDir)
    .withMapLookupDirectories(dirs)
    .withPhysicalDpi(130.0)
    .withUnits("metrics")
    .build();
```

**Rationale**: Mirrors the `OSMScoutQtBuilder` / `OSMScoutQt` pair exactly. The Qt library uses builder → `Init()` → singleton. Here builder → `build()` → returned instance. Fluent setters match idiomatic Java/Kotlin builder conventions.

**Alternative considered**: Constructor with many params — discarded, unreadable and brittle.

### 2. Settings Storage Without Qt

**Decision**: Add a minimal `InMemorySettingsStorage` implementation in `libosmscout-client-java` C++ side that wraps a `std::map<std::string, std::string>`.

**Rationale**: `Settings` requires a `SettingsStorage` pointer. `OSMScoutQt` uses `QtSettingsStorage(QSettings*)`. For Java we provide a simple in-memory key-value store — sufficient for minimal open/close.

**Alternative considered**: Exposing `SettingsStorage` as Java abstract class with native callback — discarded as overly complex for minimal scope; can be added later.

### 3. DBThread / MapManager / Settings Wiring

**Decision**: `build()` JNI creates all three C++ objects, stores them in a static struct (singleton). The returned Java `OSMScoutClient` holds a `long nativeHandle` referencing an opaque C++ `ClientData` pointer.

```
build() →
  InMemorySettingsStorage → Settings(dpi, units)
  MapManager(lookupDirs)
  DBThread(basemapDir, iconDir, settings, mapManager, customPoiTypes)
  DBThread->Initialize()
  → new OSMScoutClient(nativeHandle)
```

`openDatabase(path)` retrieves `ClientData` from handle, registers path via `MapManager`, triggers `DBThread.OnDatabaseListChanged()`.
`close()` releases shared_ptrs, resets static pointer, frees `ClientData`.

**Rationale**: Mirrors `OSMScoutQt` Init/FreeInstance lifecycle. Parameters come from Java builder, avoiding platform-dependent system queries in JNI.

### 4. JNI Header + Method Mapping

**Decision**: Meson `native_headers()` auto-generates JNI header from `OSMScoutClientBuilder.java` + `OSMScoutClient.java`.

| Java | JNI C++ |
|---|---|
| `OSMScoutClientBuilder.build()` | `Java_..._OSMScoutClientBuilder_build` |
| `OSMScoutClient.openDatabase(String)` | `Java_..._OSMScoutClient_openDatabase` |
| `OSMScoutClient.close()` | `Java_..._OSMScoutClient_close` |
| `OSMScoutClient.isInitialized()` | `Java_..._OSMScoutClient_isInitialized` |

Builder fluent setters are pure Java — no JNI overhead.

### 5. Export Visibility Macros

**Decision**: Add `ClientJavaImportExport.h` with `OSMSCOUT_CLIENT_JAVA_API` macro, following pattern in `ClientImportExport.h` and `ClientQtImportExport.h`.

**Rationale**: Required for shared library symbol visibility on Windows and GCC.

## Risks / Trade-offs

**[Risk] Singleton C++ state vs multiple Java builder calls** → C++ state is a single static pointer. Calling `build()` twice without `close()` first returns null or asserts. Mitigation: guard with static pointer, same as `OSMScoutQt`.

**[Risk] Memory management across GC boundary** → Java `close()` must release C++ references. If GC collects `OSMScoutClient` without `close()`, C++ resources leak. Mitigation: document mandatory `close()`. Add `finalize()` as safety net.

**[Risk] No Qt event loop** → `DBThread` uses `AsyncWorker` which expects a running event loop. Verify if `DBThread::Initialize()` / `OnDatabaseListChanged()` blocks or requires async dispatch.

**[Risk] Settings defaults may be wrong on Android** → DPI and units passed from Java builder params, caller responsible for system-appropriate values.