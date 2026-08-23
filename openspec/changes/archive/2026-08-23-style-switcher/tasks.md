## 1. Client-java native API (JNI)

Spec: `specs/client-java-style-switching/spec.md` — "Client switches active style at runtime", "Client enumerates available styles". Design: D1, D2.

- [x] 1.1 Implement `getStyleSheetDirectory` JNI method in `libosmscout-client-java/src/OSMScoutClient.cpp` reading `Settings::GetStyleSheetDirectory()`; verify via unit test that it returns the configured directory and falls back to `"stylesheets"` when unset
- [x] 1.2 Implement `getActiveStyleSheet` JNI method reading `Settings::GetStyleSheetFile()`; verify via unit test it returns `"standard.oss"` by default
- [x] 1.3 Implement `loadStyleSheet(String name)` JNI method that resolves `dir + "/" + name + ".oss"`, calls `Settings::SetStyleSheetFile(file)` then `DBThread::LoadStyle(file, currentFlags)`; verify via unit test it returns false for a missing stylesheet and leaves the previous active style unchanged
- [x] 1.4 Verify both CMake and Meson builds of `libosmscout-client-java` compile the new native methods without errors

## 2. Client-java Java API

### Spec: `spec-java-style-switching` (same as above). Design: D1.

- [x] 2.1 Declare native methods `getStyleSheetDirectory()`, `getActiveStyleSheet()`, `loadStyleSheet(String)` in `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` with Javadoc
- [x] 2.2 Implement `getAvailableStyleSheets()` in `OSMScoutClient.java` scanning the stylesheet directory for top-level `*.oss` files, deriving names without the `.oss` extension, sorted; verify empty directory yields empty list and non-`.oss` files are ignored
- [x] 2.3 Add JUnit tests in `JavaScout/src/test/java/com/framstag/libosmscout/client/` covering enumeration (temp dir with `standard.oss`, `cycle.oss`, `notes.txt`), successful switch, switch to unknown style, unloadable stylesheet keeping previous style, active-style query, and flags surviving a switch; verify with `./mvnw test`

## 3. JavaScout UI

### Spec: `spec-javascout-style-switcher`. Design: D3, D4.

- [x] 3.1 Add "Switch Style…" `MenuItem` to the main menu in `MainController.createMainMenuButton()`; verify manually that the item is visible in the menu
- [x] 3.2 Create style chooser dialog with a `ComboBox<String>` populated from `client.getAvailableStyleSheets()`, preselected to `client.getActiveStyleSheet()` (name without `.oss`); verify dialog lists styles and disables confirm when no styles exist
- [x] 3.3 Wire dialog confirmation to `client.loadStyleSheet(name)`; on success call `renderer.requestRenderPreserveRoute(...)`; on failure show error message and keep current style — verify switching to "cycle" redraws the map with the cycle style and preserves center/zoom, and cancelling changes nothing

## 4. Build & verification

- [x] 4.1 Build `libosmscout-client-java` (CMake and Meson) and JavaScout (Maven) with no new warnings/errors
- [x] 4.2 Run the existing JavaScout test suite (`./mvnw test` in `JavaScout/`) and confirm all previously passing tests still pass
- [x] 4.3 Manual smoke test: launch JavaScout, switch style via menu dialog, confirm immediate redraw with preserved view and that an invalid style keeps the previous rendering
