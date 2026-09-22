# Tasks

## 1. Service operation (spec: fav-location-service)

- [x] 1.1 Add the positional move to the favorite location service as specified in `fav-location-service` / "A fav can be moved to another position within its group": a public method that takes the group name, the fav name and a zero-based target position, declared with Doxygen in `libosmscout-client/include/osmscoutclient/FavoriteLocationService.h`. Verify: the header declares the method and documents that the position refers to the list after the removal and is clamped to the list bounds.
- [x] 1.2 Implement the method in `libosmscout-client/src/osmscoutclient/FavoriteLocationService.cpp` under the same `std::unique_lock lock(mutex_)` as the other mutators, scanning the group's fav vector for the name, erasing the entry, clamping the target to the list size and inserting it back. Verify: `MoveFavorite` returns false for an unknown group and an unknown fav without touching `groups_`.
- [x] 1.3 Confirm that the persisted representation needs no change, so the moved order reaches the file: `Save()` writes the favs in vector order and `Load()` restores it. Verify: the existing "Favorite order persists across save and load" case in `Tests/src/FavoriteLocationServiceTest.cpp` still passes.

## 2. Java binding (spec: fav-location-java-bindings)

- [x] 2.1 Add the native declaration `boolean moveFavorite(String groupName, String favName, int newIndex)` with Javadoc to `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`, documenting the zero-based position, the clamping and that a negative position means the first position. Verify: the declaration carries Javadoc for every parameter and the return value.
- [x] 2.2 Add the JNI entry point to `libosmscout-client-java/src/OSMScoutClient.cpp`, delegating through the existing `ClientData` handle and mapping a negative Java index to `0` before calling the service. Verify: the function follows the null-handle and string-release pattern of the neighbouring `renameFavorite`/`setStarred` entry points.

## 3. Unit tests (specs: fav-location-service, fav-location-java-bindings)

- [x] 3.1 Cover the ordering contract of the new operation in `Tests/src/FavoriteLocationServiceTest.cpp`: move to the front, into the middle (position refers to the list after the removal), to the last position, a target beyond the end (clamped), a move to the current position, a group with a single fav, and an empty group. Verify: each case asserts the full resulting order, not a single entry.
- [x] 3.2 Cover the failure contract: an unknown group name and an unknown fav name both report false and leave the order unchanged. Verify: the case asserts the unchanged order after both failing calls.
- [x] 3.3 Cover the "The moved fav keeps its data" scenario: a fav carrying coordinates and a starred attribute is moved, then its coordinates and attributes are unchanged and the other favs are intact. Verify: the new case fails when the implementation rebuilds the fav instead of moving it.
- [x] 3.4 Cover "A move affects only the named group": two groups holding favs of the same names, moving one fav in the first group. Verify: the second group's order is asserted unchanged.
- [x] 3.5 Cover "The moved order survives a save and reload cycle": reorder, `Save()`, then read the same file with a second store instance. Verify: the reloaded order equals the reordered order.

## 4. Build and regression verification (specs: fav-location-service, fav-location-java-bindings)

- [x] 4.1 Configure and build the CMake build with the client library, the Java client library and the tests, and verify the build compiles without errors and without new warnings.
- [x] 4.2 Build the Meson build the same way and verify it compiles without errors, so both maintained build systems still accept the change.
- [x] 4.3 Run `Tests/src/FavoriteLocationServiceTest.cpp` through both build systems and verify the new and the existing cases pass (`ctest -R FavoriteLocationServiceTest` for CMake, `meson test` for Meson).
- [x] 4.4 Run the rest of the test suite for the affected libraries (at minimum the Java client and client tests) and verify no existing test regresses.
- [x] 4.5 Run `openspec validate "client-move-favorite" --strict` and verify the change validates.

## 5. Documentation and change hygiene

- [x] 5.1 Verify the public API documentation of the new method is complete (Doxygen in the header, Javadoc in the Java declaration) and that no user-facing document in `Documentation/` or a README describes the favorite operations in a way that is now incomplete.
- [x] 5.2 Verify nothing else changed: `git diff master` touches only the five files listed in the proposal's Impact section, and no build-system, style-sheet or type-definition file is in the diff.

## 6. Verification evidence

- [x] 6.1 CMake: `ninja FavoriteLocationServiceTest osmscout_client_java` builds the client library, the Java client library and the test without errors and without warnings from the touched files (`Tests/FavoriteLocationServiceTest`).
- [x] 6.2 CMake: `ctest -R FavoriteLocationServiceTest` passes, and the full suite passes 88/88 with `--exclude-regex "PerformanceTest"` under `xvfb-run` with `QT_QPA_PLATFORM=offscreen`.
- [x] 6.3 Meson: `meson compile FavoriteLocationServiceTest osmscout_client_java libosmscoutclientjava` builds without errors, and `meson test "Check FavoriteLocationService"` passes.
- [x] 6.4 `openspec validate "client-move-favorite" --strict` reports the change as valid.
