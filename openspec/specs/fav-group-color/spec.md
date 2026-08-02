# fav-group-color Specification

## Purpose

Lets users assign a color to a favorite group for visual organization, stored as a simple 6-character RGB hex string.

## Requirements

### Requirement: Assign color to a group

A `FavLocationGroup` SHALL support an optional color stored in its `attributes` map under the key `"color"` with a 6-character hex RGB value (e.g. `"FF5733"`). Absence of the key means no color assigned.

The C++ `FavoriteLocationService` SHALL provide:
- `bool SetGroupColor(const std::string &groupName, const std::string &color)` — set color (validated as 6 hex chars)
- `std::string GetGroupColor(const std::string &groupName) const` — return color string or empty

The Java `OSMScoutClient` SHALL provide matching methods:
- `boolean setGroupColor(String groupName, String color)`
- `String getGroupColor(String groupName)`

#### Scenario: Assign color to group
- **WHEN** `SetGroupColor("Work", "FF5733")` is called
- **THEN** `GetGroupColor("Work")` returns `"FF5733"`
- **AND** `FavLocationGroup.attributes["color"]` equals `"FF5733"`

#### Scenario: Remove color from group
- **WHEN** `SetGroupColor("Work", "")` is called on a group that had a color
- **THEN** `GetGroupColor("Work")` returns `""`
- **AND** the `"color"` key SHALL be removed from `attributes`

#### Scenario: Color persists across save/load
- **WHEN** a group with color is saved to JSON and loaded back
- **THEN** `GetGroupColor()` still returns the same color

#### Scenario: Invalid color rejected
- **WHEN** `SetGroupColor("Work", "XYZ")` is called with non-hex characters
- **THEN** it returns `false`
- **AND** the color is not changed

#### Scenario: Color on non-existent group
- **WHEN** `SetGroupColor("NonExistent", "FF5733")` is called
- **THEN** it returns `false`

### Requirement: JavaScout shows color swatch for groups

The `FavLocationDialog` SHALL display a small color swatch next to each group name in the group list when a color is assigned. A color picker button SHALL allow the user to pick or clear the group color.

The `FavoritePickerDialog` SHALL show a color swatch next to group names in the tree.

#### Scenario: Color swatch in group list
- **WHEN** a group has a color assigned
- **THEN** the group list cell shows a colored rectangle before the group name

#### Scenario: Color picker opens
- **WHEN** user clicks the color button for a group
- **THEN** a color picker dialog opens showing current color (or no color)

#### Scenario: Color applied from picker
- **WHEN** user picks a color in the picker and confirms
- **THEN** the group color is updated and the swatch refreshes immediately

#### Scenario: Color cleared
- **WHEN** user clears the color in the picker
- **THEN** the group color is removed and the swatch disappears
