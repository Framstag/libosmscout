## ADDED Requirements

### Requirement: Java class `FavoriteLocation` models a single favorite
The system SHALL provide a Java class `com.framstag.libosmscout.client.FavoriteLocation` with public fields: `String name`, `double lat`, `double lon`, and a `Map<String, String> attributes` for extensibility.

#### Scenario: FavoriteLocation stores name and coordinates
- **WHEN** a `FavoriteLocation` is constructed with name, lat, and lon
- **THEN** the fields SHALL be readable via public access

#### Scenario: FavoriteLocation supports extensible attributes
- **WHEN** attributes are set on a `FavoriteLocation`
- **THEN** they SHALL be readable via the attributes map

### Requirement: Java class `FavoriteLocationGroup` models a group
The system SHALL provide a Java class `com.framstag.libosmscout.client.FavoriteLocationGroup` with public fields: `String name`, `List<FavoriteLocation> favorites`, and a `Map<String, String> attributes` for extensibility.

#### Scenario: Group stores name and fav list
- **WHEN** a `FavoriteLocationGroup` is constructed with a name
- **THEN** the name SHALL be readable and the fav list SHALL be empty

#### Scenario: Group supports extensible attributes
- **WHEN** attributes are set on a group
- **THEN** they SHALL be readable via the attributes map

### Requirement: `OSMScoutClient` exposes native methods for fav location CRUD
The system SHALL add the following native methods to `OSMScoutClient`:

- `boolean loadFavoriteLocations(String filePath)` — loads favs from JSON file, returns true on success
- `boolean saveFavoriteLocations(String filePath, FavoriteLocationGroup[] groups)` — saves groups to JSON file, returns true on success
- `FavoriteLocationGroup[] getFavoriteGroups()` — returns all loaded groups
- `boolean addGroup(String name)` — adds a new empty group, returns false if name exists
- `boolean deleteGroup(String name)` — deletes a group, returns false if not found
- `boolean addFavorite(String groupName, String favName, double lat, double lon)` — adds a fav to a group, returns false if group not found or duplicate name
- `boolean deleteFavorite(String groupName, String favName)` — deletes a fav from a group, returns false if not found
- `boolean renameFavorite(String groupName, String oldName, String newName)` — renames a fav, returns false if old not found or new name exists

#### Scenario: Load and get groups round-trips
- **WHEN** `loadFavoriteLocations` is called with a valid file path
- **THEN** `getFavoriteGroups` SHALL return the groups from that file

#### Scenario: Add group succeeds
- **WHEN** `addGroup` is called with a new name
- **THEN** the group SHALL appear in `getFavoriteGroups`

#### Scenario: Add duplicate group returns false
- **WHEN** `addGroup` is called with an existing name
- **THEN** it SHALL return false

#### Scenario: Add favorite to group succeeds
- **WHEN** `addFavorite` is called with valid group name, fav name, and coordinates
- **THEN** the fav SHALL appear in that group's list

#### Scenario: Add duplicate favorite returns false
- **WHEN** `addFavorite` is called with a fav name that already exists in the group
- **THEN** it SHALL return false

#### Scenario: Delete favorite succeeds
- **WHEN** `deleteFavorite` is called with valid group and fav names
- **THEN** the fav SHALL be removed from the group

#### Scenario: Rename favorite succeeds
- **WHEN** `renameFavorite` is called with valid group, old name, and new name
- **THEN** the fav SHALL have the new name

#### Scenario: Save and reload preserves data
- **WHEN** groups are modified and saved via `saveFavoriteLocations`, then loaded again via `loadFavoriteLocations`
- **THEN** all groups and favs SHALL match the saved state

### Requirement: JNI C++ layer delegates to `FavoriteLocationService`
The native implementation of each Java method SHALL delegate to the C++ `FavoriteLocationService` class. The `ClientData` struct SHALL hold a `FavoriteLocationService` instance. The service SHALL be initialized during `OSMScoutClientBuilder::build()`.

#### Scenario: Service is initialized at build time
- **WHEN** `OSMScoutClientBuilder.build()` succeeds
- **THEN** the `ClientData` SHALL contain a ready `FavoriteLocationService`

#### Scenario: Java methods call through to C++
- **WHEN** any fav location Java method is called
- **THEN** it SHALL invoke the corresponding C++ method on `FavoriteLocationService`
