## 1. Export Visibility Header

- [x] 1.1 Create `libosmscout-client-java/include/osmscoutclientjava/ClientJavaImportExport.h` with `OSMSCOUT_CLIENT_JAVA_API` macro, following pattern of `ClientQtImportExport.h`
  - Spec: `client-java-build` / Export visibility macros
  - Effort: 1

## 2. Java Classes

- [x] 2.1 Create `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — public class with:
  - `long nativeHandle` field (set from JNI)
  - `native boolean openDatabase(String path)`
  - `native boolean close()`
  - `native boolean isInitialized()`
  - `protected void finalize()` safety net calling `close()` if still alive
  - No `init()` — init is handled by the builder
  - Spec: `java-client-object`
  - Effort: 2
- [x] 2.2 Create `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClientBuilder.java` — fluent builder with:
  - Pure Java fields: `String basemapLookupDirectory`, `String iconDirectory`, `String[] mapLookupDirectories`, `double physicalDpi`, `String units`
  - Fluent `with*` setters returning `this`
  - `native OSMScoutClient build()`
  - Spec: `java-client-object`
  - Effort: 2
- [x] 2.3 Update `libosmscout-client-java/java/meson.build` to include `OSMScoutClientBuilder.java` in JNI headers and JAR
  - Spec: `client-java-build`
  - Effort: 1

## 3. JNI C++ Implementation

- [x] 3.1 Add `InMemorySettingsStorage` class implementing `SettingsStorage` with `std::map<std::string, std::string>`, in `OSMScoutClient.cpp` or dedicated file
  - Spec: `java-client-object`
  - Effort: 1
- [x] 3.2 Implement `Java_com_framstag_libosmscout_client_OSMScoutClientBuilder_build` — create `Settings`, `MapManager`, `DBThread`, call `DBThread->Initialize()`, store opaque `ClientData*`, return new `OSMScoutClient` with native handle set
  - Guard against double init with static pointer
  - Spec: `java-client-object` / Requirement: OSMScoutClient initialization
  - Effort: 3
- [x] 3.3 Implement `Java_..._OSMScoutClient_openDatabase` — retrieve `ClientData` from handle, register path with `MapManager`, trigger `DBThread.OnDatabaseListChanged()`
  - Spec: `java-client-object` / Requirement: Open database
  - Effort: 2
- [x] 3.4 Implement `Java_..._OSMScoutClient_close` — release C++ shared_ptrs, reset static pointer, free `ClientData`
  - Spec: `java-client-object` / Requirement: Close database
  - Effort: 1
- [x] 3.5 Implement `Java_..._OSMScoutClient_isInitialized` — check if native handle is non-zero and C++ objects exist
  - Spec: `java-client-object`
  - Effort: 1

## 4. Build Integration

- [x] 4.1 Add `osmscoutclientjavaIncDir` include path for `ClientJavaImportExport.h` header to `libosmscout-client-java/src/meson.build`
- [x] 4.2 Verify root `meson.build` `buildJava` guard correctly gates the `libosmscout-client-java` subdir
- [x] 4.3 Build and verify: `meson setup build -Dbuild_java=true && meson compile -C build` produces `.so` and `.jar`
  - Spec: `client-java-build` / Meson subproject build + JNI header generation
  - Effort: 2

## 5. Verification

- [x] 5.1 Verify `libosmscout-binding` is NOT referenced or linked anywhere in `libosmscout-client-java`
  - Effort: 1
- [ ] 5.2 Manual smoke test (requires real map data — left for user): write a small `Main.java` that uses `OSMScoutClientBuilder` → `build()` → `openDatabase(path)` → `close()`, compile and run against a real map directory
  - Effort: 2

## 6. JavaScout Demo

- [x] 6.1 Create `JavaScout/src/main/java/com/framstag/libosmscout/JavaScout.java` — demo app opens/closes database, prints debug info
  - Spec: `java-scout-demo`
  - Effort: 2
- [x] 6.2 Create `JavaScout/meson.build` — executable JAR with `link_with` to client Java lib
  - Spec: `java-scout-demo`
  - Effort: 1
- [x] 6.3 Wire `JavaScout` subdir into root `meson.build` inside `if buildJava` block
  - Effort: 1
- [ ] 6.4 Manual smoke test (requires real map data): run JavaScout against a valid .osmscout directory
  - Effort: 2

## Tasks Summary

| Tasks | Story Points |
|---|---|
| 5 — Verification | 3 |
| 6 — JavaScout demo | 5 |
| **Total** | **24** |