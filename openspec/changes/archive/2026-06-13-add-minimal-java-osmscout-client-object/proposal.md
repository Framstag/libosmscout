## Why

Enable Java/Kotlin clients (e.g. Android apps) to use libosmscout offline map data. Existing `libosmscout-binding` (SWIG-generated, low-level DB types) is a separate, independent library — this change does not touch or build upon it.
The `libosmscout-client-qt` library provides high-level client lifecycle via `OSMScoutQt`, but no equivalent exists for Java/JNI. Adding a minimal `OSMScoutClient` Java object allows opening/closing a database through the existing `libosmscout-client` abstractions (`DBThread`, `MapManager`, `Settings`), filling the gap for JVM-based clients.

## What Changes

- New `libosmscout-client-java` subproject with Meson build (already scaffolded)
- `OSMScoutClient.java` — Java class with `openDatabase()`, `close()`, `isInitialized()` native methods, native handle pattern
- `OSMScoutClientBuilder.java` — fluent builder with `with*` setters (pure Java) + `native OSMScoutClient build()`
- `OSMScoutClient.cpp` — JNI implementation using `DBThread`, `MapManager`, `Settings`, `InMemorySettingsStorage`
- JNI header generated via Meson `native_headers()`
- Add `ClientJavaImportExport.h` with visibility macros for the Java client library
- Wire new subproject into root `meson.build`
- `JavaScout/` demo application — opens/closes database, prints debug info, built as executable JAR

## Capabilities

### New Capabilities
- `java-client-object`: Java `OSMScoutClient` class with builder pattern lifecycle (open database path, close, isInitialized)
- `client-java-build`: Meson build integration, JNI header generation, shared library + JAR output
- `java-scout-demo`: JavaScout demo application showing open/close workflow

### Modified Capabilities
- *(none)* — no existing spec-level requirements change

## Impact

- **New directory**: `libosmscout-client-java/` with `java/` and `src/` subtrees
- **New library**: `libosmscout_client_java.so` (shared) + `libosmscoutclientjava.jar`
- **New demo**: `JavaScout/` directory with `meson.build`, executable JAR, depends on client Java library
- **Dependency**: JNI (`jniDep`), linked against `libosmscout`, `libosmscout-map`, `libosmscout-client`
- **Root meson.build**: adds `subdir('libosmscout-client-java')` + `subdir('JavaScout')` guarded by `buildJava`
- **libosmscout-client**: `InMemorySettingsStorage` provided entirely in new library, no changes needed to existing `libosmscout-client`