# Tasks

## 1. Store component (spec: fav-location-store)

- [x] 1.1 Add the store component to `libosmscout-client/include/osmscoutclient/FavoriteStore.h`: it owns the service instance behind one `std::mutex`, is non-copyable, exposes the two wholesale replacements (`ReplaceByPath`, `ReplaceAndSave`), the explicit `Shutdown()` and the forwarded group/favorite operations, each documented with Doxygen. Verify: the header documents the atomicity of a replacement, the no-store results, the write-failure behaviour and the store-then-service lock order.
- [x] 1.2 Implement the component in `libosmscout-client/src/osmscoutclient/FavoriteStore.cpp` with every operation taking `std::scoped_lock lock(mutex_)`, the replacements rebuilding the service inside that critical section, and the forwarders returning false (or an empty result) when no store is loaded. Verify: no path reaches the service instance without the lock, and no path takes the service lock before the store lock.
- [x] 1.3 Hold the service instance in an owning smart pointer (`std::unique_ptr<FavoriteLocationService>`) instead of the extracted raw owned pointer, so replacement is an assignment, destruction is `reset()` and no manual `delete` remains (`guidelines/CodeStyles.md` gives raw pointers a non-owning meaning). Verify: the file contains no `delete`/`new` for the service instance.
- [x] 1.4 Register the new header in `libosmscout-client/CMakeLists.txt` and the new source in `libosmscout-client/CMakeLists.txt` and `libosmscout-client/src/meson.build`. Verify: both build systems compile the new translation unit.

## 2. Java binding (spec: fav-location-java-bindings)

- [x] 2.1 Replace the `ClientData` service pointer with a store owned by value and repoint every favorite JNI function at the store, deleting the manual service construction and destruction. Verify: no JNI function constructs or deletes a service instance, and every favorite function forwards to the store.
- [x] 2.2 Report the no-store state instead of faulting: the read functions return their empty result and the mutating functions return `JNI_FALSE` when no store is loaded. Verify: the JNI functions assert only the client handle, not a service pointer.
- [x] 2.3 Destroy the store through `Shutdown()` in the client's close path, so a favorite call in flight cannot run against a destroyed instance. Verify: the close path no longer deletes a service pointer.

## 3. Unit tests (spec: fav-location-store)

- [x] 3.1 Add `Tests/src/FavoriteStoreTest.cpp` with the atomicity contract: a reader thread against repeated replacements, and mutations during a replacement that neither fault nor half-apply. Verify: each read is checked to be either empty or a complete generation.
- [x] 3.2 Cover the persistence contract: a replaced store persists the supplied content unchanged, and a replacement can replace an existing store with different content. Verify: the file is re-read by a new service instance and compared with the supplied content.
- [x] 3.3 Cover the no-store contract: a store that was shut down reports no store, every operation returns its no-store result, a second shutdown is safe, and a later replacement installs content again. Verify: all mutating and reading operations are asserted in that state.
- [x] 3.4 Cover a fresh store that never had a file set, and a replacement whose file cannot be written: the operation reports failure and the store keeps the supplied content. Verify: the new case fails when the replacement is changed to restore the previous generation.
- [x] 3.5 Register the new test in `Tests/CMakeLists.txt` and `Tests/meson.build`. Verify: both build systems build and run it.

## 4. Build and regression verification (specs: fav-location-store, fav-location-java-bindings)

- [x] 4.1 Build the CMake build with the client library, the Java client library and the tests, and verify it compiles without errors and without warnings from the touched files.
- [x] 4.2 Build the Meson build the same way, including the Java client shared library and the jar, and verify it compiles without errors.
- [x] 4.3 Run `FavoriteStoreTest` and `FavoriteLocationServiceTest` through both build systems and verify all cases pass.
- [x] 4.4 Run the rest of the suite and verify no existing test regresses.
- [x] 4.5 Run `openspec validate "client-favorite-store-ownership" --strict` and verify the change validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the new public API is documented: the class contract, both replacements (including the write-failure behaviour and the load-result limitation of the path replacement), the explicit destruction and each forwarded operation.
- [x] 5.2 Verify the Java API surface is unchanged: no Java declaration, signature or return value differs from the previous state.
- [x] 5.3 Verify nothing else changed: the diff touches only the files listed in the proposal's Impact section, and no style-sheet, type-definition or format-version file is in the diff.

## 6. Verification evidence

- [x] 6.1 CMake: `ninja FavoriteStoreTest osmscout_client_java` compiles the store, the bridge and the tests without errors and without compiler warnings from the touched files.
- [x] 6.2 CMake: `FavoriteStoreTest` passes with 6 cases / 361 assertions, `ctest -R FavoriteStoreTest` passes, and the full suite passes 89/89 with `--exclude-regex "PerformanceTest"` under `xvfb-run` with `QT_QPA_PLATFORM=offscreen`.
- [x] 6.3 Meson: `meson compile FavoriteStoreTest osmscout_client_java libosmscoutclientjava` builds without errors and `meson test "Check FavoriteStore" "Check FavoriteLocationService"` passes 2/2.
- [x] 6.4 `openspec validate "client-favorite-store-ownership" --strict` reports the change as valid.
- [x] 6.5 The JNI diff contains favorite-related lines only: includes, the `ClientData` member, the close path and the favorite functions; no unrelated region of `OSMScoutClient.cpp` changes.
