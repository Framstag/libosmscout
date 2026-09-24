## Purpose

Expose favorite location data and CRUD operations to Java clients through the `libosmscout-client-java` JNI layer.

## Requirements

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
- `boolean moveFavorite(String groupName, String favName, int newIndex)` — moves a fav to the given 0-based position inside its group, returns false if the group or the fav is not found

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

#### Scenario: Move favorite reorders the group
- **GIVEN** a group whose favs are ordered `A,B,C,D`
- **WHEN** `moveFavorite` is called for `D` with position `0`
- **THEN** it SHALL return true
- **AND** `getFavoriteGroups` SHALL report the order `D,A,B,C` for that group

#### Scenario: A negative target position means the first position
- **GIVEN** a loaded group in which a fav sits at a later position
- **WHEN** `moveFavorite` is called for that fav with a negative position
- **THEN** it SHALL return true
- **AND** the fav SHALL be the first entry of its group

#### Scenario: Move of an unknown group or fav returns false
- **GIVEN** a loaded group
- **WHEN** `moveFavorite` is called with a group name that was not loaded, and with a fav name that group does not contain
- **THEN** both calls SHALL return false
- **AND** no group SHALL change its fav order

#### Scenario: A moved position reaches the file
- **GIVEN** a loaded group whose fav order was changed by `moveFavorite`
- **WHEN** `saveFavoriteLocations` is called and the file is loaded again
- **THEN** the group SHALL report the moved fav order

### Requirement: JNI C++ layer delegates through the client's favorite store
The native implementation of each Java method SHALL delegate to the C++ `FavoriteLocationService` class through the client's own favorite store. The `ClientData` struct SHALL own that store by value and SHALL NOT own a service instance itself; the store owns the service instance. A client SHALL start without a loaded store, and the first load or save SHALL install one. While no store is loaded, the native side of the favorite methods SHALL report the documented "no store" results instead of faulting. The client's shutdown path SHALL destroy the store through the store's own destruction operation, so that a favorite call running on another thread cannot operate on a destroyed instance.

#### Scenario: Client starts without a loaded store
- **GIVEN** an `OSMScoutClient` built without a favorite file having been loaded
- **WHEN** `getFavoriteGroups` is called
- **THEN** it SHALL return no groups
- **AND** a favorite changing call SHALL report failure instead of faulting

#### Scenario: Loading a file installs a store
- **WHEN** `loadFavoriteLocations` is called with a valid file path
- **THEN** the client SHALL hold a store backed by that file
- **AND** `getFavoriteGroups` SHALL return the groups of that file

#### Scenario: Saving replaces the store as one operation
- **WHEN** `saveFavoriteLocations` is called with a group array
- **THEN** the store SHALL be replaced with that content in one step
- **AND** a concurrent `getFavoriteGroups` SHALL observe either the previous content or the new content, never a partially rebuilt one

#### Scenario: Java methods call through to C++
- **WHEN** any fav location Java method is called
- **THEN** it SHALL invoke the corresponding operation on the client's store, which forwards it to `FavoriteLocationService`

#### Scenario: A call in flight when the client closes
- **GIVEN** a client with a loaded store and a favorite call running on another thread
- **WHEN** the client is closed
- **THEN** the store SHALL be destroyed under its own lock, after the call has left it
- **AND** the closing call SHALL NOT delete an instance that another call is still using
