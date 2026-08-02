## Purpose

Lets users mark a favorite location as "starred" (favorite-of-favorites) for quick visual identification in lists and on the map.

## ADDED Requirements

### Requirement: Star/unstar a favorite

A favorite location SHALL support a boolean "starred" flag stored in its `attributes` map under the key `"starred"` with value `"true"` when starred. Absence of the key or any other value SHALL mean not starred.

The C++ `FavoriteLocationService` SHALL provide:
- `bool SetStarred(const std::string &groupName, const std::string &favName, bool starred)` — set or clear the star
- `bool IsStarred(const std::string &groupName, const std::string &favName) const` — query star state

The Java `OSMScoutClient` SHALL provide matching methods:
- `boolean setStarred(String groupName, String favName, boolean starred)`
- `boolean isStarred(String groupName, String favName)`

#### Scenario: Star a favorite
- **WHEN** `SetStarred("Work", "Office", true)` is called
- **THEN** `IsStarred("Work", "Office")` returns `true`
- **AND** `FavLocation.attributes["starred"]` equals `"true"`

#### Scenario: Unstar a favorite
- **WHEN** `SetStarred("Work", "Office", false)` is called on a previously starred favorite
- **THEN** `IsStarred("Work", "Office")` returns `false`
- **AND** the `"starred"` key SHALL be removed from `attributes`

#### Scenario: Star persists across save/load
- **WHEN** a starred favorite is saved to JSON and loaded back
- **THEN** `IsStarred()` still returns `true`

#### Scenario: Star on non-existent group or favorite
- **WHEN** `SetStarred("NonExistent", "X", true)` is called
- **THEN** it returns `false`

### Requirement: JavaScout shows star marker in fav list

The `FavLocationDialog` SHALL display a star icon (★) next to starred favorites in the favorites list. A toggle button SHALL allow starring/unstarring the selected favorite.

The `FavoritePickerDialog` SHALL show a star marker next to starred favorites in the tree.

#### Scenario: Star visible in fav list
- **WHEN** a favorite is starred
- **THEN** the fav list cell shows "★" prefix or suffix

#### Scenario: Toggle star from dialog
- **WHEN** user selects a favorite and clicks the star toggle button
- **THEN** the star state flips and the list cell updates immediately

#### Scenario: Star visible in picker tree
- **WHEN** `FavoritePickerDialog` loads favorites
- **THEN** starred favorites show a star marker in the tree cell text
