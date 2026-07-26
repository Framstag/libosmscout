## 1. C++ FavoriteLocationService

- [x] 1.1 Add `nlohmann/json` header to `libosmscout-extern` (or vendor in `libosmscout-client`)
- [x] 1.2 Create `FavoriteLocationService.h` with `FavLocation` and `FavLocationGroup` structs and `FavoriteLocationService` class
- [x] 1.3 Implement `FavoriteLocationService::Load()` — parse JSON file into in-memory group map
- [x] 1.4 Implement `FavoriteLocationService::Save()` — serialize group map to JSON file (temp file + atomic rename)
- [x] 1.5 Implement group CRUD: `GetGroups()`, `AddGroup()`, `DeleteGroup()`
- [x] 1.6 Implement fav CRUD: `GetFavorites()`, `AddFavorite()`, `DeleteFavorite()`, `RenameFavorite()`
- [x] 1.7 Add thread safety with `std::shared_mutex` (shared lock for reads, exclusive for writes)
- [x] 1.8 Add `FavoriteLocationService` to CMakeLists.txt and meson.build in `libosmscout-client/`
- [x] 1.9 Write Catch2 unit tests for `FavoriteLocationService` (file I/O, CRUD, thread safety)

## 2. Java JNI Bindings

- [x] 2.1 Create `FavoriteLocation.java` — Java bean with name, lat, lon, attributes map
- [x] 2.2 Create `FavoriteLocationGroup.java` — Java bean with name, fav list, attributes map
- [x] 2.3 Add native method declarations to `OSMScoutClient.java`: `loadFavoriteLocations`, `saveFavoriteLocations`, `getFavoriteGroups`, `addGroup`, `deleteGroup`, `addFavorite`, `deleteFavorite`, `renameFavorite`
- [x] 2.4 Implement JNI methods in `OSMScoutClient.cpp` — marshal Java arrays to C++ structs and delegate to `FavoriteLocationService`
- [x] 2.5 Add `FavoriteLocationService` instance to `ClientData` struct and initialize in `OSMScoutClientBuilder::build()`
- [x] 2.6 Add new Java source files to `libosmscout-client-java/java/meson.build`
- [x] 2.7 Add new C++ source files to `libosmscout-client-java/src/meson.build` and CMakeLists.txt

## 3. JavaScout Fav Management Dialog

- [x] 3.1 Create `FavLocationDialog.java` — modal JavaFX Stage with group ListView and fav ListView
- [x] 3.2 Implement "Add group" action — prompt for name, call `client.addGroup()`
- [x] 3.3 Implement "Delete group" action — confirm, call `client.deleteGroup()`
- [x] 3.4 Implement "Add favorite" action — prompt for name + coordinates, call `client.addFavorite()`
- [x] 3.5 Implement "Delete favorite" action — confirm, call `client.deleteFavorite()`
- [x] 3.6 Implement "Rename favorite" action — prompt for new name, call `client.renameFavorite()`
- [x] 3.7 Wire dialog save on close — call `client.saveFavoriteLocations()`
- [x] 3.8 Add "Manage favorites" menu item or button to `MainController`

## 4. JavaScout Search Overlay Fav Tab

- [x] 4.1 Add tab/toggle to `SearchOverlay` for switching between OSM search and favorites
- [x] 4.2 Implement fav browsing mode — display group tree, show favs on group select
- [x] 4.3 Wire fav selection to map navigation (same as OSM result navigation)
- [x] 4.4 Load favorites lazily on first fav tab activation

## 5. JavaScout Route Panel Fav Integration

- [x] 5.1 Extend `RoutePanel.LocationPicker` to support "From favorites" option
- [x] 5.2 Implement fav picker flow — show group/fav tree, return selected `LocationEntry`
- [x] 5.3 Wire fav selection to set start or destination in route panel

## 6. Persistence Wiring

- [x] 6.1 Add `favorites.json` path to `Config` class (same OS-specific config directory)
- [x] 6.2 Load favorites on app startup in `MainController` (lazy, on first fav feature use)
- [x] 6.3 Save favorites on dialog close and on app shutdown
