## ADDED Requirements

### Requirement: Fav management dialog allows full CRUD on groups and favs
The system SHALL provide a modal dialog window in JavaScout for managing favorite locations. The dialog SHALL display a list of groups. Selecting a group SHALL display its favs. The dialog SHALL provide buttons/actions to: add a new group, delete a group, add a fav to a group, delete a fav, and rename a fav.

#### Scenario: Dialog opens from main window
- **WHEN** user triggers "Manage favorites" action (menu or button)
- **THEN** a modal dialog SHALL open showing all groups

#### Scenario: Add group via dialog
- **WHEN** user clicks "Add group" and enters a name
- **THEN** a new empty group SHALL appear in the group list

#### Scenario: Delete group via dialog
- **WHEN** user selects a group and clicks "Delete group"
- **THEN** the group and all its favs SHALL be removed

#### Scenario: Add fav to group via dialog
- **WHEN** user selects a group, clicks "Add favorite", and enters name + coordinates
- **THEN** the fav SHALL appear in that group's fav list

#### Scenario: Delete fav via dialog
- **WHEN** user selects a fav and clicks "Delete"
- **THEN** the fav SHALL be removed from the group

#### Scenario: Rename fav via dialog
- **WHEN** user selects a fav and triggers rename
- **THEN** a rename prompt SHALL appear and the fav name SHALL update

#### Scenario: Dialog persists changes on save
- **WHEN** user makes changes and closes the dialog
- **THEN** all changes SHALL be persisted to the fav locations file

### Requirement: Search overlay includes fav location tab
The search overlay SHALL have a tab or toggle to switch between OSM location search and favorite locations browsing. When in fav mode, the overlay SHALL display the group structure and allow selecting a fav to navigate the map to it.

#### Scenario: Fav tab is visible in search overlay
- **WHEN** search overlay is expanded
- **THEN** a "Favorites" tab or toggle SHALL be visible alongside the search field

#### Scenario: Fav mode shows groups
- **WHEN** user switches to fav mode
- **THEN** the group list SHALL be displayed instead of search results

#### Scenario: Selecting a fav navigates the map
- **WHEN** user clicks a fav in fav mode
- **THEN** the map SHALL navigate to that fav's coordinates

#### Scenario: Search still works in OSM mode
- **WHEN** user switches back to OSM search mode
- **THEN** normal location search SHALL work as before

### Requirement: Route panel can use fav locations as start/destination
The route panel SHALL allow selecting a favorite location as start or destination. When the user taps a start/destination label, the location picker SHALL include an option to browse favorites.

#### Scenario: Route start can be set from favorites
- **WHEN** user taps "Set start" in route panel and selects "From favorites"
- **THEN** a fav picker SHALL open and selecting a fav SHALL set it as start

#### Scenario: Route destination can be set from favorites
- **WHEN** user taps "Set destination" in route panel and selects "From favorites"
- **THEN** a fav picker SHALL open and selecting a fav SHALL set it as destination

### Requirement: Fav locations persist across app restarts
The system SHALL store favorite locations in a JSON file in the OS-specific config directory (same location as `config.properties`). The file SHALL be loaded on app startup and saved on changes.

#### Scenario: Favs survive restart
- **WHEN** user adds a favorite, closes the app, and reopens
- **THEN** the favorite SHALL still be present

#### Scenario: Fav file path matches config directory
- **WHEN** the app starts
- **THEN** fav locations SHALL be loaded from `<configDir>/favorites.json`
