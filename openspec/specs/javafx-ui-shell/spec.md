# javafx-ui-shell

JavaFX application shell for JavaScout — window, config, database loading, status bar.

## Purpose

Foundation UI layer for the JavaScout desktop application. Provides the window, config file management, database directory scanning, and status bar. All future features (map rendering, search, routing) plug into this shell.

## Requirements

### Requirement: Application window
JavaScout SHALL display a JavaFX application window (Stage) with a title "JavaScout".

#### Scenario: Window appears on launch
- **WHEN** user launches JavaScout
- **THEN** a JavaFX window with title "JavaScout" SHALL appear

#### Scenario: Window has minimum size
- **WHEN** the window appears
- **THEN** it SHALL have a minimum width of 800px and minimum height of 600px

#### Scenario: No menu bar
- **WHEN** the window appears
- **THEN** there SHALL be no menu bar

### Requirement: Config file
JavaScout SHALL read configuration from `~/.config/javascout/config.properties` (Linux), `~/Library/Application Support/JavaScout/config.properties` (macOS), or `%APPDATA%\JavaScout\config.properties` (Windows).

#### Scenario: Config file exists with maps.directory
- **WHEN** the config file exists
- **AND** contains `maps.directory=/home/user/maps`
- **THEN** JavaScout SHALL use `/home/user/maps` as the database directory

#### Scenario: Config file does not exist
- **WHEN** no config file exists
- **AND** no CLI argument is provided
- **THEN** the window SHALL appear with empty state
- **AND** the status bar SHALL show "No maps directory configured"

#### Scenario: CLI argument overrides config
- **WHEN** user runs `javascout.sh /custom/path`
- **THEN** JavaScout SHALL use `/custom/path` as the database directory
- **AND** SHALL ignore the config file value

#### Scenario: CLI argument is saved to config
- **WHEN** user runs `javascout.sh /custom/path`
- **THEN** the config file SHALL be created/updated with `maps.directory=/custom/path`

### Requirement: Database directory scanning
JavaScout SHALL scan the configured database directory and load all `.osmscout` databases found within.

#### Scenario: Directory contains one database
- **WHEN** the database directory contains a valid `.osmscout` database
- **THEN** JavaScout SHALL call `OSMScoutClient.openDatabase()` with the directory path
- **AND** the status bar SHALL show the database directory path

#### Scenario: Directory contains multiple databases
- **WHEN** the database directory contains multiple `.osmscout` databases
- **THEN** JavaScout SHALL load all of them
- **AND** the status bar SHALL show the directory path

#### Scenario: Directory is invalid
- **WHEN** the configured directory does not contain valid `.osmscout` data
- **THEN** the window SHALL show empty state
- **AND** the status bar SHALL show "No databases found"

### Requirement: Map panel placeholder
The window SHALL contain a central content area reserved for map rendering.

#### Scenario: Empty map panel is visible
- **WHEN** the window appears
- **AND** no database is loaded
- **THEN** the central area SHALL show a placeholder message "No map loaded"

#### Scenario: Map panel fills available space
- **WHEN** the window is resized
- **THEN** the central map panel SHALL resize to fill the available space between top and status bar
- **AND** the map SHALL be redrawn to the new size
- **AND** the center of the map SHALL remain the same

### Requirement: Application passes display scale to overlays
The JavaScout application shell SHALL compute a UI scale factor from the primary screen DPI and make it available to overlay controls.

#### Scenario: DPI is detected on startup
- **WHEN** the application starts
- **THEN** the shell SHALL read the primary screen DPI
- **AND** compute a UI scale factor of at least 1.0
- **AND** pass the scale to search and route overlays

### Requirement: Keyboard shortcuts open search and route dialogs
The JavaScout shell SHALL provide keyboard shortcuts to open the search overlay and the route panel.

#### Scenario: Ctrl+F opens search
- **WHEN** the user presses Ctrl+F
- **THEN** the search overlay SHALL open

#### Scenario: Ctrl+R toggles route panel
- **WHEN** the user presses Ctrl+R
- **THEN** the route panel SHALL open if closed
- **AND** the route panel SHALL close if open

#### Scenario: Keyboard shortcuts only fire when map has focus
- **GIVEN** focus is in an overlay text field or result list
- **WHEN** the user presses Ctrl+F or Ctrl+R
- **THEN** the shortcut SHALL NOT trigger
- **AND** the native text-field behavior (if any) SHALL remain

#### Scenario: Clicking map gives it focus
- **WHEN** the user clicks on the map canvas
- **THEN** the map canvas SHALL gain focus
- **AND** subsequent Ctrl+F / Ctrl+R shortcuts SHALL trigger

### Requirement: Zoom overlay buttons
The JavaScout shell SHALL provide on-screen zoom in and zoom out buttons.

#### Scenario: Zoom in button increases magnification
- **WHEN** the user clicks the zoom in button
- **THEN** the map magnification SHALL increase by one level
- **AND** the map SHALL re-render with the new magnification

#### Scenario: Zoom out button decreases magnification
- **WHEN** the user clicks the zoom out button
- **THEN** the map magnification SHALL decrease by one level
- **AND** the map SHALL re-render with the new magnification

### Requirement: Status bar
The window SHALL contain a status bar at the bottom showing database directory and coordinates.

#### Scenario: Status bar shows database directory
- **WHEN** a database directory is loaded
- **THEN** the status bar SHALL display the directory path on the left side

#### Scenario: Status bar shows coordinate placeholder
- **WHEN** no database is loaded
- **THEN** the status bar SHALL show "Lat: -- Lon: --" on the right side

### Requirement: Maven build with JavaFX
The Maven build SHALL include JavaFX dependencies and support running via `mvn javafx:run`.

#### Scenario: Build succeeds with JavaFX
- **WHEN** user runs `mvn package` in JavaScout/
- **THEN** the build SHALL succeed
- **AND** produce an executable JAR

#### Scenario: Run via Maven plugin
- **WHEN** user runs `mvn javafx:run` in JavaScout/
- **THEN** the JavaFX application SHALL launch
