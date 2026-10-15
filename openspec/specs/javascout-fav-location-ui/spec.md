## Purpose

Provide JavaScout UI for managing favorite locations and using them in search and routing.

## Requirements

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

### Requirement: Fav management dialog reorders groups

The dialog SHALL let the user change the order of the groups and SHALL show the groups in that order
instead of an alphabetical order. The dialog SHALL offer a way to move the selected group one position
towards the beginning and one position towards the end of the order. The change SHALL be visible in the
group list immediately and SHALL be persisted when the dialog saves, so the order is the same the next
time the dialog opens. Moving the first group further towards the beginning and the last group further
towards the end SHALL have no effect.

Every other place in JavaScout that presents groups SHALL follow the same order, so a group is not shown
in one place at one position and elsewhere at another.

#### Scenario: Groups are shown in the stored order

- **GIVEN** a favorites file whose group order is `Work,Home,Uni`
- **WHEN** the dialog opens
- **THEN** the group list SHALL show `Work`, then `Home`, then `Uni`

#### Scenario: Moving a group towards the beginning

- **GIVEN** a dialog showing the groups `Work,Home,Uni`
- **WHEN** the user selects `Uni` and moves it towards the beginning
- **THEN** the group list SHALL show `Work,Uni,Home`

#### Scenario: Moving a group towards the end

- **GIVEN** a dialog showing the groups `Work,Home,Uni`
- **WHEN** the user selects `Work` and moves it towards the end
- **THEN** the group list SHALL show `Home,Work,Uni`

#### Scenario: A move at the border of the list has no effect

- **GIVEN** a dialog showing the groups `Work,Home,Uni`
- **WHEN** the user moves `Work` towards the beginning and `Uni` towards the end
- **THEN** the group list SHALL still show `Work,Home,Uni`

#### Scenario: The group order persists

- **GIVEN** a dialog in which the groups were reordered to `Home,Uni,Work`
- **WHEN** the dialog is closed and opened again
- **THEN** the group list SHALL show `Home,Uni,Work`

#### Scenario: The favorite picker follows the group order

- **GIVEN** a favorites file whose group order is `Work,Home,Uni`
- **WHEN** the favorite picker is opened
- **THEN** it SHALL show the groups in the order `Work,Home,Uni`

#### Scenario: The favorites overlay follows the group order

- **GIVEN** a favorites file whose group order is `Work,Home,Uni`
- **WHEN** the user switches the search overlay to favorites
- **THEN** the groups SHALL be listed in the order `Work,Home,Uni`

### Requirement: Fav management dialog moves a fav to another group

The dialog SHALL let the user move the selected fav from its group into another group. The dialog SHALL
offer the groups as the possible destination. After a successful move the fav SHALL disappear from the
source group's list and appear in the destination group's list, and the map markers SHALL be refreshed
together with the rest of the favorite data. When the destination group already holds a fav of the name
being moved, the dialog SHALL report that the move was refused and SHALL leave both group lists
unchanged, instead of reporting a success or dropping a fav.

#### Scenario: A fav is moved to another group

- **GIVEN** a dialog showing the group `Home` with the favs `A,B` and the group `Work` with the favs `X,Y`
- **WHEN** the user selects `B` and moves it to the group `Work`
- **THEN** `Home`'s fav list SHALL show `A`
- **AND** `Work`'s fav list SHALL show `X,Y,B`

#### Scenario: A refused move is reported and changes nothing

- **GIVEN** a dialog showing the group `Home` with the favs `A,B` and the group `Work` with a fav named `B`
- **WHEN** the user selects `B` in `Home` and moves it to the group `Work`
- **THEN** the dialog SHALL report that the move was refused
- **AND** `Home`'s fav list SHALL still show `A,B`
- **AND** `Work`'s fav list SHALL be unchanged

#### Scenario: The new group assignment persists

- **GIVEN** a dialog in which a fav was moved from `Home` to `Work`
- **WHEN** the dialog is closed and opened again
- **THEN** the fav SHALL be listed under `Work` and not under `Home`

#### Scenario: A moved fav keeps its star

- **GIVEN** a dialog showing a starred fav in the group `Home`
- **WHEN** the user moves that fav to the group `Work`
- **THEN** the fav SHALL still be shown as starred in `Work`

