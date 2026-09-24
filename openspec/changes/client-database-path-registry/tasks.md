# Tasks

## 1. Registry component (spec: database-path-registry)

- [x] 1.1 Add the component to `libosmscout-client/include/osmscoutclient/DatabasePathRegistry.h`: the registered set behind one `std::mutex`, a non-copyable class with `Register`, `RegisterAll` (returning the complete set, the per-input disposition and the added count), `Snapshot`, `Size`, `SetChangeCount` and `Clear`, each documented with Doxygen. Verify: the header documents idempotent registration, the value nature of a snapshot, one set change per call, and that a caller may release the lock before using a snapshot.
- [x] 1.2 Implement the component in `libosmscout-client/src/osmscoutclient/DatabasePathRegistry.cpp`, every operation taking the lock, scanning for duplicates instead of appending blindly, and counting one set change per registration call. Verify: a batch of K paths and a loop of K single registrations produce the same set but different set-change counts.
- [x] 1.3 Register the new header and source in `libosmscout-client/CMakeLists.txt` and `libosmscout-client/src/meson.build`. Verify: both build systems compile the new translation unit.

## 2. Java binding (spec: client-java-database-paths)

- [x] 2.1 Replace the `ClientData` path vector with the registry, so the client handle no longer exposes a list another thread can append to. Verify: no code in the bridge reads or writes a path list without going through the registry.
- [x] 2.2 Make `openDatabase(String)` register through the registry and hand the database thread a value snapshot, with the registry lock released before that call. Verify: the database thread receives a value, not the registry's own storage.
- [x] 2.3 Add the batch entry point `openDatabases(String[])`, registering the whole list with one call and asking the database thread to process one set. Verify: exactly one database-set change per call, independent of the list length.
- [x] 2.4 Return an index-aligned boolean array: true when that input directory is part of the registered set afterwards, false for a null or unreadable element, an empty array for a null or empty input, and every entry false when the client has no usable database thread. Verify: the mapping is index-aligned, including for skipped elements.
- [x] 2.5 Add the Java declaration `boolean[] openDatabases(String[] paths)` with Javadoc documenting the batch semantics, the index-aligned result and the null-element case. Verify: the declaration exists in `OSMScoutClient.java`, so the native symbol is reachable from Java.

## 3. Unit tests (specs: database-path-registry, client-java-database-paths)

- [x] 3.1 Add `Tests/src/DatabaseOpenTest.cpp` covering single registration, idempotent double registration and the one-element equivalence. Verify: each case asserts the whole set, not a single entry.
- [x] 3.2 Cover the batch contract: a whole list registered in one call, duplicates inside the list and against the set skipped, an already registered set kept complete, and the added count. Verify: the added count only covers the paths that were not part of the set before.
- [x] 3.3 Cover the set-change contract: one batch of K paths is one set change for K in {1, 8, 64}, while a per-path loop counts one per path, and both end in the same set. Verify: the case fails if the batch is implemented as a loop of single registrations.
- [x] 3.4 Cover that a snapshot is a value: it reports the set it was taken with after further registrations, and the current set is complete and duplicate-free. Verify: the new case fails if the snapshot hands out the registry's own storage.
- [x] 3.5 Cover `Clear` forgetting the set while keeping the set-change count, and concurrent registration from several threads leaving a complete, duplicate-free set. Verify: the concurrent case asserts uniqueness and completeness after all threads joined.
- [x] 3.6 Register the new test in `Tests/CMakeLists.txt` and `Tests/meson.build`. Verify: both build systems build and run it.

## 4. Build and regression verification (specs: database-path-registry, client-java-database-paths)

- [x] 4.1 Build the CMake build with the client library, the Java client library and the tests, and verify it compiles without errors and without warnings from the touched files.
- [x] 4.2 Build the Meson build the same way, including the Java client shared library and the jar, and verify it compiles without errors.
- [x] 4.3 Run `DatabaseOpenTest` through both build systems and verify all cases pass.
- [x] 4.4 Run the rest of the suite and verify no existing test regresses.
- [x] 4.5 Run `openspec validate "client-database-path-registry" --strict` and verify the change validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the new public API is documented: the class contract, the snapshot value semantics, the set-change count, and the Javadoc of the new Java call (including the null-element and unusable-client answers).
- [x] 5.2 Verify the existing Java call is unchanged in signature and result, and that the map lookup directories are untouched (they remain a map-manager mechanism, not part of the registered set).
- [x] 5.3 Verify nothing else changed: the diff touches only the files listed in the proposal's Impact section, and no style-sheet, type-definition or format-version file is in the diff.

## 6. Verification evidence

- [x] 6.1 CMake: `ninja DatabaseOpenTest osmscout_client_java` compiles the registry, the bridge and the tests without errors and without compiler warnings from the touched files.
- [x] 6.2 CMake: `DatabaseOpenTest` passes with 10 cases / 198 assertions, `ctest -R 'DatabaseOpenTest|FavoriteStoreTest'` passes 2/2, and the full suite passes 90/90 with `--exclude-regex "PerformanceTest"` under `xvfb-run` with `QT_QPA_PLATFORM=offscreen`.
- [x] 6.3 Meson: `meson compile DatabaseOpenTest osmscout_client_java libosmscoutclientjava` builds without errors and `meson test "Check database path registration"` passes.
- [x] 6.4 `openspec validate "client-database-path-registry" --strict` reports the change as valid.
- [x] 6.5 The bridge diff is path-registration only: the include, the `ClientData` member, the builder reset, the single-directory call and the new batch function; no unrelated region of `OSMScoutClient.cpp` changes.
