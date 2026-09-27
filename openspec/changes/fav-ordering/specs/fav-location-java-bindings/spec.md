# Spec Delta

## ADDED Requirements

### Requirement: Java class `StarredFavoriteLocation` models a starred entry with its group

The system SHALL provide a Java class `com.framstag.libosmscout.client.StarredFavoriteLocation` that
carries a starred favorite together with the name of the group that holds it, so a Java client can both
navigate to the favorite and select the group it lives in. The class SHALL expose the group name as a
public field `String groupName` and the favorite as a public field `FavoriteLocation favorite`.

#### Scenario: A starred entry names its group and its favorite

- **WHEN** a `StarredFavoriteLocation` is returned for a starred favorite held by the group `Work`
- **THEN** `groupName` SHALL be `"Work"`
- **AND** `favorite` SHALL carry the name and coordinates of that favorite

### Requirement: `OSMScoutClient` exposes native methods for group, cross-group and starred order

The system SHALL add the following native methods to `OSMScoutClient`:

- `boolean moveGroup(String groupName, int newIndex)` — moves a group to the given 0-based position in
  the group order, returns false if the group is not found
- `boolean moveFavoriteToGroup(String groupName, String favName, String targetGroupName, int newIndex)` —
  moves a fav into another group at the given 0-based position, returns false if either group or the fav
  is not found, and returns false without changing anything when the target group already holds a fav of
  that name
- `boolean moveStarredFavorite(String groupName, String favName, int newIndex)` — moves a starred fav to
  the given 0-based position in the starred order, returns false if the group or the fav is not found or
  the fav is not starred
- `StarredFavoriteLocation[] getStarredFavorites()` — returns the starred favorites in their order,
  each entry naming its group

A negative target position SHALL mean the first position, matching the existing `moveFavorite` behaviour.
The group order SHALL be what `getFavoriteGroups` returns.

#### Scenario: Move a group reorders the group list

- **GIVEN** a loaded store whose groups are ordered `Work,Home,Uni`
- **WHEN** `moveGroup` is called for `Uni` with position `0`
- **THEN** it SHALL return true
- **AND** `getFavoriteGroups` SHALL report the order `Uni,Work,Home`

#### Scenario: A negative group position means the first position

- **GIVEN** a loaded store in which a group sits at a later position
- **WHEN** `moveGroup` is called for that group with a negative position
- **THEN** it SHALL return true
- **AND** the group SHALL be the first entry of `getFavoriteGroups`

#### Scenario: Moving a favorite into another group reorders both groups

- **GIVEN** a loaded group `Home` whose favs are ordered `A,B,C` and a group `Work` whose favs are
  ordered `X,Y`
- **WHEN** `moveFavoriteToGroup` is called for `B` with target group `Work` and position `1`
- **THEN** it SHALL return true
- **AND** `getFavoriteGroups` SHALL report `A,C` for `Home` and `X,B,Y` for `Work`

#### Scenario: A colliding name in the target group returns false

- **GIVEN** a loaded group `Home` containing `B` and a group `Work` that also contains a fav named `B`
- **WHEN** `moveFavoriteToGroup` is called for `B` with target group `Work`
- **THEN** it SHALL return false
- **AND** both groups SHALL keep their favs and their orders

#### Scenario: Move of an unknown group or fav returns false

- **GIVEN** a loaded store
- **WHEN** `moveGroup` is called for a group that was not loaded, and `moveFavoriteToGroup` is called
  with a group or a fav name that does not exist
- **THEN** all calls SHALL return false
- **AND** no group order and no fav order SHALL change

#### Scenario: Starred favorites are returned in order with their groups

- **GIVEN** a loaded store whose starred favorites were arranged as `Gym,Office`
- **WHEN** `getStarredFavorites` is called
- **THEN** it SHALL return `Gym` before `Office`
- **AND** each entry SHALL name the group that holds it

#### Scenario: Moving a starred favorite reorders the starred list

- **GIVEN** a loaded store whose starred favorites are ordered `Office,Home,Gym`
- **WHEN** `moveStarredFavorite` is called for `Gym` with position `0`
- **THEN** it SHALL return true
- **AND** `getStarredFavorites` SHALL report the order `Gym,Office,Home`

#### Scenario: Moving a favorite that is not starred returns false

- **GIVEN** a loaded store with a favorite that is not starred
- **WHEN** `moveStarredFavorite` is called for that favorite
- **THEN** it SHALL return false
- **AND** the starred order SHALL be unchanged

#### Scenario: The arranged orders reach the file

- **GIVEN** a loaded store whose group order, fav groups and starred order were changed by the ordering
  methods
- **WHEN** `saveFavoriteLocations` is called and the file is loaded again
- **THEN** the group order SHALL be as arranged
- **AND** the favs SHALL be in the groups they were moved to, in the arranged order
- **AND** the starred order SHALL be as arranged

#### Scenario: A moved group and a moved favorite keep their data through Java

- **GIVEN** a group that carries a color attribute and a favorite that carries attributes and a star
- **WHEN** the group is moved and the favorite is moved into another group
- **THEN** the group color SHALL be unchanged
- **AND** the favorite's attributes SHALL be unchanged
- **AND** the favorite SHALL still be starred

### Requirement: `OSMScoutClient` exposes the favorites file version state

The system SHALL add the following native methods to `OSMScoutClient`, so a Java client can tell a file
written by a newer client apart from a file without favorites:

- `int getFavoriteFileFormatVersion()` — the format version of the loaded favorites file, or `-1` when no
  file is loaded
- `boolean isFavoriteFileFormatSupported()` — whether the loaded file's format version is one this client
  can read and persist; false when the file's version is newer and false when no file is loaded

While the loaded file's version is unsupported, `getFavoriteGroups` SHALL return no groups and the
favorite changing methods, including `saveFavoriteLocations`, SHALL return false, so a Java client cannot
overwrite a file it does not understand.

#### Scenario: A file written by a newer client is reported as unsupported

- **GIVEN** a favorites file that carries a version newer than the client knows
- **WHEN** `loadFavoriteLocations` is called for it
- **THEN** `isFavoriteFileFormatSupported` SHALL return false
- **AND** `getFavoriteFileFormatVersion` SHALL return the version found in the file
- **AND** `getFavoriteGroups` SHALL return no groups

#### Scenario: A file written by a newer client cannot be overwritten from Java

- **GIVEN** a client opened on a file whose version is newer than the client knows
- **WHEN** `saveFavoriteLocations` is called
- **THEN** it SHALL return false
- **AND** the file SHALL still hold its content unchanged

#### Scenario: A file written by this client is reported as supported

- **GIVEN** a favorites file written by this client
- **WHEN** `loadFavoriteLocations` is called for it
- **THEN** `isFavoriteFileFormatSupported` SHALL return true
- **AND** `getFavoriteFileFormatVersion` SHALL return the version this client writes

#### Scenario: A pre-version file is reported as supported

- **GIVEN** a favorites file in the pre-version form
- **WHEN** `loadFavoriteLocations` is called for it
- **THEN** it SHALL return true
- **AND** `isFavoriteFileFormatSupported` SHALL return true
- **AND** `getFavoriteGroups` SHALL return the groups of that file

#### Scenario: A client without a loaded file reports no version

- **GIVEN** an `OSMScoutClient` without a loaded favorites file
- **WHEN** the version state is queried
- **THEN** `getFavoriteFileFormatVersion` SHALL return `-1`
- **AND** `isFavoriteFileFormatSupported` SHALL return false