### Requirement: Fav management dialog orders the starred favorites

The dialog SHALL present the starred favorites as a list in their stored order, spanning all groups, and
SHALL show for each entry the group that holds it. The dialog SHALL offer a way to move the selected
starred favorite one position towards the beginning and one position towards the end of that order. The
change SHALL be visible in the list immediately and SHALL be persisted when the dialog saves. Moving a
starred favorite that already sits at the border of the list SHALL have no effect.

The list SHALL contain exactly the starred favorites: starring a favorite SHALL add it at the end of the
list, and unstarring a favorite SHALL remove it from the list.

#### Scenario: Starred favorites are listed in their order

- **GIVEN** a favorites file whose starred order is `Office,Home,Gym`
- **WHEN** the dialog opens
- **THEN** the starred list SHALL show `Office`, then `Home`, then `Gym`

#### Scenario: Each starred entry shows its group

- **GIVEN** a starred favorite `Office` held by the group `Work`
- **WHEN** the starred list is shown
- **THEN** the entry for `Office` SHALL show the group `Work`

#### Scenario: Moving a starred favorite towards the beginning

- **GIVEN** a starred list showing `Office,Home,Gym`
- **WHEN** the user selects `Gym` and moves it towards the beginning
- **THEN** the starred list SHALL show `Office,Gym,Home`

#### Scenario: Moving a starred favorite towards the end

- **GIVEN** a starred list showing `Office,Home,Gym`
- **WHEN** the user selects `Office` and moves it towards the end
- **THEN** the starred list SHALL show `Home,Office,Gym`

#### Scenario: Starring adds to the end of the list

- **GIVEN** a starred list showing `Office,Home`
- **WHEN** the user stars the favorite `Gym`
- **THEN** the starred list SHALL show `Office,Home,Gym`

#### Scenario: Unstarring removes from the list

- **GIVEN** a starred list showing `Office,Home,Gym`
- **WHEN** the user unstars `Home`
- **THEN** the starred list SHALL show `Office,Gym`

#### Scenario: The starred order persists

- **GIVEN** a dialog in which the starred list was reordered to `Gym,Office,Home`
- **WHEN** the dialog is closed and opened again
- **THEN** the starred list SHALL show `Gym,Office,Home`

#### Scenario: Reordering stars does not reorder the groups

- **GIVEN** a dialog showing the groups `Work,Home,Uni` and a starred list
- **WHEN** the user reorders the starred list
- **THEN** the group list SHALL still show `Work,Home,Uni`

### Requirement: Fav management dialog reports a favorites file it cannot read

When the loaded favorites file carries a format version newer than the client supports, the dialog SHALL
report that the file was written by a newer version instead of presenting an empty favorites list, so the
user does not read the state as "all my favorites are gone". The dialog SHALL refuse to persist over that
file and SHALL report the refusal, and SHALL NOT offer a way to save, overwrite or otherwise replace the
file's content. Opening and closing the dialog SHALL leave the file unchanged.

For a file this client can read, whether written by this client or in the pre-version form, the dialog
SHALL NOT show that notice.

#### Scenario: A newer file is reported instead of shown as empty

- **GIVEN** a favorites file that carries a version newer than the client knows
- **WHEN** the dialog opens
- **THEN** the dialog SHALL report that the file was written by a newer version
- **AND** it SHALL NOT present the empty list as if the file had no favorites

#### Scenario: The dialog cannot overwrite a newer file

- **GIVEN** a dialog opened on a file written by a newer version
- **WHEN** the user attempts to save
- **THEN** the dialog SHALL report that the file cannot be written by this version
- **AND** the file SHALL still hold its content unchanged

#### Scenario: Opening and closing the dialog leaves a newer file alone

- **GIVEN** a dialog opened on a file written by a newer version
- **WHEN** the dialog is closed without any other action
- **THEN** the file content SHALL be byte-for-byte what it was before the dialog opened

#### Scenario: A readable file shows no notice

- **GIVEN** a favorites file in the pre-version form, and one written by this client
- **WHEN** the dialog opens for each
- **THEN** neither SHALL show the newer-version notice
- **AND** each SHALL present its own groups
