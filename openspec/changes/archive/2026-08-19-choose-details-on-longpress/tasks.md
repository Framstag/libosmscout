## 1. Native client: candidate list retrieval (spec: long-press-description — JNI bridge)

- [x] 1.1 Refactor candidate collection/ranking block in `libosmscout-client-java/src/OSMScoutClient.cpp` (L2660-2703) into a shared helper returning the full ordered candidate list (ranked by description data, visibility, proximity; capped at N=10)
- [x] 1.2 Implement `Java_com_framstag_libosmscout_client_OSMScoutClient_getDescriptionCandidates` JNI entry point returning `List<ObjectDescription>`, one per ranked candidate, each carrying its object reference
- [x] 1.3 Add object reference (type + file offset) fields to `ObjectDescription` construction in the JNI layer; fill them for `getDescription` path too
- [x] 1.4 Change existing `getDescription` JNI to delegate to the shared helper (top-ranked candidate) — behavior unchanged
- [x] 1.5 Build `libosmscout-client-java` native library compiles without errors

## 2. Java client API (spec: long-press-description — JNI bridge)

- [x] 2.1 Add native method `List<ObjectDescription> getDescriptionCandidates(double lat, double lon)` to `OSMScoutClient.java`
- [x] 2.2 Extend `ObjectDescription.java` with object ref fields (type name/type, file offset) and a compatible constructor; update `DescriptionEntry` docs if needed
- [x] 2.3 Existing `getDescription(lat, lon)` behavior unchanged: zero entries outside database / with no database loaded

## 3. JavaScout UI: candidate picker (spec: long-press-candidate-picker)

- [x] 3.1 Extract the 3-line search-result cell rendering from `SearchOverlay.java` (L192-229) into a shared builder so candidate entries use the identical format
- [x] 3.2 Create `CandidatePickerOverlay.java` (StackPane, fade/slide animation, click-outside + Escape close, fullscreen <600px, centered ≥600px) listing ranked candidates with the search-result cell format; selecting an entry closes the picker and returns the chosen `ObjectDescription`
- [x] 3.3 Rewire `MainController.onLongPress` (L1801): background task calls `getDescriptionCandidates`; 0 candidates → "No description available" dialog; 1 candidate → `DescriptionOverlay` directly; N candidates → `CandidatePickerOverlay`
- [x] 3.4 `DescriptionOverlay` shows details for the selected candidate unchanged (spec: long-press-description — overlay dialog)

## 4. Verification

- [x] 4.1 Manual check: long-press on overlapping objects (building + street + POI) shows picker with search-result-formatted entries; selection opens details
- [x] 4.2 Manual check: long-press on sparse area with single object shows description directly, no intermediate list
- [x] 4.3 Manual check: long-press on empty area shows "No description available", no error; Escape / click-outside closes picker without details
- [x] 4.4 Run existing JavaScout build and existing tests; confirm no regressions in search overlay and single-object long-press
