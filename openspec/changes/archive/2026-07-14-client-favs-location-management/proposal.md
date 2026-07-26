## What Changes

Add favorite locations management across three layers of the libosmscout stack:

1. **C++ client library** (`libosmscout-client/`) — new `FavoriteLocationService` with JSON-file persistence for grouped favorite locations
2. **Java client library** (`libosmscout-client-java/`) — JNI bindings exposing the C++ service to Java via `OSMScoutClient`
3. **JavaScout app** (`JavaScout/`) — UI integration: fav location picker in search/routing dialogs + dedicated management dialog

## Capabilities

### New Capabilities

- `fav-location-service`: C++ service in `libosmscout-client/` for CRUD on favorite locations persisted to a JSON file. Supports one-level grouping. Groups have a name. Groups can be added/deleted. Each group holds a list of favs (or empty). Each fav has a name and `GeoCoord`. Data model uses `std::map<std::string, std::vector<FavLocation>>` with extensible attribute maps on both groups and favs for future fields.

- `fav-location-java-bindings`: JNI bindings in `libosmscout-client-java/` exposing the C++ service. New Java class `FavoriteLocation` (name, lat, lon) and `FavoriteLocationGroup` (name, list of `FavoriteLocation`). New native methods on `OSMScoutClient`: `loadFavoriteLocations(String filePath)`, `saveFavoriteLocations(String filePath, FavoriteLocationGroup[] groups)`, `getFavoriteGroups()`, `addGroup(String name)`, `deleteGroup(String name)`, `addFavorite(String groupName, String favName, double lat, double lon)`, `deleteFavorite(String groupName, String favName)`, `renameFavorite(String groupName, String oldName, String newName)`.

- `javascout-fav-location-ui`: JavaFX UI in `JavaScout/`:
  - **Fav management dialog**: modal window listing groups and their favs, with add/delete/rename actions
  - **Fav picker in search overlay**: tab or toggle to browse/search favorite locations alongside OSM search results
  - **Fav picker in route panel**: ability to select a favorite as start or destination location
  - **Persistence**: fav locations stored in a JSON file next to the existing `config.properties` in the OS-specific config directory

### Modified Capabilities

*(none — no existing capability has spec-level requirement changes)*

## Impact

| Area | Impact |
|------|--------|
| `libosmscout-client/include/osmscoutclient/` | New header: `FavoriteLocationService.h` |
| `libosmscout-client/src/osmscoutclient/` | New impl: `FavoriteLocationService.cpp` |
| `libosmscout-client-java/java/.../client/` | New Java class `FavoriteLocation.java`, new native methods on `OSMScoutClient.java` |
| `libosmscout-client-java/src/` | New JNI impl in `OSMScoutClient.cpp` (or new file) |
| `libosmscout-client-java/java/meson.build` | Add new Java source files |
| `libosmscout-client-java/src/meson.build` | Add new C++ source files |
| `JavaScout/src/main/java/.../` | New `FavLocationDialog.java`, modifications to `SearchOverlay.java`, `RoutePanel.java`, `MainController.java` |
| `JavaScout/src/main/resources/.../main.fxml` | May need layout adjustments for new dialog |
| `JavaScout/pom.xml` | No JSON dependency needed; serialization handled in C++ layer |
| `libosmscout-client/CMakeLists.txt` | Add new source files |
| `libosmscout-client-java/CMakeLists.txt` | Add new source files |
