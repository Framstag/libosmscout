## What Changes

Fix broken long-press description dialog in JavaScout. Long press on map should show object details overlay, but dialog does not open.

## Capabilities

### Modified Capabilities
- `long-press-description`: Restore broken long-press → object lookup → description overlay flow in JavaScout

## Impact

### Changes Made (diagnostic logging)
- **`JavaScout/src/main/java/.../MainController.java`** — Added `[MainController]` logging to `onLongPress()`, `showDescriptionOverlay()`, and the background task to trace where the break occurs
- **`JavaScout/src/main/java/.../DescriptionOverlay.java`** — Added `[DescriptionOverlay]` logging to `open()` method
- **`libosmscout-client-java/src/OSMScoutClient.cpp`** — Added `[JNI]` logging to `getDescription()`: database count, candidate count, best candidate selection, entry marshaling

### Tests Added
- **`Tests/src/DescriptionServiceTest.cpp`** — 3 test cases, 17 assertions:
  - `ObjectDescription entry management` — verifies entry add/order
  - `DescriptionService construction` — verifies service instantiation
  - `Candidate ranking logic` — 10 sections testing the ranking algorithm used in JNI `getDescription()` (empty, single, hasData priority, very-close beats contains, small area beats non-containing, smaller area beats larger, type rank, distance tiebreak, large area bonus suppression, very-close beats type rank)
- **`JavaScout/src/test/java/.../ObjectDescriptionTest.java`** — 6 tests for ObjectDescription model
- **`JavaScout/src/test/java/.../DescriptionEntryTest.java`** — 9 tests for DescriptionEntry model
- **`JavaScout/src/test/java/.../ConfigTest.java`** — 4 tests for Config long-press timeout
- **`JavaScout/pom.xml`** — Added JUnit 5 dependency

### Root Cause (not yet identified)
Logging added at all layers. Run JavaScout with long press and check:
1. `[MainController] onLongPress called` — confirms gesture detection works
2. `[MainController] calling client.getDescription` — confirms background task starts
3. `[JNI] getDescription(...)` — confirms JNI entry
4. `[JNI] RunSynchronousJob callback: N databases` — confirms DBThread initialized
5. `[JNI] found N candidates` — confirms objects found near click
6. `[JNI] best candidate idx=... entries=N` — confirms candidate selected
7. `[JNI] marshaling N entries to Java` — confirms data returned to Java
8. `[MainController] getDescription returned N entries` — confirms Java received data
9. `[MainController] showing overlay` — confirms dialog should appear
10. `[DescriptionOverlay] open() called` — confirms overlay construction
11. `[DescriptionOverlay] animation finished` — confirms overlay displayed
